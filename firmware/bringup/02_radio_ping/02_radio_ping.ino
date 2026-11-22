/*
 * 02_radio_ping — bring-up step 2: CC1101 SPI init + desk-range ping.
 *
 * Flash this on board #1 with ROLE = ROLE_RX, then on board #2 with
 * ROLE = ROLE_TX (change the line below and re-flash).
 *
 * WIRING (CONFIRMED map for our clone — docs/hardware.md §4, D-labels are
 * raw GPIO numbers):
 *   CC1101 VCC  -> 3V3        (NEVER 5 V; module is 1.8-3.6 V)
 *   CC1101 GND  -> GND        (common ground)
 *   CC1101 SI/MOSI -> "D13" label (GPIO13)
 *   CC1101 SO/MISO -> "D12" label (GPIO12)
 *   CC1101 SCK/SCLK -> "D14" label (GPIO14)
 *   CC1101 CSN  -> "D15" label (GPIO15)
 *   CC1101 GDO0 -> "D4" label (GPIO4)   (NOT D2! D2=GPIO2 killed boot)
 *
 *   Do NOT use the pins printed MOSI/SCLK/MISO/CSO: they are different nets
 *   on this clone and the radio does not answer there (2026-11-06).
 *   CC1101 GDO2 -> not connected (RadioLib makes it optional)
 *
 * !!! Screw the antenna onto the module BEFORE applying power. Transmitting
 * without an antenna can damage the transmitter.
 *
 * Radio settings for bring-up: RadioLib CC1101 defaults (434 MHz, 4.8 kbps,
 * 5 kHz deviation, 135 kHz RX BW, 16-bit preamble) with output power stepped
 * DOWN to 0 dBm — plenty for a desk, conservative legally. Protocol radio
 * parameters get set properly in the next firmware stage (protocol-spec §14).
 */

#define ROLE_RX 0
#define ROLE_TX 1
#define ROLE ROLE_RX        // <— set to ROLE_TX on the second board

#include <RadioLib.h>

CC1101 radio = new Module(15, 4, RADIOLIB_NC, RADIOLIB_NC);  // CS=GPIO15(CSO), GDO0=GPIO4(D4)

volatile bool radioInterrupt = false;

#if defined(ESP8266)
ICACHE_RAM_ATTR
#endif
void rxFlagSet(void) {
  radioInterrupt = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("[02_radio_ping] starting");

  Serial.print("[CC1101] Initializing ... ");
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("success");
  } else {
    Serial.print("failed, code ");
    Serial.println(state);
    Serial.println("[CC1101] Check wiring: VCC->3V3, GND, SI->MOSI, "
                   "SO->MISO, SCK->SCLK, CSN->CSO, GDO0->D4");
    while (true) { delay(10); }
  }

  // conservative bench power (protocol radio params come in the next stage)
  state = radio.setOutputPower(0);
  Serial.print("[CC1101] Output power 0 dBm ... ");
  Serial.println(state == RADIOLIB_ERR_NONE ? "ok" : String(state));

#if ROLE == ROLE_RX
  radio.setPacketReceivedAction(rxFlagSet);
  state = radio.startReceive();
  Serial.print("[RX] startReceive ... ");
  Serial.println(state == RADIOLIB_ERR_NONE ? "ok" : String(state));
  Serial.println("[RX] waiting for pings ...");
#else
  Serial.println("[TX] sending a ping every 2 s ...");
#endif
}

uint32_t counter = 0;

void loop() {
#if ROLE == ROLE_TX
  counter++;
  uint8_t payload[4];
  memcpy(payload, &counter, 4);
  Serial.print("[TX] ping #");
  Serial.print(counter);
  Serial.print(" ... ");
  int state = radio.transmit(payload, 4);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("sent");
  } else {
    Serial.print("failed, code ");
    Serial.println(state);
  }
  delay(2000);
#else
  if (radioInterrupt) {
    radioInterrupt = false;
    uint8_t buf[8];
    int state = radio.readData(buf, 4);
    if (state == RADIOLIB_ERR_NONE) {
      uint32_t n;
      memcpy(&n, buf, 4);
      Serial.print("[RX] ping #");
      Serial.print(n);
      Serial.print("  RSSI: ");
      Serial.print(radio.getRSSI(), 1);
      Serial.print(" dBm  LQI: ");
      Serial.println(radio.getLQI());
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.println("[RX] packet CRC mismatch (link works, data corrupted)");
    } else {
      Serial.print("[RX] read failed, code ");
      Serial.println(state);
    }
    radio.startReceive();
  }
  delay(10);
#endif
}
