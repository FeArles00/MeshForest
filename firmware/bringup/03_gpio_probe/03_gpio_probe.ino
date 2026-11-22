/*
 * 03_gpio_probe — identifies which GPIO hides behind each silkscreen label.
 * Made for our non-standard NodeMCU clone (docs/esp_pinout.md).
 *
 * HOW TO USE:
 *  1. Flash this, open the serial monitor (115200).
 *  2. The sketch prints the state of every usable GPIO every 300 ms.
 *  3. Touch a jumper from the pin you want to identify to GND.
 *  4. The line ">>> GPIO x changed" tells you the mapping.
 *
 * RULES (read before probing):
 *  - Only probe pins NOT wired to a peripheral (leave MOSI/SCLK/MISO/CSO
 *    and the DHT pin alone — we identify those differently).
 *  - Never hold a pin to GND while pressing RESET: GPIO0/GPIO2/GPIO15 low
 *    at boot change the boot mode (harmless, but the sketch won't start).
 *  - GPIO16 has only an internal pull-DOWN on ESP8266; it is included but
 *    reads differently (touch it to 3V3 to see it go high).
 */

static const uint8_t PINS[] = {0, 2, 4, 5, 12, 13, 14, 15, 16};
static const uint8_t NPin = sizeof(PINS);
static int prev[sizeof(PINS)];        // per-index (GPIO16 safe)

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println();
  Serial.println("=== MeshForest GPIO probe ===");
  Serial.printf("chip id:  0x%06X  (ESP8266 expected)\n", ESP.getChipId());
  Serial.printf("flash:    %u KB real, %u KB mapped\n",
                ESP.getFlashChipRealSize() / 1024,
                ESP.getFlashChipSize() / 1024);
  Serial.printf("core:     %s\n", ESP.getCoreVersion().c_str());
  Serial.printf("reset:    %s\n", ESP.getResetReason().c_str());
  Serial.println();
  Serial.println("Probing GPIOs 0,2,4,5,12,13,14,15,16 (+A0).");
  Serial.println("Touch the pin under test to GND (GPIO16: to 3V3).");
  Serial.println();

  // GPIO12/13/14 carry the CC1101 (high-Z when deselected -> safe to read),
  // GPIO0 carries the DHT22 pull-up (so it is left alone here).
  const uint8_t pins[] = {0, 2, 4, 5, 12, 13, 14, 15};
  for (uint8_t i = 0; i < sizeof(pins); i++)
    pinMode(pins[i], INPUT_PULLUP);
  pinMode(16, INPUT_PULLDOWN_16);
  for (uint8_t i = 0; i < NPin; i++) prev[i] = -1;
}

void loop() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < 300) return;
  last = now;

  bool anyChange = false;
  for (uint8_t i = 0; i < NPin; i++) {
    int v = digitalRead(PINS[i]);
    if (prev[i] != -1 && prev[i] != v) {
      Serial.printf(">>> GPIO %u changed %d -> %d  (label under test!)\n",
                    PINS[i], prev[i], v);
      anyChange = true;
    }
    prev[i] = v;
  }
  int a0 = analogRead(A0);

  if (anyChange) Serial.println("------------------------------------------------");
  int cur[sizeof(PINS)];
  for (uint8_t i = 0; i < NPin; i++) cur[i] = prev[i];
  Serial.printf("[0:%d 2:%d 4:%d 5:%d 12:%d 13:%d 14:%d 15:%d 16:%d] A0:%d\n",
                cur[0], cur[1], cur[2], cur[3], cur[4], cur[5],
                cur[6], cur[7], cur[8], a0);
}
