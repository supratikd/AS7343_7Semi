#ifndef AS7343_7Semi_H
#define AS7343_7Semi_H

#include <Arduino.h>
#include <Wire.h>
#include "sensor/AS7343.h"


class AS7343_7Semi {
public:
	using Gain = AS7343::Gain;

	AS7343_7Semi();

	bool begin(TwoWire& wire = Wire);
	bool begin(int8_t sdaPin, int8_t sclPin, TwoWire& wire = Wire);
	bool read();
	bool sensorPresent();

	float blue_FZ() const;
	float greenYellow_F5() const;
	float yellowGreen_FY() const;
	float orange_FXL() const;
	float violetBlue_F2() const;
	float blueCyan_F3() const;
	float green_F4() const;
	float red_F6() const;
	float violet_F1() const;
	float deepRed_F7() const;
	float redNearInfrared_F8() const;
	float nearInfrared_NIR() const;
	uint16_t getVIS() const;
	// void powerOn();
	// void powerOff();
	void ledOn();
	void ledOff();
	void setGain(Gain gain);
	bool enableFlickerDetection(bool enable = true);
	bool flickerDetectionValid();
	bool flickerDetectionSaturated();
	bool getFlickerInfo(bool& detected, uint16_t& frequencyHz);
	bool configureFlickerFifo(uint16_t integrationTicks = 200);
	bool readFlickerFifo(uint8_t* samples, uint16_t capacity, uint16_t& sampleCount);
	bool flickerFifoOverflowed(bool& overflowed);
	static bool estimateFlickerFrequencyHz(const uint8_t* samples,
	                                       uint16_t sampleCount,
	                                       float sampleRateHz,
	                                       float& frequencyHz);
	void configureAutoSMUX();
	void setIntegration();
	void powerOn();
private:
	bool initSensors(TwoWire& wire);
	AS7343 spectralSensor;
	bool _initialized;
};
#endif