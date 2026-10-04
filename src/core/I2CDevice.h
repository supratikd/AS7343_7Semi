#ifndef HW290_CORE_I2CDEVICE_H
#define HW290_CORE_I2CDEVICE_H

#include <Arduino.h>
#include <Wire.h>

class I2CDevice {
protected:
    TwoWire* _wire;
    uint8_t _address;

public:
    explicit I2CDevice(uint8_t address);

    bool begin(TwoWire& wire);

    bool write8(uint8_t reg, uint8_t value);
    uint8_t read8(uint8_t reg);
    uint16_t read16(uint8_t reg);
    bool readBytes(uint8_t reg, uint8_t* buffer, uint8_t len);
};

#endif
