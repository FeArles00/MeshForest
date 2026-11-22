/*
 * MeshForest demo gateway (hackathon demo, 2026-10-26).
 *
 * Hardware: NodeMCU V3 + CC1101 (same pin plan as the nodes) + speaker/
 * siren module on PIN_SIREN (D16/GPIO16). Receives the managed flood,
 * dedups alerts, ACKs them (sole ACK authority, protocol v1 §9.3), fires
 * the siren once per unique alert, and drives a 128x64 SSD1306 OLED on the
 * I2C pins from MeshForestBoard.h (D2/D5) plus a live serial status line.
 *
 * DEMO simplification: the OLED shows the latest alert/report only — there
 * is no per-node freshness table (protocol-spec §12.2 unimplemented).
 */

#include <YankiProtocol.h>
#include <MeshForestBoard.h>
#include <RadioLib.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

CC1101 radio = new Module(PIN_RADIO_CS, PIN_RADIO_GDO0, RADIOLIB_NC, RADIOLIB_NC);
static Proto::Node node(ID_GATEWAY, DEMO_PSK, DEMO_NET_ID, true);

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);  // no reset pin

static void oledInit() {
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  bool ok = false;
  for (uint8_t addr = 0x3C; addr <= 0x3F && !ok; addr++) {
    if (display.begin(SSD1306_SWITCHCAPVCC, addr)) {
      ok = true;
      Serial.printf("[oled] SSD1306 found at 0x%02X\n", addr);
    }
  }
  if (!ok) {
    Serial.println("[oled] NOT FOUND at any address — check wiring");
    return;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("MeshForest Gateway");
  display.println("listening...");
  display.display();
}

static void oledShowAlert(uint8_t src, uint8_t level, int16_t temp, uint16_t hum) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(level >= Proto::LEVEL_CRITICAL ? "!! FIRE !!" : "! WARNING");
  display.setTextSize(1);
  display.printf("node 0x%02X  lvl %u\n", src, level);
  display.printf("temp %d.%dC\n", temp / 10, temp < 0 ? -temp % 10 : temp % 10);
  display.printf("hum  %u.%u%%\n", hum / 10, hum % 10);
  display.printf("ACK sent, escalate");
  display.display();
}

static void oledShowIdle() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("MeshForest Gateway");
  const Proto::Node::ReportInfo& r = node.lastReport();
  if (r.valid) {
    display.printf("node 0x%02X  temp %d\n", r.src, r.temp);
    display.printf("hum %u  ok\n", r.hum);
  } else {
    display.println("no reports yet");
  }
  display.printf("rx=%u tx=%u siren=%lu\n",
                 node.stats().rx_ok, node.stats().tx,
                 node.sirenEvents());
  display.display();
}

volatile bool rxFlag = false;
ICACHE_RAM_ATTR void onRxDone(void) { rxFlag = true; }

static uint32_t sirenUntilMs = 0;
static uint32_t lastStatusMs = 0;
static uint32_t sirenCountPrev = 0;

static void radioToRx() {
  if (radio.startReceive() != RADIOLIB_ERR_NONE)
    Serial.println("[radio] startReceive FAILED");
}

static void drainTx() {
  uint8_t pkt[Proto::MAX_PACKET_LEN], len;
  while (node.popTx(pkt, &len)) {
    radio.standby();
    radio.clearPacketReceivedAction();
    int st = radio.startTransmit(pkt, len);
    if (st == RADIOLIB_ERR_NONE) {
      delay(100);
      radio.finishTransmit();
      Serial.printf("[tx] %u bytes (ACK or relay)\n", len);
    } else {
      Serial.printf("[tx] FAILED: %d\n", st);
    }
    radio.setPacketReceivedAction(onRxDone);
    radioToRx();
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println();
  Serial.println("=== MeshForest demo GATEWAY, id=0x01 ===");
  if (!node.selfTest(Serial)) {
    Serial.println("[selftest] FAILED — HALTING (firmware bug; report it)");
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
  pinMode(PIN_SIREN, OUTPUT);
  digitalWrite(PIN_SIREN, LOW);
  Serial.printf("[radio] ok: %.1f MHz, %d dBm\n", RF_FREQ_MHZ, RF_POWER_DBM);
  oledInit();
  Serial.println("[gateway] listening for reports and alerts ...");
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

  // ---- siren: fire once per unique alert (level != TEST) ----------------
  uint32_t events = node.sirenEvents();
  if (events != sirenCountPrev) {
    sirenCountPrev = events;
    const Proto::Node::AlertInfo& a = node.lastAlert();
    Serial.println("********************************************");
    Serial.printf("* ALERT from node 0x%02X  level=%u  id=0x%04X\n",
                   a.src, a.level, a.mid);
    Serial.printf("* evidence: temp=%d (0.1C) hum=%u (0.1%%RH)\n",
                   a.temp, a.hum);
    Serial.println("* ACK sent — escalating to authorized personnel");
    Serial.println("********************************************");
    oledShowAlert(a.src, a.level, a.temp, a.hum);
    sirenUntilMs = now + SIREN_MS;
    digitalWrite(PIN_SIREN, HIGH);
  }
  if (sirenUntilMs != 0 && (int32_t)(now - sirenUntilMs) >= 0) {
    digitalWrite(PIN_SIREN, LOW);
    sirenUntilMs = 0;
  }

  // ---- periodic status line (freshness display, serial version) ----------
  if (now - lastStatusMs >= 30000UL) {
    lastStatusMs = now;
    const Proto::Node::ReportInfo& r = node.lastReport();
    const Proto::NodeStats& s = node.stats();
    if (r.valid) {
      Serial.printf("[status] last report: node 0x%02X, temp=%d, hum=%u\n",
                    r.src, r.temp, r.hum);
    } else {
      Serial.println("[status] no reports yet (silence = unknown, not 'ok')");
    }
    Serial.printf("[status] rx=%u tx=%u dup=%u mic=%u len=%u net=%u ver=%u win=%u forger=%u siren=%lu\n",
                  s.rx_ok, s.tx, s.drop_dup, s.drop_mic, s.drop_len,
                  s.drop_net, s.drop_ver, s.drop_window, s.drop_forger, events);
    oledShowIdle();
  }
}
