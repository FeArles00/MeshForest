/*
 * 01_blink — bring-up step 1: proves board + USB driver + flashing + serial.
 * Expected serial output (115200 baud): the boot banner and a reset reason.
 *
 * No external wiring required. Onboard blue LED on NodeMCU = GPIO2,
 * active LOW (the core's LED_BUILTIN is preconfigured for nodemcuv2).
 */

void setup() {
  Serial.begin(115200);
  delay(1000);                       // let the USB-serial settle
  Serial.println();
  Serial.println("[01_blink] NodeMCU alive. Flash + serial OK.");
  Serial.print("[01_blink] Reset reason: ");
  Serial.println(ESP.getResetReason());
  Serial.print("[01_blink] Core version: ");
  Serial.println(ESP.getCoreVersion());
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);   // LED off (active low)
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);    // on
  delay(200);
  digitalWrite(LED_BUILTIN, HIGH);  // off
  delay(800);
}
