# BatteryCharger Library

A comprehensive C++ library for Li-Ion battery charger management and battery monitoring on ESP32 and Arduino platforms.

[![Build](https://github.com/epsilonrt/BatteryCharger/actions/workflows/build.yml/badge.svg)](https://github.com/epsilonrt/BatteryCharger/actions/workflows/build.yml)
[![GitHub release (latest by date including pre-releases)](https://img.shields.io/github/v/release/epsilonrt/BatteryCharger?include_prereleases)](https://github.com/epsilonrt/BatteryCharger/releases)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/epsilonrt/library/BatteryCharger.svg)](https://registry.platformio.org/libraries/epsilonrt/BatteryCharger)  
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue)](https://www.arduino.cc/)

## Features

This library is designed primarily for charger circuits based on the CN306X and CN316X families.

- **Battery Voltage Measurement**: ADC-based voltage acquisition with EMA filtering for noise reduction
- **Charge Status Monitoring**: Detection of DONE (charge complete) and CHG (charging in progress) pin states
- **Optional Features** (via configuration):
  - Battery temperature measurement using NTC thermistor (with Beta equation conversion)
  - Charge current estimation via ISET pin of charger IC (CN306X/CN316X families)
  - WS2812 RGB status LED control via FreeRTOS task (ESP32)
  - Measurement circuit shutdown/low-power mode control

## Hardware Requirements

### Mandatory
- ADC-capable microcontroller (ESP32, Arduino, STM32, etc.)
- Li-Ion charger IC (e.g., CN3063, CN3165, MCP73871) with:
  - VBAT output (voltage divider for ADC measurement)
  - DONE pin (charge completion indicator, active low)
  - CHG pin (charging status, active low)

### Optional
- GPIO for measurement circuit shutdown control
- GPIO for WS2812 RGB LED
- GPIO for NTC thermistor temperature measurement
- GPIO for ISET pin current measurement

**Note:** ISET current measurement is currently supported only for charger ICs from CN306X and CN316X families.

For the TEMP/NTC input, a practical and coherent circuit is:
- battery positive -> fixed pull-up resistor -> TEMP/ADC node -> NTC -> battery negative
- choose the fixed resistor equal to the NTC nominal resistance at 25 C (typically 10 kOhm)

This places the TEMP node near 50% of battery voltage at 25 C, which matches well the valid temperature window described by CN306X/CN316X datasheets.

## Installation

### PlatformIO (Recommended)
Add to `platformio.ini`:
```ini
lib_deps =
    BatteryCharger @ ^1.0.0
```

### Arduino IDE
1. Download the library as ZIP from repository
2. Sketch → Include Library → Add .ZIP Library...
3. Select the BatteryChargerZIP file

## Quick Start

```cpp
#include <BatteryCharger.h>

// Configuration with required parameters
BatteryCharger::Config config(
    2,      // vBatPin (ADC pin for voltage)
    19,     // donePin (GPIO for DONE indicator)
    20,     // chrgPin (GPIO for CHG indicator)
    1.997f  // vBatScalingFactor (voltage divider ratio)
);

// Create charger instance
BatteryCharger charger(config);

void setup() {
    Serial.begin(115200);
    
    // Initialize charger
    if (!charger.begin()) {
        Serial.println("Failed to initialize charger!");
        return;
    }
}

void loop() {
    // Get battery voltage
    float voltage = charger.voltage();  // in volts
    
    // Get state of charge
    float soc = charger.percent();      // 0-100%
    
    // Check charging status
    bool charging = charger.charging();
    bool done = charger.done();
    
    Serial.printf("V: %.2f V, SOC: %.0f%%, Charging: %d, Done: %d\n",
                  voltage, soc, charging, done);
    
    delay(500);
}
```

## Configuration Structure

The `BatteryCharger::Config` struct controls all aspects of the library:

```cpp
struct BatteryCharger::Config {
    static constexpr float IBAT_FACTOR_CN306X = 900.0f;
    static constexpr float IBAT_FACTOR_CN316X = 986.0f;
    static constexpr float NTC_NOMINAL_TEMP_C = 25.0f;
    static constexpr float NTC_BETA_TYPICAL = 3950.0f;

    int8_t vBatPin;              // ADC pin for battery voltage (required)
    int8_t donePin;              // GPIO for DONE indicator (required)
    int8_t chrgPin;              // GPIO for CHG indicator (required)
    float vBatScalingFactor;     // Voltage divider scaling (required, typ. 1.9-2.0)
    
    int8_t shutdownPin;          // GPIO for shutdown control (-1 = disabled)
    bool shutdownActiveLow;      // Shutdown polarity (true = active low)
    int8_t rgbLedPin;            // GPIO for WS2812 LED (-1 = disabled)
    int8_t iBatPin;              // ADC pin for current measurement (-1 = disabled)
    uint16_t iSetResistanceOhms; // RISET value in ohms (default: 2000)
    float iBatCurrentFactor;     // ISET factor (use IBAT_FACTOR_CN306X/CN316X)
    int8_t ntcPin;               // ADC pin for temperature (-1 = disabled)
    uint16_t ntcResistanceOhms;  // NTC resistance at nominal temperature (default: 10000)
    uint16_t ntcPullupResistanceOhms; // Fixed pull-up resistor value (default: 10000)
};
```

Temperature conversion uses an internal typical model:
- `NTC_NOMINAL_TEMP_C = 25 C`
- `NTC_BETA_TYPICAL = 3950`

This keeps the API simple when only `R25` is known, which is common for battery packs.

## API Reference

### Constructor
```cpp
BatteryCharger(const BatteryCharger::Config &config);
```

### Core Methods
- `bool begin()` - Initialize GPIO configuration
- `float voltage()` - Read battery voltage (volts)
- `float percent()` - Estimate state of charge (0-100%)
- `bool charging()` - Check if currently charging
- `bool done()` - Check if charging is complete
- `bool hasChargeCurrent() const` - Check if IBat measurement is configured
- `float chargeCurrentmA()` - Read charge current from ISET pin (CN306X/CN316X)
- `bool hasBatteryTemp() const` - Check if battery temperature measurement is configured
- `float batteryTempC()` - Read battery temperature from the NTC divider

### Shutdown Control
- `bool shutdown()` - Get current shutdown state
- `void setShutdown(bool state)` - Enable/disable measurements

### Status LED (Optional)
- `bool enableStatusLed(bool enable)` - Enable/disable LED task
- `bool isSatusLedEnabled()` - Check LED status
- `void setStatusLedBrightness(uint8_t brightness)` - Set LED brightness (0-255)
- `uint8_t statusLedBrightness()` - Get current LED brightness

**LED Behavior:**
- Red: Charging
- Green: Charging complete
- Off: Idle

### Internal Filtering
- `float readFilteredMilliVolts(uint8_t pin, AdcFilterState &state, uint8_t sampleCount = 16, float emaAlpha = 0.1f)` - Protected method for EMA-filtered ADC reads

## ISET Current Reference (CN306X/CN316X)

Formulas used by the library:

- CN306X family (`IBAT_FACTOR_CN306X = 900`): `ICH(A) = (VISET / RISET) * 900`
- CN316X family (`IBAT_FACTOR_CN316X = 986`): `ICH(A) = (VISET / RISET) * 986`

At constant-current regulation (`VISET` nominal):

| Charger | VISET (CC mode) | Factor | ICH max | RISET for ICH max |
|---|---:|---:|---:|---:|
| CN3063 | 2.0 V | 900 | 0.6 A | 3.0 kOhm |
| CN3065 | 2.0 V | 900 | 1.0 A | 1.8 kOhm |
| CN3163 | 1.205 V | 986 | 1.0 A | 1.19 kOhm (use 1.2 kOhm) |
| CN3165 | 1.205 V | 986 | 1.0 A | 1.19 kOhm (use 1.2 kOhm) |

Use 1% tolerance resistors when targeting an accurate charge current.

## Example: Full Configuration

```cpp
#include <BatteryCharger.h>

// All optional features enabled
BatteryCharger::Config config(
    2,       // vBatPin
    19,      // donePin
    20,      // chrgPin
    1.997f,  // vBatScalingFactor
    18,      // shutdownPin
    true,    // shutdown active low
    RGB_BUILTIN,  // rgbLedPin
    1,       // iBatPin
    2000,    // iSetResistanceOhms
    BatteryCharger::Config::IBAT_FACTOR_CN306X,
    0,       // ntcPin
    10000,   // ntcResistanceOhms at 25 C
    10000    // ntcPullupResistanceOhms
);

BatteryCharger charger(config);

void setup() {
    charger.begin();
    charger.enableStatusLed(true);
    charger.setStatusLedBrightness(64);
}

void loop() {
    float v = charger.voltage();
    float soc = charger.percent();
    float iBat = charger.chargeCurrentmA();
    float tBat = charger.batteryTempC();

    Serial.printf("V: %.2f V, SOC: %.0f%%, ICH: %.0f mA, TBAT: %.1f C\n", v, soc, iBat, tBat);
    delay(500);
}
```

## Future Extensions

Version 1.0.0 focuses on voltage, charge status, ISET current and NTC temperature. Upcoming features:
- Temperature-based charge rate control
- EEPROM calibration storage

## License

BSD-3-Clause. See LICENSE file for details.

## Support & Issues

For bug reports and feature requests, visit the [GitHub Issues](https://github.com/epsilonrt/BatteryCharger/issues) page.
