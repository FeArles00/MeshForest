/*
 * 03_ping_rx — over-the-air cross-test, RX side (two-board test).
 *
 * Derived from 02_radio_ping, RF configuration MIRRORED from the demo
 * firmware (MeshForestBoard.h): 434.0 MHz, 0 dBm, sync 0x12AD, RadioLib
 * defaults otherwise. Flash on the GATEWAY board (COM5) — this mirrors
 * the failing demo direction (node TX -> gateway RX).
 *
 * Prints RSSI + LQI for every packet, reports CRC mismatches (which
 * mean "link works, data corrupted"), and prints a 10 s heartbeat so
 * that silence is unambiguous (board alive but hearing nothing).
 *
 * Also holds the siren/buzzer pin LOW so the gateway stays quiet
 * while this runs (the demo firmware does the same).
 */

#include <RadioLib.h>

#define RF_FREQ_MHZ  434.0
#define RF_POWER_DBM 0
#define RF_SYNC0     0x12
#define RF_SYNC1     0xAD
// 25 kHz deviation, mirroring the demo config (decision D15): the RadioLib
// default of 5 kHz was measured too narrow for this module pair. Kept local
// so this diagnostic sketch stays standalone-buildable.
#define RF_FREQ_DEV_KHZ 25.0f

CC1101 radio = new Module(15, 4, RADIOLIB_NC, RADIOLIB_NC);  // CS=GPIO15, GDO0=GPIO4

volatile bool radioInterrupt = false;
ICACHE_RAM_ATTR void rxFlagSet(void) { radioInterrupt = true; }

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("[03_ping_rx] starting");
  Serial.print("[CC1101] init ... ");
  int state = radio.begin(RF_FREQ_MHZ);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("FAILED: %d\n", state);
    while (true) delay(10);
  }
  radio.setSyncWord(RF_SYNC0, RF_SYNC1, 0, false);
  radio.setFrequencyDeviation(RF_FREQ_DEV_KHZ);  // mirror demo config (MeshForestBoard.h rationale)
  radio.setOutputPower(RF_POWER_DBM);
  radio.setPacketReceivedAction(rxFlagSet);
  state = radio.startReceive();
  Serial.printf("ok (%.1f MHz, %d dBm, sync 0x%02X%02X) startReceive=%d\n",
                RF_FREQ_MHZ, RF_POWER_DBM, RF_SYNC0, RF_SYNC1, state);
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW);  // siren/buzzer OFF while testing
  Serial.println("[RX] waiting for pings ...");
}

uint32_t lastHeartbeat = 0;
uint32_t rxCount = 0;

void loop() {
  if (radioInterrupt) {
    radioInterrupt = false;
    uint8_t buf[8];
    int state = radio.readData(buf, 4);
    if (state == RADIOLIB_ERR_NONE) {
      uint32_t n;
      memcpy(&n, buf, 4);
      rxCount++;
      Serial.printf("[RX] ping #%lu  RSSI: %.1f dBm  LQI: %u\n",
                    (unsigned long)n, radio.getRSSI(), radio.getLQI());
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.println("[RX] CRC mismatch (link works, data corrupted)");
    } else {
      Serial.printf("[RX] read failed: %d\n", state);
    }
    radio.startReceive();
  }
  if (millis() - lastHeartbeat >= 10000UL) {
    lastHeartbeat = millis();
    Serial.printf("[RX] %lu s: %lu packets so far\n",
                  (unsigned long)(millis() / 1000UL), (unsigned long)rxCount);
  }
  delay(10);
}
