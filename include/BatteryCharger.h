#pragma once

#include <Arduino.h>

/**
   @brief Handles Li-Ion charger status and battery voltage acquisition.

   This class provides:
   - Battery voltage measurement with EMA-filtered ADC reads
   - Charge status monitoring (DONE, CHG pins)
   - Optional battery temperature measurement (NTC thermistor)
   - Optional charge current estimation (ISET pin)
   - Optional WS2812 RGB status LED control (FreeRTOS task on ESP32)

   Configuration is passed via a BatteryCharger::Config struct, making this class
   independent of platform-specific headers and portable across microcontrollers.
*/
class BatteryCharger {
  public:
    /**
       @brief Configuration structure for BatteryCharger.

       All pin numbers should be valid GPIO/ADC pins for your platform.
       Use -1 to disable optional features.

       NTC temperature conversion uses a typical internal model based on:
       - NTC_NOMINAL_TEMP_C (default 25 C)
       - NTC_BETA_TYPICAL (default 3950)

       This keeps user configuration simple: only R25 and pull-up resistor are required.
    */
    struct Config {
      static constexpr float IBAT_FACTOR_CN306X = 900.0f;
      static constexpr float IBAT_FACTOR_CN316X = 986.0f;
      static constexpr float NTC_NOMINAL_TEMP_C = 25.0f;
      static constexpr float NTC_BETA_TYPICAL = 3950.0f;

      int8_t vBatPin;
      int8_t donePin;
      int8_t chrgPin;
      float vBatScalingFactor;
      int8_t shutdownPin;
      bool shutdownActiveLow;
      int8_t rgbLedPin;
      int8_t iBatPin;
      uint16_t iSetResistanceOhms;
      float iBatCurrentFactor;
      int8_t ntcPin;
      uint16_t ntcResistanceOhms;
      uint16_t ntcPullupResistanceOhms;

      Config (int8_t vbat,
              int8_t done,
              int8_t chrg,
              float scaling = 2.0f,
              int8_t shutdown = -1,
              bool shutdownLow = true,
              int8_t rgbLed = -1,
              int8_t iBat = -1,
              uint16_t isetOhms = 2000,
              float iBatFactor = IBAT_FACTOR_CN306X,
              int8_t ntc = -1,
              uint16_t ntcOhms = 10000,
              uint16_t ntcPullupOhms = 10000)
        : vBatPin (vbat),
          donePin (done),
          chrgPin (chrg),
          vBatScalingFactor (scaling),
          shutdownPin (shutdown),
          shutdownActiveLow (shutdownLow),
          rgbLedPin (rgbLed),
          iBatPin (iBat),
          iSetResistanceOhms (isetOhms),
          iBatCurrentFactor (iBatFactor),
          ntcPin (ntc),
          ntcResistanceOhms (ntcOhms),
          ntcPullupResistanceOhms (ntcPullupOhms) {
      }
    };

    /**
       @brief Persistent state for one EMA-filtered ADC input.
    */
    struct AdcFilterState {
      float emaMv;
      bool initialized;

      AdcFilterState() : emaMv (0.0f), initialized (false) {
      }
    };

    /**
       @brief Creates a battery charger instance with configuration.
       @param config Configuration struct with pin assignments and parameters.
    */
    BatteryCharger (const Config &config);

    /**
       @brief Configures GPIOs used by the charger and battery sensing circuit.
       @return True when initialization succeeds.
    */
    bool begin();

    /**
       @brief Gets the current shutdown control state.
       @return True when measurement circuitry is disabled.
    */
    bool shutdown() const;

    /**
       @brief Enables or disables the battery measurement circuitry.
       @param state Set true to disable measurements (low power), false to enable measurements.
    */
    void setShutdown (bool state);

    /**
       @brief Enables or disables the periodic status LED update task.
       @param enable Set true to enable LED updates.
       @return True when request was applied (requires rgbLedPin configured and ESP32 native RGB support).
    */
    bool enableStatusLed (bool enable);

    /**
       @brief Indicates whether status LED task is enabled.
       @return True when LED updates are enabled.
    */
    bool isSatusLedEnabled() const;

    /**
       @brief Sets status LED brightness.
       @param brightness Brightness in range [0, 255].
    */
    void setStatusLedBrightness (uint8_t brightness);

    /**
       @brief Gets current status LED brightness.
       @return Brightness in range [0, 255].
    */
    uint8_t statusLedBrightness() const;

    /**
       @brief Indicates if battery charging is complete.
       @return True when DONE pin is active (logic low).
    */
    bool done() const;

    /**
       @brief Indicates if battery charging is in progress.
       @return True when CHRG pin is active (logic low).
    */
    bool charging() const;

    /**
       @brief Reads battery voltage in volts.
       @return Battery voltage in V.
    */
    float voltage();

    /**
       @brief Estimates battery state of charge.
       @return Battery percentage in range [0, 100].
    */
    float percent();

    /**
       @brief Indicates if charge current measurement is configured.
       @return True when iBatPin and ISET parameters are valid.
    */
    bool hasChargeCurrent() const;

    /**
       @brief Reads estimated charge current from ISET monitor pin.

       Uses the formula: Ibat(mA) = (VISET(mV) / RISET(ohms)) * factor.
       Typical factor values are 900 (CN306x) and 986 (CN316x).

       @return Charge current in mA, or NAN if unavailable.
    */
    float chargeCurrentmA();

    /**
       @brief Indicates if battery temperature measurement is configured.
       @return True when NTC parameters are valid.
    */
    bool hasBatteryTemp() const;

    /**
       @brief Reads battery temperature from an NTC divider.

       Assumes a divider made of a pull-up resistor tied to battery voltage and
       an NTC thermistor tied to ground. Conversion uses ntcResistanceOhms (R25)
       plus an internal typical Beta model constants from Config.

       @return Battery temperature in degree Celsius, or NAN if unavailable.
    */
    float batteryTempC();

  protected:
    /**
       @brief Reads an ADC pin and applies averaging + EMA filtering.
       @param pin ADC pin to sample.
       @param state Persistent filter state associated to this ADC channel.
       @param sampleCount Number of raw ADC samples used for averaging.
       @param emaAlpha EMA coefficient in range (0, 1]. Lower is smoother.
       @return Filtered voltage in millivolts.
    */
    float readFilteredMilliVolts (uint8_t pin, AdcFilterState &state,
                                  uint8_t sampleCount = 16,
                                  float emaAlpha = 0.1f);

  private:
    void updateStatusLed();
    void setStatusLedTaskActive (bool active);
    static void statusLedTaskEntry (void *arg);

    Config m_config;
    bool m_shutdown;
    AdcFilterState m_vbatFilterState;
    AdcFilterState m_ibatFilterState;
    AdcFilterState m_ntcFilterState;
    float m_lastVoltage;
    bool m_hasLastVoltage;
    bool m_statusLedAllowed;
    bool m_statusLedEnabled;
    void *m_statusLedTask;
    uint8_t m_statusLedBrightness;
};

