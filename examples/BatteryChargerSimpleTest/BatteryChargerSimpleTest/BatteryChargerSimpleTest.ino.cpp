# 1 "C:\\Users\\pasca\\AppData\\Local\\Temp\\tmp7axyevdi"
#include <Arduino.h>
# 1 "C:/Users/pasca/src/BatteryCharger/examples/BatteryChargerSimpleTest/BatteryChargerSimpleTest/BatteryChargerSimpleTest.ino"




#include <Arduino.h>
#include <BatteryCharger.h>



BatteryCharger::Config config (
  A0,
  5,
  19,
  2.0f,
  21,
  true,
  RGB_BUILTIN
);

BatteryCharger charger (config);

constexpr unsigned long SamplePeriodMs = 500;
unsigned long lastSampleMs = 0;
void printTeleplotSamples();
void setup();
void loop();
#line 26 "C:/Users/pasca/src/BatteryCharger/examples/BatteryChargerSimpleTest/BatteryChargerSimpleTest/BatteryChargerSimpleTest.ino"
void printTeleplotSamples() {

  if (!charger.shutdown()) {
    const bool done = charger.done();
    const bool charging = charger.charging();
    const float voltage = charger.voltage();
    const float precent = charger.percent();


    Serial.printf ("done:%d\n", done ? 1 : 0);
    Serial.printf ("charging:%d\n", charging ? 1 : 0);
    Serial.printf ("voltage:%.3f\n", voltage);
    Serial.printf ("precent:%.1f\n", precent);
  }
}

void setup() {

  Serial.begin (115200);
  delay (2000);

  if (!charger.begin()) {
    Serial.println ("Failed to initialize BatteryCharger");
    while (1);
  }

  Serial.println ("BatteryCharger test ready");
}

void loop() {
  const unsigned long now = millis();
  if ( (now - lastSampleMs) >= SamplePeriodMs) {
    lastSampleMs = now;
    printTeleplotSamples();
  }
}