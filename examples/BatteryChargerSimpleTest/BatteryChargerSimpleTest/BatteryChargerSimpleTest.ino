// BatteryChargerSimpleTest - Simple test of BatteryCharger library with Teleplot output
// This example demonstrates basic usage of the BatteryCharger library to 
// monitor battery status and output data

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
  RGB_BUILTIN // RgbLedPin: portable constant for onboard WS2812, defined in all Arduino-ESP32 variants that have one
);
// Create a BatteryCharger instance with the specified configuration
BatteryCharger charger (config);

constexpr unsigned long SamplePeriodMs = 500;
unsigned long lastSampleMs = 0;

// Function to read battery status and output it in Teleplot format (key:value pairs)
void printTeleplotSamples() {

  if (!charger.shutdown()) {
    const bool done = charger.done();
    const bool charging = charger.charging();
    const float voltage = charger.voltage();
    const float precent = charger.percent();

    // Teleplot format: one metric per line, key:value
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
