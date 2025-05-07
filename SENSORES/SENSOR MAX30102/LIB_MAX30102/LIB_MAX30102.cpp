#include "LIB_MAX30102.h"

// Tiempo de espera en ms para operaciones con polling
static constexpr uint32_t TIMEOUT_MS = 100;

// Máscaras de bits para registro MODE_CONFIG
static constexpr uint8_t MODE_SHDN_MASK   = 0x7F;
static constexpr uint8_t MODE_SHDN        = 0x80;
static constexpr uint8_t MODE_RESET_MASK  = 0xBF;
static constexpr uint8_t MODE_RESET       = 0x40;
static constexpr uint8_t MODE_MODE_MASK   = 0xF8;

// Máscaras de bits para registro SPO2_CONFIG
static constexpr uint8_t SPO2_RANGE_MASK  = 0x9F; // bits 6:5
static constexpr uint8_t SPO2_SR_MASK     = 0xE3; // bits 4:2
static constexpr uint8_t SPO2_PW_MASK     = 0xFC; // bits 1:0

// Máscaras de bits para registro FIFO_CONFIG
static constexpr uint8_t FIFO_AVG_MASK        = 0x1F; // ~0b11100000
static constexpr uint8_t FIFO_ROLLOVER_MASK   = 0xEF; // ~0b00010000
static constexpr uint8_t FIFO_A_FULL_MASK     = 0xF0; // ~0b00001111

MAX30102::MAX30102(TwoWire &wire, uint8_t address)
    : _wire(wire), _addr(address), _error(OK), _revId(0) { }

bool MAX30102::begin(uint32_t i2cSpeed) {
    _wire.begin();
    _wire.setClock(i2cSpeed);

    uint8_t id;
    if (!readRegister(REG_PART_ID, id)) {
        setError(I2C_ERROR);
        return false;
    }
    if (id != 0x15) {
        setError(NOT_CONNECTED);
        return false;
    }
    // Leer Revision ID (no crítico para funcionamiento)
    readRegister(REG_REV_ID, _revId);
    setError(OK);
    return true;
}

bool MAX30102::softReset() {
    if (!bitMask(REG_MODE_CONFIG, MODE_RESET_MASK, MODE_RESET)) {
        setError(I2C_ERROR);
        return false;
    }
    // Polling hasta que el bit RESET se limpie
    uint32_t start = millis();
    uint8_t m;
    do {
        if (!readRegister(REG_MODE_CONFIG, m)) {
            setError(I2C_ERROR);
            return false;
        }
        if (millis() - start > TIMEOUT_MS) {
            setError(I2C_ERROR);
            return false;
        }
    } while (m & MODE_RESET);
    return true;
}

bool MAX30102::shutdown() {
    if (!bitMask(REG_MODE_CONFIG, MODE_SHDN_MASK, MODE_SHDN)) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

bool MAX30102::wakeUp() {
    if (!bitMask(REG_MODE_CONFIG, MODE_SHDN_MASK, 0x00)) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

bool MAX30102::setLEDMode(uint8_t mode) {
    // mode: 0x02 = RED only, 0x03 = RED+IR, 0x07 = multi-LED
    if ((mode & ~0x07) != 0) {
        setError(INVALID_PARAM);
        return false;
    }
    if (!bitMask(REG_MODE_CONFIG, MODE_MODE_MASK, mode)) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

bool MAX30102::setADCRange(uint8_t range) {
    // range debe estar codificado en bits [6:5]
    if (!bitMask(REG_SPO2_CONFIG, SPO2_RANGE_MASK, range << 5)) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

bool MAX30102::setSampleRate(uint8_t rate) {
    // rate debe ir en bits [4:2]
    if (!bitMask(REG_SPO2_CONFIG, SPO2_SR_MASK, rate << 2)) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

bool MAX30102::setPulseWidth(uint8_t width) {
    // width en bits [1:0]
    if (!bitMask(REG_SPO2_CONFIG, SPO2_PW_MASK, width)) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

bool MAX30102::setPulseAmplitudeRed(uint8_t value) {
    if (!writeRegister(REG_LED1_PA, value)) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

bool MAX30102::setPulseAmplitudeIR(uint8_t value) {
    if (!writeRegister(REG_LED2_PA, value)) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

bool MAX30102::setPulseAmplitudeGreen(uint8_t value) {
    if (!writeRegister(REG_LED3_PA, value)) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

bool MAX30102::setPulseAmplitudeProximity(uint8_t value) {
    if (!writeRegister(REG_PROX_INT_THRESH, value)) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

bool MAX30102::setProximityThreshold(uint8_t threshold) {
    // Umbral de proximidad
    return setPulseAmplitudeProximity(threshold);
}

bool MAX30102::setFIFOAverage(uint8_t samples) {
    // samples en 0..7
    if (samples > 7) {
        setError(INVALID_PARAM);
        return false;
    }
    if (!bitMask(REG_FIFO_CONFIG, FIFO_AVG_MASK, samples << 5)) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

bool MAX30102::enableFIFORollover(bool on) {
    uint8_t val = on ? 0x10 : 0x00;
    if (!bitMask(REG_FIFO_CONFIG, FIFO_ROLLOVER_MASK, val)) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

bool MAX30102::setFIFOAlmostFull(uint8_t level) {
    // level en 0..15
    if (level > 15) {
        setError(INVALID_PARAM);
        return false;
    }
    if (!bitMask(REG_FIFO_CONFIG, FIFO_A_FULL_MASK, level)) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

bool MAX30102::clearFIFO() {
    bool ok = true;
    ok &= writeRegister(REG_FIFO_WR_PTR,  0);
    ok &= writeRegister(REG_OVF_COUNTER,  0);
    ok &= writeRegister(REG_FIFO_RD_PTR,  0);
    if (!ok) {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

uint8_t MAX30102::getWritePointer() {
    uint8_t ptr = 0;
    readRegister(REG_FIFO_WR_PTR, ptr);
    return ptr;
}

uint8_t MAX30102::getReadPointer() {
    uint8_t ptr = 0;
    readRegister(REG_FIFO_RD_PTR, ptr);
    return ptr;
}

bool MAX30102::readFIFO(uint32_t &red, uint32_t &ir) {
    uint8_t buf[6];
    if (!burstRead(REG_FIFO_DATA, buf, 6)) {
        setError(I2C_ERROR);
        return false;
    }
    red = (uint32_t(buf[0]) << 16) | (uint32_t(buf[1]) << 8) | buf[2];
    ir  = (uint32_t(buf[3]) << 16) | (uint32_t(buf[4]) << 8) | buf[5];
    // 18 bits válido
    red &= 0x3FFFF;
    ir  &= 0x3FFFF;
    setError(OK);
    return true;
}

bool MAX30102::readTemperature(float &temp) {
    // Iniciar conversión
    if (!writeRegister(REG_TEMP_CONFIG, 0x01)) {
        setError(I2C_ERROR);
        return false;
    }
    // Polling hasta que DIE_TEMP_RDY (bit1 en INT_STATUS2) se active
    uint32_t start = millis();
    uint8_t s2;
    do {
        if (!readRegister(REG_INTR_STATUS2, s2)) {
            setError(I2C_ERROR);
            return false;
        }
        if (millis() - start > TIMEOUT_MS) {
            setError(I2C_ERROR);
            return false;
        }
    } while (!(s2 & 0x02));
    // Leer parte entera y fracción
    uint8_t ti, tf;
    if (!readRegister(REG_TEMP_INT,  ti) ||
        !readRegister(REG_TEMP_FRAC, tf))
    {
        setError(I2C_ERROR);
        return false;
    }
    temp = float(int8_t(ti)) + (tf * 0.0625f);
    setError(OK);
    return true;
}

bool MAX30102::readInterruptStatus(uint8_t &stat1, uint8_t &stat2) {
    if (!readRegister(REG_INTR_STATUS1, stat1) ||
        !readRegister(REG_INTR_STATUS2, stat2))
    {
        setError(I2C_ERROR);
        return false;
    }
    return true;
}

uint8_t MAX30102::getPartID() {
    uint8_t id = 0;
    readRegister(REG_PART_ID, id);
    return id;
}

uint8_t MAX30102::getRevisionID() {
    if (!readRegister(REG_REV_ID, _revId)) {
        setError(I2C_ERROR);
    }
    return _revId;
}

MAX30102::ErrorCode MAX30102::getError() const {
    return _error;
}

const char* MAX30102::getErrorStr() const {
    switch (_error) {
        case OK:             return "OK";
        case NOT_CONNECTED:  return "No conectado";
        case I2C_ERROR:      return "Error I2C";
        case INVALID_PARAM:  return "Parametro invalido";
        default:             return "Error desconocido";
    }
}

// ——— MÓDULOS I2C DE BAJO NIVEL ———————————————————

bool MAX30102::writeRegister(uint8_t reg, uint8_t val) {
    _wire.beginTransmission(_addr);
    _wire.write(reg);
    _wire.write(val);
    if (_wire.endTransmission() != 0) {
        return false;
    }
    return true;
}

bool MAX30102::readRegister(uint8_t reg, uint8_t &val) {
    _wire.beginTransmission(_addr);
    _wire.write(reg);
    if (_wire.endTransmission(false) != 0) {
        return false;
    }
    if (_wire.requestFrom(_addr, (uint8_t)1) != 1) {
        return false;
    }
    val = _wire.read();
    return true;
}

bool MAX30102::burstRead(uint8_t reg, uint8_t *buf, uint8_t len) {
    _wire.beginTransmission(_addr);
    _wire.write(reg);
    if (_wire.endTransmission(false) != 0) {
        return false;
    }
    if (_wire.requestFrom(_addr, len) != len) {
        return false;
    }
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = _wire.read();
    }
    return true;
}

bool MAX30102::bitMask(uint8_t reg, uint8_t mask, uint8_t value) {
    uint8_t orig;
    if (!readRegister(reg, orig)) {
        return false;
    }
    orig = (orig & mask) | value;
    return writeRegister(reg, orig);
}
