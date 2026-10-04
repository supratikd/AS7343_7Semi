#ifndef AS7343_H
#define AS7343_H

#include <Arduino.h>
#include <Wire.h>
#include "../core/I2CDevice.h"

class AS7343 : public I2CDevice {
public:
	AS7343();
	
	 enum class Gain : uint8_t {
        X0_5   = 0,
        X1     = 1,
        X2     = 2,
        X4     = 3,
        X8     = 4,
        X16    = 5,
        X32    = 6,
        X64    = 7,
        X128   = 8,
        X256   = 9,
        X512   = 10,
        X1024  = 11,
        X2048  = 12
    };


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

	void configureAutoSMUX();
	void setGain(Gain gain);
	void setIntegration();
	void powerOn();
	void powerOff();
	void ledOn();
	void ledOff();
	bool enableFlickerDetection(bool enable = true);
	bool flickerDetectionValid();
	bool flickerDetectionSaturated();
	uint16_t flickerFrequencyHz();

private:
	bool startMeasurement();
	bool waitForData(uint32_t timeoutMs);
	bool readRaw18(uint16_t raw[18]);
	bool read16(uint8_t reg, uint16_t& value);  

	float _f1;
	float _f2;
	float _f3;
	float _f4;
	float _f5;
	float _f6;
	float _f7;
	float _f8;
	float _fz;
	float _fy;
	float _fxl;
	float _nir;
	uint16_t _vis;
};  
#endif