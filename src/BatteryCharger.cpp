#include <BatteryCharger.h>
#include <math.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

#if defined(__has_include)
#if __has_include(<esp32-hal-rgb-led.h>)
#include <esp32-hal-rgb-led.h>
#define BATTERY_CHARGER_HAS_NATIVE_RGBLED 1
#else
#define BATTERY_CHARGER_HAS_NATIVE_RGBLED 0
#endif
#else
#define BATTERY_CHARGER_HAS_NATIVE_RGBLED 0
#endif

namespace {
	constexpr uint8_t VBatAdcSamples = 16;
	constexpr float VBatEmaAlpha = 0.1f;
	constexpr uint8_t IBatAdcSamples = 16;
	constexpr float IBatEmaAlpha = 0.1f;
	constexpr uint8_t NTCAdcSamples = 16;
	constexpr float NTCEmaAlpha = 0.1f;
	constexpr float MinBatteryVoltage = 3.0f;
	constexpr float MaxBatteryVoltage = 4.2f;
	constexpr float KelvinOffset = 273.15f;

	#if BATTERY_CHARGER_HAS_NATIVE_RGBLED
	inline uint8_t scaleColor(uint8_t value, uint8_t brightness) {
		return static_cast<uint8_t>((static_cast<uint16_t>(value) * brightness) / 255u);
	}
	#endif
}

// Validate configuration at compile time
static_assert(sizeof(BatteryCharger::Config) > 0, "BatteryCharger::Config must be defined");

// ----------------------------------------------------------------------------
BatteryCharger::BatteryCharger(const Config &config)
	: m_config(config),
	  m_shutdown(false),
	  m_lastVoltage(0.0f),
	  m_hasLastVoltage(false),
	  m_statusLedAllowed(config.rgbLedPin >= 0),
	  m_statusLedEnabled(false),
	  m_statusLedTask(nullptr),
	  m_statusLedBrightness(32) {
}

// ----------------------------------------------------------------------------
bool
BatteryCharger::begin() {

	// Validate required pins
	if (m_config.vBatPin < 0 || m_config.donePin < 0 || m_config.chrgPin < 0) {
		return false;
	}

	pinMode(m_config.donePin, INPUT_PULLUP);
	pinMode(m_config.chrgPin, INPUT_PULLUP);
	pinMode(m_config.vBatPin, INPUT);
	if (m_config.iBatPin >= 0) {
		pinMode(m_config.iBatPin, INPUT);
	}
	if (m_config.ntcPin >= 0) {
		pinMode(m_config.ntcPin, INPUT);
	}

	// Configure shutdown pin if enabled
	if (m_config.shutdownPin >= 0) {
		pinMode(m_config.shutdownPin, OUTPUT);
		setShutdown(false);
	}
	else {
		m_shutdown = false;
	}

	// Configure ADC attenuation explicitly for high input range measurements.
	#if defined(ARDUINO_ARCH_ESP32)
	#if defined(ADC_11db)
	analogSetPinAttenuation(m_config.vBatPin, ADC_11db);
	if (m_config.iBatPin >= 0) {
		analogSetPinAttenuation(m_config.iBatPin, ADC_11db);
	}
	if (m_config.ntcPin >= 0) {
		analogSetPinAttenuation(m_config.ntcPin, ADC_11db);
	}
	#elif defined(ADC_ATTEN_DB_12)
	analogSetPinAttenuation(m_config.vBatPin, ADC_ATTEN_DB_12);
	if (m_config.iBatPin >= 0) {
		analogSetPinAttenuation(m_config.iBatPin, ADC_ATTEN_DB_12);
	}
	if (m_config.ntcPin >= 0) {
		analogSetPinAttenuation(m_config.ntcPin, ADC_ATTEN_DB_12);
	}
	#endif
	#endif

	enableStatusLed(m_config.rgbLedPin >= 0);
	return true;
}

// ----------------------------------------------------------------------------
bool
BatteryCharger::shutdown() const {

	return m_shutdown;
}

// ----------------------------------------------------------------------------
void
BatteryCharger::setShutdown(bool state) {

	m_shutdown = state;
	if (m_config.shutdownPin >= 0) {
		// shutdownActiveLow=true : LOW active (circuit off) / HIGH inactive (circuit on)
		// shutdownActiveLow=false: HIGH active (circuit off) / LOW inactive (circuit on)
		digitalWrite(m_config.shutdownPin, m_config.shutdownActiveLow ? !m_shutdown : m_shutdown);
	}
	setStatusLedTaskActive(!m_shutdown);
}

// ----------------------------------------------------------------------------
bool
BatteryCharger::enableStatusLed(bool enable) {

	if (!enable) {
		m_statusLedEnabled = false;
		setStatusLedTaskActive(false);
		return true;
	}

	#if !defined(ARDUINO_ARCH_ESP32) || (BATTERY_CHARGER_HAS_NATIVE_RGBLED == 0)
	return false;
	#else
	if (m_config.rgbLedPin < 0) {
		return false;
	}

	if (m_statusLedTask == nullptr) {
		rgbLedWrite(static_cast<uint8_t>(m_config.rgbLedPin), 0, 0, 0);

		TaskHandle_t taskHandle = nullptr;
		if (xTaskCreate(BatteryCharger::statusLedTaskEntry, "BatteryCharger_led", 2048,
						this, 1, &taskHandle) != pdPASS) {
			return false;
		}
		m_statusLedTask = static_cast<void *>(taskHandle);
	}

	m_statusLedEnabled = true;
	setStatusLedTaskActive(!m_shutdown);
	return true;
	#endif
}

// ----------------------------------------------------------------------------
bool
BatteryCharger::isSatusLedEnabled() const {

	return m_statusLedEnabled;
}

// ----------------------------------------------------------------------------
void
BatteryCharger::setStatusLedBrightness(uint8_t brightness) {

	m_statusLedBrightness = brightness;
	#if BATTERY_CHARGER_HAS_NATIVE_RGBLED
	if (m_statusLedTask != nullptr) {
		updateStatusLed();
	}
	#endif
}

// ----------------------------------------------------------------------------
uint8_t
BatteryCharger::statusLedBrightness() const {

	return m_statusLedBrightness;
}

// ----------------------------------------------------------------------------
bool
BatteryCharger::done() const {

	if (m_config.donePin < 0) {
		return false;
	}
	return digitalRead(m_config.donePin) == LOW;
}

// ----------------------------------------------------------------------------
bool
BatteryCharger::charging() const {

	if (m_config.chrgPin < 0) {
		return false;
	}
	return digitalRead(m_config.chrgPin) == LOW;
}

// ----------------------------------------------------------------------------
void
BatteryCharger::updateStatusLed() {

	#if BATTERY_CHARGER_HAS_NATIVE_RGBLED
	if (m_config.rgbLedPin < 0) {
		return;
	}

	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;

	if (charging()) {
		r = scaleColor(255, m_statusLedBrightness);
	}
	else if (done()) {
		g = scaleColor(255, m_statusLedBrightness);
	}

	rgbLedWrite(static_cast<uint8_t>(m_config.rgbLedPin), r, g, b);
	#endif
}

// ----------------------------------------------------------------------------
void
BatteryCharger::setStatusLedTaskActive(bool active) {

	#if defined(ARDUINO_ARCH_ESP32) && (BATTERY_CHARGER_HAS_NATIVE_RGBLED == 1)
	if (m_statusLedTask == nullptr) {
		return;
	}

	TaskHandle_t taskHandle = static_cast<TaskHandle_t>(m_statusLedTask);
	if (!m_statusLedEnabled || !active) {
		vTaskSuspend(taskHandle);
		rgbLedWrite(static_cast<uint8_t>(m_config.rgbLedPin), 0, 0, 0);
		return;
	}
	vTaskResume(taskHandle);
	#endif
}

// ----------------------------------------------------------------------------
void
BatteryCharger::statusLedTaskEntry(void *arg) {

	#if defined(ARDUINO_ARCH_ESP32)
	BatteryCharger *self = static_cast<BatteryCharger *>(arg);
	for (;;) {
		self->updateStatusLed();
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	#else
	(void) arg;
	#endif
}

// ----------------------------------------------------------------------------
float
BatteryCharger::readFilteredMilliVolts(uint8_t pin, AdcFilterState &state,
									   uint8_t sampleCount,
									   float emaAlpha) {

	if (sampleCount == 0) {
		sampleCount = 1;
	}

	if (emaAlpha <= 0.0f) {
		emaAlpha = 0.1f;
	}
	else if (emaAlpha > 1.0f) {
		emaAlpha = 1.0f;
	}

	uint32_t adcMv = 0;
	for (uint8_t i = 0; i < sampleCount; ++i) {

		// analogReadMilliVolts uses ESP32 ADC calibration to return mV.
		adcMv += analogReadMilliVolts(pin);
	}

	const float averageMv = static_cast<float>(adcMv) /
							static_cast<float>(sampleCount);

	if (!state.initialized) {
		state.emaMv = averageMv;
		state.initialized = true;
	}
	else {
		state.emaMv = (emaAlpha * averageMv) + ((1.0f - emaAlpha) * state.emaMv);
	}

	return state.emaMv;
}

// ----------------------------------------------------------------------------
float
BatteryCharger::voltage() {

	if (m_config.vBatPin < 0) {
		return 0.0f;
	}

	if (m_shutdown && (m_config.shutdownPin >= 0)) {
		m_hasLastVoltage = false;
		return NAN;
	}

	const bool wasShutdownEnabled = shutdown();

	const float filteredMv = readFilteredMilliVolts(m_config.vBatPin, m_vbatFilterState,
													VBatAdcSamples, VBatEmaAlpha);
	(void)wasShutdownEnabled;

	m_lastVoltage = (filteredMv * m_config.vBatScalingFactor) / 1000.0f;
	m_hasLastVoltage = true;
	return m_lastVoltage;
}

// ----------------------------------------------------------------------------
float
BatteryCharger::percent() {
	if (m_shutdown && (m_config.shutdownPin >= 0)) {
		return NAN;
	}

	const float vbat = m_hasLastVoltage ? m_lastVoltage : voltage();
	if (isnan(vbat)) {
		return NAN;
	}

	float value = (vbat - MinBatteryVoltage) * 100.0f /
				  (MaxBatteryVoltage - MinBatteryVoltage);

	if (value < 0.0f) {
		value = 0.0f;
	}
	else if (value > 100.0f) {
		value = 100.0f;
	}
	return value;
}

// ----------------------------------------------------------------------------
bool
BatteryCharger::hasChargeCurrent() const {

	return (m_config.iBatPin >= 0) &&
			   (m_config.iSetResistanceOhms > 0) &&
			   (m_config.iBatCurrentFactor > 0.0f);
}

// ----------------------------------------------------------------------------
float
BatteryCharger::chargeCurrentmA() {

	if (!hasChargeCurrent()) {
		return NAN;
	}

	if (m_shutdown && (m_config.shutdownPin >= 0)) {
		return NAN;
	}

	const float vIsetMv = readFilteredMilliVolts(static_cast<uint8_t>(m_config.iBatPin),
											   m_ibatFilterState,
											   IBatAdcSamples,
											   IBatEmaAlpha);

	// Datasheet model (CN306x/CN316x families):
	// Ibat(mA) = (VISET(mV) / RISET(ohms)) * factor
	const float currentmA = (vIsetMv / static_cast<float>(m_config.iSetResistanceOhms)) *
							m_config.iBatCurrentFactor;

	if (currentmA < 0.0f) {
		return 0.0f;
	}
	return currentmA;
}

// ----------------------------------------------------------------------------
bool
BatteryCharger::hasBatteryTemp() const {

	return (m_config.ntcPin >= 0) &&
			   (m_config.ntcResistanceOhms > 0) &&
			   (m_config.ntcPullupResistanceOhms > 0);
}

// ----------------------------------------------------------------------------
float
BatteryCharger::batteryTempC() {

	if (!hasBatteryTemp()) {
		return NAN;
	}

	if (m_shutdown && (m_config.shutdownPin >= 0)) {
		return NAN;
	}

	const float vBat = m_hasLastVoltage ? m_lastVoltage : voltage();
	if (isnan(vBat) || (vBat <= 0.0f)) {
		return NAN;
	}

	const float vNtcMv = readFilteredMilliVolts(static_cast<uint8_t>(m_config.ntcPin),
										  m_ntcFilterState,
										  NTCAdcSamples,
										  NTCEmaAlpha);
	const float vBatMv = vBat * 1000.0f;

	if ((vNtcMv <= 0.0f) || (vNtcMv >= vBatMv)) {
		return NAN;
	}

	// Divider model: Vbat -> Rpullup -> node -> NTC -> GND
	const float ntcResistance = static_cast<float>(m_config.ntcPullupResistanceOhms) *
								(vNtcMv / (vBatMv - vNtcMv));
	if (ntcResistance <= 0.0f) {
		return NAN;
	}

	const float nominalTempK = Config::NTC_NOMINAL_TEMP_C + KelvinOffset;
	const float invTempK = (1.0f / nominalTempK) +
						  (logf(ntcResistance / static_cast<float>(m_config.ntcResistanceOhms)) /
						   Config::NTC_BETA_TYPICAL);
	if (invTempK <= 0.0f) {
		return NAN;
	}

	return (1.0f / invTempK) - KelvinOffset;
}
