// BatteryChargerFullTest - Full test of BatteryCharger library with Teleplot output
// This example demonstrates voltage, state-of-charge, charge current and battery temperature monitoring.

#include <Arduino.h>
#include <BatteryCharger.h>

// Pin definitions and configuration parameters for the battery charger
// <WARNING> Adjust these pin numbers and parameters according to your hardware setup !!
BatteryCharger::Config config (
  A0, // vBatPin, ADC pin for battery voltage measurement, VBatScalingFactor is applied to this reading to get actual battery voltage
  5, // DonePin, charging complete when LOW
  19, // ChrgPin, charging in progress when LOW
  2.0f, // VBatScalingFactor, resistor divider scaling factor for voltage measurement (e.g. 2.0 for 100k/100k divider)
  21, // ShutdownPin, -1 to disable this pin
  true, // shutdownActiveLow, shutdown is active when LOW
  RGB_BUILTIN, // RgbLedPin: portable constant for onboard WS2812, defined in all Arduino-ESP32 variants that have one
  A1, // iBatPin (ISET monitor ADC pin)
  3000, // iSetResistanceOhms (RISET resistor value)
  BatteryCharger::Config::IBAT_FACTOR_CN306X, // ISET factor (CN306X=900, CN316X=986)
  A2, // ntcPin (battery NTC divider node)
  4700, // ntcResistanceOhms at 25 C
  4700 // ntcPullupResistanceOhms
);

BatteryCharger charger (config);

constexpr unsigned long SamplePeriodMs = 500;
unsigned long lastSampleMs = 0;

void printTeleplotSamples() {

  if (!charger.shutdown()) {
    const bool done = charger.done();
    const bool charging = charger.charging();
    const float voltage = charger.voltage();
    const float percent = charger.percent();
    const float iBatmA = charger.chargeCurrentmA();
    const float tBatC = charger.batteryTempC();

    Serial.printf ("done:%d\n", done ? 1 : 0);
    Serial.printf ("charging:%d\n", charging ? 1 : 0);
    Serial.printf ("voltage:%.3f\n", voltage);
    Serial.printf ("percent:%.1f\n", percent);
    Serial.printf ("ibat_ma:%.1f\n", iBatmA);
    Serial.printf ("tbat_c:%.1f\n", tBatC);
  }
}

void setup() {

  Serial.begin (115200);
  delay (2000);

  if (!charger.begin()) {
    Serial.println ("Failed to initialize BatteryCharger");
    while (1);
  }

  Serial.println ("BatteryCharger full test ready");
}

void loop() {
  const unsigned long now = millis();
  if ( (now - lastSampleMs) >= SamplePeriodMs) {
    lastSampleMs = now;
    printTeleplotSamples();
  }
}
