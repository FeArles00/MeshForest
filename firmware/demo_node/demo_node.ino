/*
 * MeshForest demo node — sensor-node role (hackathon demo, 2026-10-26).
 *
 * Hardware: NodeMCU V3 + CC1101 (see MeshForestBoard.h pins) + DHT22 +
 * flame-sensor module (digital output). Sends SENSOR_REPORT every
 * REPORT_INTERVAL_MS and an ALERT (with retries + dedup + MIC) when the
 * flame sensor triggers with debounce. Relays other traffic per protocol
 * v1 (managed flood). Everything is authenticated with the demo PSK and
 * verified at boot against the golden vectors from the Python model.
 *
 * DEMO simplifications (documented): msg_id counter not persisted (spec
 * §6); no MQ-2 on the critical path (decision D14).
 */

#include <YankiProtocol.h>
#include <MeshForestBoard.h>
#include <RadioLib.h>
#include <DHT.h>

CC1101 radio = new Module(PIN_RADIO_CS, PIN_RADIO_GDO0, RADIOLIB_NC, RADIOLIB_NC);
static Proto::Node node(ID_NODE1, DEMO_PSK, DEMO_NET_ID, false);
static DHT dht(PIN_DHT22, DHT22);

volatile bool rxFlag = false;
ICACHE_RAM_ATTR void onRxDone(void) { rxFlag = true; }

static uint32_t lastReportMs = 0;
static uint32_t flameSeenMs = 0;      // 0 = not currently seeing flame
static bool flameActive = false;
static uint32_t lastAlertMs = 0;
static uint16_t lastAlertMid = 0;
static bool confirmPrinted = true;

static void radioToRx() {
  if (radio.startReceive() != RADIOLIB_ERR_NONE)
    Serial.println("[radio] startReceive FAILED");
}

static void drainTx() {
  uint8_t pkt[Proto::MAX_PACKET_LEN], len;
  while (node.popTx(pkt, &len)) {
    radio.standby();                 // explicit: leave RX before TX
    radio.clearPacketReceivedAction();  // don't let the RX ISR fire during TX
    int st = radio.startTransmit(pkt, len);
    if (st == RADIOLIB_ERR_NONE) {
      // non-blocking: wait long enough for the packet to clear the air
      // 17-19 bytes at 4.8 kbps ≈ 40 ms; 100 ms gives ample margin
      delay(100);
      radio.finishTransmit();
      Serial.printf("[tx] %u bytes\n", len);
    } else {
      Serial.printf("[tx] FAILED: %d\n", st);
    }
    radio.setPacketReceivedAction(onRxDone);
    radioToRx();
  }
}

static uint8_t mapResetReason() {
  String r = ESP.getResetReason();
  if (r == "Power on" || r == "External System") return 0;
  if (r == "Hardware Watchdog") return 1;
  if (r == "Exception") return 2;
  if (r == "Software reset" || r == "Software/System restart") return 3;
  if (r == "Deep-Sleep Wake") return 4;
  return 0xFF;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println();
  Serial.printf("=== MeshForest demo node, id=0x%02X ===\n", ID_NODE1);
  if (!node.selfTest(Serial)) {
    Serial.println("[selftest] FAILED — HALTING (this is a firmware bug, "
                   "report it, do not demo with it)");
    while (true) delay(10);
  }
  randomSeed(ESP.getCycleCount());
  Serial.print("[radio] CC1101 init ... ");
  int st = radio.begin(RF_FREQ_MHZ);
  if (st != RADIOLIB_ERR_NONE) {
    Serial.printf("FAILED: %d\n", st);
    Serial.println("       wiring: VCC->3V3, GND, SCLK->D14, MOSI->D13, "
                   "MISO->D12, CSN->D15, GDO0->D4, antenna on!");
    while (true) delay(10);
  }
  radio.setSyncWord(RF_SYNC0, RF_SYNC1, 0, false);
  radio.setFrequencyDeviation(RF_FREQ_DEV_KHZ);
  radio.setOutputPower(RF_POWER_DBM);
  radio.setPacketReceivedAction(onRxDone);
  radioToRx();
  Serial.printf("[radio] ok: %.1f MHz, %d dBm\n", RF_FREQ_MHZ, RF_POWER_DBM);
  dht.begin();
  pinMode(PIN_FLAME, INPUT);
  node.originateStatus(millis(), Proto::NF_BOOTED, 0, mapResetReason());
  Serial.printf("[node] up: reporting every %lu s, watching the flame pin\n",
                (unsigned long)(REPORT_INTERVAL_MS / 1000UL));
}

void loop() {
  uint32_t now = millis();

  if (rxFlag) {
    rxFlag = false;
    uint8_t buf[Proto::MAX_PACKET_LEN];
    size_t len = radio.getPacketLength();
    if (len == 0 || len > Proto::MAX_PACKET_LEN) len = Proto::MAX_PACKET_LEN;
    int st = radio.readData(buf, len);
    Serial.printf("[rx] len=%u read=%d RSSI=%.1f LQI=%u\n",
                  (unsigned)len, st, radio.getRSSI(), radio.getLQI());
    if (st == RADIOLIB_ERR_NONE) {
      node.onReceive(buf, (uint8_t)len, now);
    }
    radioToRx();
  }

  node.tick(now);
  drainTx();

  if (lastAlertMid != 0 && !confirmPrinted &&
      node.alertConfirmed(lastAlertMid)) {
    Serial.println("[alert] CONFIRMED — gateway ACK received");
    confirmPrinted = true;
  }

  // ---- periodic sensor report ------------------------------------------
  if (now - lastReportMs >= REPORT_INTERVAL_MS) {
    lastReportMs = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    bool dhtOk = !isnan(t) && !isnan(h);
    int16_t temp = dhtOk ? (int16_t)round(t * 10.0f) : Proto::TEMP_MISSING;
    uint16_t hum = dhtOk ? (uint16_t)round(h * 10.0f) : Proto::HUM_MISSING;
    uint8_t flags = Proto::SF_DHT_PRESENT | Proto::SF_FLAME_PRESENT;
    if (dhtOk) flags |= Proto::SF_DHT_VALID;
    if (flameActive) flags |= Proto::SF_FLAME_ACTIVE;
    node.originateReport(now, flags, temp, hum);
    Serial.printf("[report] temp=%d hum=%u%s\n", temp, hum,
                  dhtOk ? "" : " (DHT invalid)");
  }

  // ---- flame detection: debounce -> ALERT, re-arm after clear ----------
  bool flameRaw = (digitalRead(PIN_FLAME) == FLAME_ACTIVE_LEVEL);
  if (flameRaw) {
    if (flameSeenMs == 0) flameSeenMs = now;
    if (!flameActive && (now - flameSeenMs) >= FLAME_DEBOUNCE_MS) {
      flameActive = true;
      if ((now - lastAlertMs) >= ALERT_REARM_MS) {
        lastAlertMs = now;
        float t = dht.readTemperature();
        float h = dht.readHumidity();
        bool dhtOk = !isnan(t) && !isnan(h);
        int16_t temp =
            dhtOk ? (int16_t)round(t * 10.0f) : Proto::TEMP_MISSING;
        uint16_t hum =
            dhtOk ? (uint16_t)round(h * 10.0f) : Proto::HUM_MISSING;
        uint8_t snap = Proto::SF_DHT_PRESENT | Proto::SF_FLAME_PRESENT |
                       Proto::SF_FLAME_ACTIVE;
        if (dhtOk) snap |= Proto::SF_DHT_VALID;
        lastAlertMid = node.originateAlert(now, Proto::AF_FLAME,
                                           Proto::LEVEL_CRITICAL, snap,
                                           temp, hum);
        confirmPrinted = false;
        Serial.printf("[ALERT] FLAME — alert 0x%04X out "
                      "(retries +3/+8/+20 s until ACKed)\n", lastAlertMid);
      }
    }
  } else {
    flameSeenMs = 0;
    if (flameActive) {
      flameActive = false;
      Serial.println("[flame] condition cleared (no CLEAR message in v1)");
    }
  }
}
