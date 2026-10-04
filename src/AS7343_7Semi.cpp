#include "AS7343_7Semi.h"

AS7343_7Semi::AS7343_7Semi() : _initialized(false) {}

bool AS7343_7Semi::begin(TwoWire& wire) {
	wire.begin();

	return initSensors(wire);
}

bool AS7343_7Semi::begin(int8_t sdaPin, int8_t sclPin, TwoWire& wire) {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_ESP8266)
	wire.begin(sdaPin, sclPin);
#else
	(void)sdaPin;
	(void)sclPin;
	wire.begin();
#endif

	return initSensors(wire);
}

bool AS7343_7Semi::initSensors(TwoWire& wire) {
	_initialized = spectralSensor.begin(wire);
	return _initialized;
}

bool AS7343_7Semi::read() {
	return _initialized && spectralSensor.read();
}

bool AS7343_7Semi::sensorPresent() {
	return _initialized && spectralSensor.sensorPresent();
}

// void AS7343_7Semi::powerOn() {
// 	spectralSensor.powerOn();
// }

// void AS7343_7Semi::powerOff() {
// 	spectralSensor.powerOff();
// }

void AS7343_7Semi::ledOn() {
	spectralSensor.ledOn();
}

void AS7343_7Semi::ledOff() {
	spectralSensor.ledOff();
}

void AS7343_7Semi::setGain(Gain gain) {
	spectralSensor.setGain(gain);
}

bool AS7343_7Semi::enableFlickerDetection(bool enable) {
	return _initialized && spectralSensor.enableFlickerDetection(enable);
}

bool AS7343_7Semi::flickerDetectionValid() {
	return _initialized && spectralSensor.flickerDetectionValid();
}

bool AS7343_7Semi::flickerDetectionSaturated() {
	return _initialized && spectralSensor.flickerDetectionSaturated();
}

bool AS7343_7Semi::getFlickerInfo(bool& detected, uint16_t& frequencyHz) {
	detected = false;
	frequencyHz = 0;
	return _initialized && spectralSensor.getFlickerInfo(detected, frequencyHz);
}

bool AS7343_7Semi::configureFlickerFifo(uint16_t integrationTicks) {
	return _initialized && spectralSensor.configureFlickerFifo(integrationTicks);
}

bool AS7343_7Semi::readFlickerFifo(uint8_t* samples, uint16_t capacity,
                                   uint16_t& sampleCount) {
	sampleCount = 0;
	return _initialized &&
	       spectralSensor.readFlickerFifo(samples, capacity, sampleCount);
}

bool AS7343_7Semi::flickerFifoOverflowed(bool& overflowed) {
	overflowed = false;
	return _initialized && spectralSensor.flickerFifoOverflowed(overflowed);
}

bool AS7343_7Semi::estimateFlickerFrequencyHz(const uint8_t* samples,
                                             uint16_t sampleCount,
                                             float sampleRateHz,
                                             float& frequencyHz) {
	return AS7343::estimateFlickerFrequencyHz(samples, sampleCount,
	                                          sampleRateHz, frequencyHz);
}

void AS7343_7Semi::configureAutoSMUX() {
	if (_initialized) {
		spectralSensor.configureAutoSMUX();
	}
}

void AS7343_7Semi::setIntegration() {
	if (_initialized) {
		spectralSensor.setIntegration();
	}
}

void AS7343_7Semi::powerOn() {
	if (_initialized) {
		spectralSensor.powerOn();
	}
}

float AS7343_7Semi::blue_FZ() const { return spectralSensor.blue_FZ(); }
float AS7343_7Semi::greenYellow_F5() const { return spectralSensor.greenYellow_F5(); }
float AS7343_7Semi::yellowGreen_FY() const { return spectralSensor.yellowGreen_FY(); }
float AS7343_7Semi::orange_FXL() const { return spectralSensor.orange_FXL(); }
float AS7343_7Semi::violetBlue_F2() const { return spectralSensor.violetBlue_F2(); }
float AS7343_7Semi::blueCyan_F3() const { return spectralSensor.blueCyan_F3(); }
float AS7343_7Semi::green_F4() const { return spectralSensor.green_F4(); }
float AS7343_7Semi::red_F6() const { return spectralSensor.red_F6(); }
float AS7343_7Semi::violet_F1() const { return spectralSensor.violet_F1(); }
float AS7343_7Semi::deepRed_F7() const { return spectralSensor.deepRed_F7(); }
float AS7343_7Semi::redNearInfrared_F8() const { return spectralSensor.redNearInfrared_F8(); }
float AS7343_7Semi::nearInfrared_NIR() const { return spectralSensor.nearInfrared_NIR(); }
uint16_t AS7343_7Semi::getVIS() const { return spectralSensor.getVIS(); }