#include "AS7343.h"
#include <math.h>

namespace {

constexpr uint8_t ADDRESS = 0x59;

// Registers
constexpr uint8_t REG_ENABLE   = 0x80;
constexpr uint8_t REG_ATIME    = 0x81;
constexpr uint8_t REG_STATUS2  = 0x90;
constexpr uint8_t REG_ASTATUS  = 0x94;
constexpr uint8_t REG_CFG1     = 0xC6;
constexpr uint8_t REG_LED      = 0xCD;
constexpr uint8_t REG_ASTEP_L  = 0xD4;
constexpr uint8_t REG_ASTEP_H  = 0xD5;
constexpr uint8_t REG_CFG20    = 0xD6;
constexpr uint8_t REG_DATA_0_L = 0x95;
constexpr uint8_t REG_FD_STATUS = 0xE3;
constexpr uint8_t REG_CFG0 = 0xBF;
constexpr uint8_t REG_FD_TIME_1 = 0xE0;
constexpr uint8_t REG_FD_TIME_2 = 0xE2;
constexpr uint8_t REG_FIFO_CFG0 = 0xDF;
constexpr uint8_t REG_FIFO_LVL = 0xFD;
constexpr uint8_t REG_FDATA_L = 0xFE;
constexpr uint8_t REG_STATUS4 = 0xBC;
constexpr uint8_t REG_CONTROL = 0xFA;

// Identification
constexpr uint8_t REG_AUXID = 0x58;
constexpr uint8_t REG_REVID = 0x59;
constexpr uint8_t REG_ID    = 0x5A;

// Enable
constexpr uint8_t ENABLE_PON   = 0x01;
constexpr uint8_t ENABLE_SP_EN = 0x02;
constexpr uint8_t ENABLE_FDEN  = 0x40;
constexpr uint8_t REGISTER_BANK1 = 0x10;
constexpr uint8_t FIFO_8BIT_FLICKER = 0x80;
constexpr uint8_t FIFO_WRITE_FLICKER = 0x80;
constexpr uint8_t FIFO_CLEAR = 0x02;
constexpr uint8_t FIFO_OVERFLOW = 0x80;
constexpr uint8_t STATUS2_AVALID = 0x40;
constexpr uint8_t FD_100HZ_DET = 0x01;
constexpr uint8_t FD_120HZ_DET = 0x02;
constexpr uint8_t FD_100HZ_VALID = 0x04;
constexpr uint8_t FD_120HZ_VALID = 0x08;
constexpr uint8_t FD_SAT = 0x10;
constexpr uint8_t FD_VALID = 0x20;

// SMUX
constexpr uint8_t CFG20_SMUX_18CH = 0x60;

// LED
constexpr uint8_t LED_ACT  = 0x80;
constexpr uint8_t LED_12MA = 0x04;

inline uint8_t applyMask(uint8_t value, uint8_t mask, bool enabled) {
    if (enabled) {
        return static_cast<uint8_t>(value | mask);
    }
    return static_cast<uint8_t>(value & static_cast<uint8_t>(~mask));
}

} // namespace

AS7343::AS7343()
    : I2CDevice(ADDRESS),
      _f1(0.0f), _f2(0.0f), _f3(0.0f), _f4(0.0f), _f5(0.0f), _f6(0.0f), _f7(0.0f), _f8(0.0f),
      _fz(0.0f), _fy(0.0f), _fxl(0.0f), _nir(0.0f), _vis(0) {}

bool AS7343::begin(TwoWire& wire) {
    I2CDevice::begin(wire);

    uint8_t aux = 0;
    uint8_t rev = 0;
    uint8_t id = 0;
    if (!selectRegisterBank(true)) {
        return false;
    }
    const bool idsRead = readBytes(REG_AUXID, &aux, 1) &&
                         readBytes(REG_REVID, &rev, 1) &&
                         readBytes(REG_ID, &id, 1);
    const bool bankRestored = selectRegisterBank(false);

    return idsRead && bankRestored &&
           aux != 0xFF && rev != 0xFF && id != 0xFF;
}

bool AS7343::begin(int8_t sdaPin, int8_t sclPin, TwoWire& wire) {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_ESP8266)
    wire.begin(sdaPin, sclPin);
#else
    (void)sdaPin;
    (void)sclPin;
    wire.begin();
#endif

    return begin(wire);
}

bool AS7343::sensorPresent() {
    if (_wire == nullptr) {
        return false;
    }

    _wire->beginTransmission(_address);
    return _wire->endTransmission() == 0;
}

bool AS7343::read() {
    uint16_t raw[18];
    if (!sensorPresent()) {
        return false;
    }

    if (!startMeasurement() || !waitForData(1000) || !readRaw18(raw)) {
        return false;
    }

    _fz = static_cast<float>(raw[0]);
    _fy = static_cast<float>(raw[1]);
    _fxl = static_cast<float>(raw[2]);
    _nir = static_cast<float>(raw[3]);
    _f2 = static_cast<float>(raw[6]);
    _f3 = static_cast<float>(raw[7]);
    _f4 = static_cast<float>(raw[8]);
    _f6 = static_cast<float>(raw[9]);
    _f1 = static_cast<float>(raw[12]);
    _f7 = static_cast<float>(raw[13]);
    _f8 = static_cast<float>(raw[14]);
    _f5 = static_cast<float>(raw[15]);
    const uint32_t visSum = static_cast<uint32_t>(raw[4]) + raw[5] +
                            raw[10] + raw[11] + raw[16] + raw[17];
    _vis = static_cast<uint16_t>(visSum / 6);
    return true;
}

bool AS7343::startMeasurement() {
    if (!sensorPresent()) {
        return false;
    }

    if (!selectRegisterBank(true)) {
        return false;
    }

    uint8_t enable = 0;
    if (!readBytes(REG_ENABLE, &enable, 1)) {
        selectRegisterBank(false);
        return false;
    }
    if ((enable & ENABLE_PON) == 0) {
        enable |= ENABLE_PON;
        if (!write8(REG_ENABLE, enable)) {
            selectRegisterBank(false);
            return false;
        }
        delay(10);
    }

    enable |= ENABLE_SP_EN;
    const bool enabled = write8(REG_ENABLE, enable);
    const bool bankRestored = selectRegisterBank(false);
    return enabled && bankRestored;
}

bool AS7343::waitForData(uint32_t timeoutMs) {
    const uint32_t startTime = millis();
    do {
        if ((read8(REG_STATUS2) & STATUS2_AVALID) != 0) {
            return true;
        }
        delay(1);
    } while (static_cast<uint32_t>(millis() - startTime) < timeoutMs);

    return false;
}

void AS7343::configureAutoSMUX() {
    uint8_t cfg20 = read8(REG_CFG20);
    cfg20 = static_cast<uint8_t>((cfg20 & static_cast<uint8_t>(~0x60)) | CFG20_SMUX_18CH);
    write8(REG_CFG20, cfg20);

    Serial.print("CFG20 = 0x");
    Serial.println(read8(REG_CFG20), HEX);
    Serial.println("Auto-SMUX = 18 channel mode");
}

void AS7343::setGain(Gain gain) {
    write8(REG_CFG1, static_cast<uint8_t>(gain));
    Serial.print("Gain = ");
    switch (gain) {
        case Gain::X0_5:
            Serial.println("0.5x");
            break;
        case Gain::X1:
            Serial.println("1x");
            break;
        case Gain::X2:
            Serial.println("2x");
            break;
        case Gain::X4:
            Serial.println("4x");
            break;
        case Gain::X8:
            Serial.println("8x");
            break;
        case Gain::X16:
            Serial.println("16x");
            break;
        case Gain::X32:
            Serial.println("32x");
            break;
        case Gain::X64:
            Serial.println("64x");
            break;
        case Gain::X128:
            Serial.println("128x");
            break;
        case Gain::X256:
            Serial.println("256x");
            break;
        case Gain::X512:
            Serial.println("512x");
            break;
        case Gain::X1024:
            Serial.println("1024x");
            break;
        case Gain::X2048:
            Serial.println("2048x");
            break;
    }
}

void AS7343::setIntegration() {
    write8(REG_ATIME, 29);
    write8(REG_ASTEP_L, static_cast<uint8_t>(599 & 0xFF));
    write8(REG_ASTEP_H, static_cast<uint8_t>((599 >> 8) & 0xFF));

    Serial.println("ATIME = 29");
    Serial.println("ASTEP = 599");
}

void AS7343::powerOn() {
    if (!selectRegisterBank(true)) {
        return;
    }

    uint8_t enable = 0;
    if (!readBytes(REG_ENABLE, &enable, 1)) {
        selectRegisterBank(false);
        return;
    }
    enable = applyMask(enable, ENABLE_PON, true);
    write8(REG_ENABLE, enable);
    selectRegisterBank(false);
    delay(10);
}

void AS7343::powerOff() {
    uint8_t enable = read8(REG_ENABLE);
    enable = applyMask(enable, ENABLE_PON | ENABLE_SP_EN, false);
    write8(REG_ENABLE, enable);
}

void AS7343::ledOn() {
    uint8_t value = read8(REG_LED);
    if (value == 0xFF) {
        value = LED_12MA;
    }

    value = applyMask(value, LED_ACT, true);
    write8(REG_LED, value);
}

void AS7343::ledOff() {
    uint8_t value = read8(REG_LED);
    if (value == 0xFF) {
        return;
    }

    value = applyMask(value, LED_ACT, false);
    write8(REG_LED, value);
}

bool AS7343::enableFlickerDetection(bool enable) {
    if (!selectRegisterBank(true)) {
        return false;
    }

    uint8_t value = 0;
    if (!readBytes(REG_ENABLE, &value, 1)) {
        selectRegisterBank(false);
        return false;
    }

    value = applyMask(value, ENABLE_FDEN, enable);
    const bool writeSucceeded = write8(REG_ENABLE, value);
    const bool bankRestored = selectRegisterBank(false);
    return writeSucceeded && bankRestored;
}

bool AS7343::flickerDetectionValid() {
    uint8_t status = 0;
    return readFlickerStatus(status) && (status & FD_VALID) != 0;
}

bool AS7343::flickerDetectionSaturated() {
    uint8_t status = 0;
    return readFlickerStatus(status) && (status & FD_SAT) != 0;
}

bool AS7343::getFlickerInfo(bool& detected, uint16_t& frequencyHz) {
    detected = false;
    frequencyHz = 0;

    uint8_t status = 0;
    if (!readFlickerStatus(status)) {
        return false;
    }

    if ((status & FD_VALID) == 0 || (status & FD_SAT) != 0) {
        return true;
    }

    if ((status & (FD_100HZ_DET | FD_100HZ_VALID)) ==
        (FD_100HZ_DET | FD_100HZ_VALID)) {
        detected = true;
        frequencyHz = 100;
    } else if ((status & (FD_120HZ_DET | FD_120HZ_VALID)) ==
               (FD_120HZ_DET | FD_120HZ_VALID)) {
        detected = true;
        frequencyHz = 120;
    }
    return true;
}

bool AS7343::readFlickerStatus(uint8_t& status) {
    return selectRegisterBank(false) && readBytes(REG_FD_STATUS, &status, 1);
}

bool AS7343::configureFlickerFifo(uint16_t integrationTicks) {
    if (integrationTicks == 0 || integrationTicks >= 256) {
        return false;
    }

    if (!selectRegisterBank(true)) {
        return false;
    }

    uint8_t enable = 0;
    if (!readBytes(REG_ENABLE, &enable, 1)) {
        selectRegisterBank(false);
        return false;
    }

    enable |= ENABLE_PON;
    enable &= static_cast<uint8_t>(~(ENABLE_FDEN | ENABLE_SP_EN));
    if (!write8(REG_ENABLE, enable)) {
        selectRegisterBank(false);
        return false;
    }
    const bool bankRestored = selectRegisterBank(false);
    if (!bankRestored) {
        return false;
    }
    delay(10);

    uint8_t fdTimeHighGain = 0;
    uint8_t cfg20 = 0;
    uint8_t fifoConfig = 0;
    if (!readBytes(REG_FD_TIME_2, &fdTimeHighGain, 1) ||
        !readBytes(REG_CFG20, &cfg20, 1) ||
        !readBytes(REG_FIFO_CFG0, &fifoConfig, 1)) {
        return false;
    }

    fdTimeHighGain = static_cast<uint8_t>(
        (fdTimeHighGain & 0xF8) | ((integrationTicks >> 8) & 0x07));
    cfg20 |= FIFO_8BIT_FLICKER;
    fifoConfig |= FIFO_WRITE_FLICKER;

    if (!write8(REG_FD_TIME_1, static_cast<uint8_t>(integrationTicks)) ||
        !write8(REG_FD_TIME_2, fdTimeHighGain) ||
        !write8(REG_CFG20, cfg20) ||
        !write8(REG_FIFO_CFG0, fifoConfig) ||
        !write8(REG_CONTROL, FIFO_CLEAR)) {
        return false;
    }

    if (!selectRegisterBank(true)) {
        return false;
    }
    if (!readBytes(REG_ENABLE, &enable, 1)) {
        selectRegisterBank(false);
        return false;
    }
    enable |= ENABLE_PON | ENABLE_FDEN | ENABLE_SP_EN;
    const bool enabled = write8(REG_ENABLE, enable);
    const bool restored = selectRegisterBank(false);
    return enabled && restored;
}

bool AS7343::readFlickerFifo(uint8_t* samples, uint16_t capacity,
                             uint16_t& sampleCount) {
    sampleCount = 0;
    if (samples == nullptr || capacity < 2 || !selectRegisterBank(false)) {
        return false;
    }

    uint8_t fifoLevel = 0;
    if (!readBytes(REG_FIFO_LVL, &fifoLevel, 1)) {
        return false;
    }

    const uint16_t entriesToRead =
        (capacity / 2 < fifoLevel) ? capacity / 2 : fifoLevel;
    for (uint16_t entry = 0; entry < entriesToRead; ++entry) {
        if (!readBytes(REG_FDATA_L, samples + sampleCount, 2)) {
            return false;
        }
        sampleCount += 2;
    }
    return true;
}

bool AS7343::flickerFifoOverflowed(bool& overflowed) {
    overflowed = false;
    if (!selectRegisterBank(false)) {
        return false;
    }
    uint8_t status = 0;
    if (!readBytes(REG_STATUS4, &status, 1)) {
        return false;
    }
    overflowed = (status & FIFO_OVERFLOW) != 0;
    return true;
}

bool AS7343::estimateFlickerFrequencyHz(const uint8_t* samples,
                                        uint16_t sampleCount,
                                        float sampleRateHz,
                                        float& frequencyHz) {
    frequencyHz = 0.0f;
    if (samples == nullptr || sampleCount < 4 ||
        !isfinite(sampleRateHz) || sampleRateHz <= 0.0f) {
        return false;
    }

    uint32_t sum = 0;
    for (uint16_t i = 0; i < sampleCount; ++i) {
        sum += samples[i];
    }
    const float mean = static_cast<float>(sum) / sampleCount;

    uint16_t risingCrossings = 0;
    float firstCrossing = 0.0f;
    float lastCrossing = 0.0f;
    float previous = static_cast<float>(samples[0]) - mean;

    for (uint16_t i = 1; i < sampleCount; ++i) {
        const float current = static_cast<float>(samples[i]) - mean;
        if (previous <= 0.0f && current > 0.0f) {
            const float fraction = -previous / (current - previous);
            const float crossing = static_cast<float>(i - 1) + fraction;
            if (risingCrossings == 0) {
                firstCrossing = crossing;
            }
            lastCrossing = crossing;
            ++risingCrossings;
        }
        previous = current;
    }

    if (risingCrossings < 3 || lastCrossing <= firstCrossing) {
        return false;
    }

    frequencyHz = (static_cast<float>(risingCrossings - 1) * sampleRateHz) /
                  (lastCrossing - firstCrossing);
    return true;
}

bool AS7343::selectRegisterBank(bool bank1) {
    uint8_t cfg0 = 0;
    if (!readBytes(REG_CFG0, &cfg0, 1)) {
        return false;
    }
    cfg0 = applyMask(cfg0, REGISTER_BANK1, bank1);
    return write8(REG_CFG0, cfg0);
}

bool AS7343::readRaw18(uint16_t raw[18]) {
    if (raw == nullptr) {
        return false;
    }

    if (read8(REG_ASTATUS) == 0xFF) {
        return false;
    }

    uint16_t values[18];
    for (int i = 0; i < 18; ++i) {
        if (!read16(static_cast<uint8_t>(REG_DATA_0_L + (i * 2)), values[i])) {
            return false;
        }
    }

    for (int i = 0; i < 18; ++i) {
        raw[i] = values[i];
    }
    return true;
}

bool AS7343::read16(uint8_t reg, uint16_t& value) {
    if (_wire == nullptr) {
        return false;
    }

    _wire->beginTransmission(_address);
    _wire->write(reg);

    if (_wire->endTransmission(false) != 0) {
        return false;
    }

    if (_wire->requestFrom(_address, static_cast<uint8_t>(2)) != 2) {
        return false;
    }

    const uint8_t lo = _wire->read();
    const uint8_t hi = _wire->read();

    value = (static_cast<uint16_t>(hi) << 8) | lo;
    return true;
}

float AS7343::blue_FZ() const { return _fz; }
float AS7343::greenYellow_F5() const { return _f5; }
float AS7343::yellowGreen_FY() const { return _fy; }
float AS7343::orange_FXL() const { return _fxl; }
float AS7343::violetBlue_F2() const { return _f2; }
float AS7343::blueCyan_F3() const { return _f3; }
float AS7343::green_F4() const { return _f4; }
float AS7343::red_F6() const { return _f6; }
float AS7343::violet_F1() const { return _f1; }
float AS7343::deepRed_F7() const { return _f7; }
float AS7343::redNearInfrared_F8() const { return _f8; }
float AS7343::nearInfrared_NIR() const { return _nir; }
uint16_t AS7343::getVIS() const { return _vis; }
