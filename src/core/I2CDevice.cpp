#include "I2CDevice.h"

I2CDevice::I2CDevice(uint8_t address) : _wire(nullptr), _address(address) {}

bool I2CDevice::begin(TwoWire& wire) {
    _wire = &wire;
    return true;
}

bool I2CDevice::write8(uint8_t reg, uint8_t value) {
    if (_wire == nullptr) {
        return false;
    }

    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(value);
    return _wire->endTransmission() == 0;
}

uint8_t I2CDevice::read8(uint8_t reg) {
    uint8_t value = 0;
    readBytes(reg, &value, 1);
    return value;
}

uint16_t I2CDevice::read16(uint8_t reg) {
    uint8_t data[2] = {0, 0};
    if (!readBytes(reg, data, 2)) {
        return 0;
    }
    return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | data[1]);
}

bool I2CDevice::readBytes(uint8_t reg, uint8_t* buffer, uint8_t len) {
    if (_wire == nullptr || buffer == nullptr || len == 0) {
        return false;
    }

    _wire->beginTransmission(_address);
    _wire->write(reg);
    if (_wire->endTransmission(false) != 0) {
        return false;
    }

    uint8_t readCount = _wire->requestFrom(_address, len);
    if (readCount != len) {
        return false;
    }

    for (uint8_t i = 0; i < len; ++i) {
        buffer[i] = static_cast<uint8_t>(_wire->read());
    }
    return true;
}
