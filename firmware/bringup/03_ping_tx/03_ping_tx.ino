/*
 * 03_ping_tx — over-the-air cross-test, TX side (two-board test).
 *
 * Derived from 02_radio_ping, with the RF configuration MIRRORED from
 * the demo firmware (MeshForestBoard.h): 434.0 MHz, 0 dBm, sync 0x12AD,
 * RadioLib defaults otherwise. Flash on the NODE board (COM7).
 *
 * Sends a 4-byte counter packet every 2 s using the same non-blocking
 * TX sequence that fixed the demo node's transmit() error -5
 * (startTransmit -> delay -> finishTransmit; no RX ISR attached).
 *
 * Diagnosis logic: if this prints "sent" ~20 times while the board
 * running 03_ping_rx stays silent, the fault is in the RF chain
 * (antenna / GDO0 wiring / crystal offset) — NOT in the demo firmware.
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

uint32_t counter = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("[03_ping_tx] starting");
  Serial.print("[CC1101] init ... ");
  int state = radio.begin(RF_FREQ_MHZ);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("FAILED: %d\n", state);
    while (true) delay(10);
  }
  radio.setSyncWord(RF_SYNC0, RF_SYNC1, 0, false);
  radio.setFrequencyDeviation(RF_FREQ_DEV_KHZ);  // mirror demo config (MeshForestBoard.h rationale)
  radio.setOutputPower(RF_POWER_DBM);
  Serial.printf("ok (%.1f MHz, %d dBm, sync 0x%02X%02X)\n",
                RF_FREQ_MHZ, RF_POWER_DBM, RF_SYNC0, RF_SYNC1);
  Serial.println("[TX] ping every 2 s");
}

void loop() {
  counter++;
  uint8_t payload[4];
  memcpy(payload, &counter, 4);
  Serial.printf("[TX] ping #%lu ... ", (unsigned long)counter);
  int state = radio.startTransmit(payload, 4);
  if (state == RADIOLIB_ERR_NONE) {
    delay(100);  // 4 bytes at 4.8 kbps clears the air in ~15 ms
    radio.finishTransmit();
    Serial.println("sent");
  } else {
    Serial.printf("FAILED: %d\n", state);
  }
  delay(2000);
}
