#ifndef __LIB_MAX30102_H__
#define __LIB_MAX30102_H__

#include <Arduino.h>
#include <Wire.h>

// Controlador del sensor MAX30102
class MAX30102 {
public:
    // Códigos de error
    enum ErrorCode {
        OK,
        NOT_CONNECTED,
        I2C_ERROR,
        INVALID_PARAM
    };

    // Crear instancia con bus I2C y dirección (0x57)
    MAX30102(TwoWire &wire = Wire, uint8_t address = 0x57);

    // Iniciar I2C (velocidad por defecto 400 kHz) y comprobar sensor
    bool begin(uint32_t i2cSpeed = 400000UL);

    // Reset suave por registro MODE_CONFIG
    bool softReset();

    // Apagar y despertar sensor (bit SHDN)
    bool shutdown();
    bool wakeUp();

    // Configuración general
    bool setLEDMode(uint8_t mode);
    bool setADCRange(uint8_t range);
    bool setSampleRate(uint8_t rate);
    bool setPulseWidth(uint8_t width);

    // Ajuste de amplitud de LEDs
    bool setPulseAmplitudeRed(uint8_t value);
    bool setPulseAmplitudeIR(uint8_t value);
    bool setPulseAmplitudeGreen(uint8_t value);
    bool setPulseAmplitudeProximity(uint8_t value);

    // Umbral de proximidad
    bool setProximityThreshold(uint8_t threshold);

    // Configuración de FIFO
    bool setFIFOAverage(uint8_t samples);
    bool enableFIFORollover(bool on = true);
    bool setFIFOAlmostFull(uint8_t level);

    // Limpia punteros FIFO
    bool clearFIFO();

    // Lee punteros FIFO
    uint8_t getWritePointer();
    uint8_t getReadPointer();

    // Lectura de datos crudos (6 bytes: red+IR)
    bool readFIFO(uint32_t &red, uint32_t &ir);

    // Lectura de temperatura interna en °C
    bool readTemperature(float &temp);

    // Leer registros de estado (INT_STATUS1 y INT_STATUS2)
    bool readInterruptStatus(uint8_t &stat1, uint8_t &stat2);

    // Identificación del chip y revisión
    uint8_t getPartID();
    uint8_t getRevisionID();

    // Último error
    ErrorCode getError() const;
    const char* getErrorStr() const;

private:
    TwoWire &_wire;
    uint8_t  _addr;
    ErrorCode _error;
    uint8_t  _revId;

    // Direcciones de registro según datasheet
    static constexpr uint8_t REG_INTR_STATUS1    = 0x00;
    static constexpr uint8_t REG_INTR_STATUS2    = 0x01;
    static constexpr uint8_t REG_INTR_ENABLE1    = 0x02;
    static constexpr uint8_t REG_INTR_ENABLE2    = 0x03;
    static constexpr uint8_t REG_FIFO_WR_PTR     = 0x04;
    static constexpr uint8_t REG_OVF_COUNTER     = 0x05;
    static constexpr uint8_t REG_FIFO_RD_PTR     = 0x06;
    static constexpr uint8_t REG_FIFO_DATA       = 0x07;
    static constexpr uint8_t REG_FIFO_CONFIG     = 0x08;
    static constexpr uint8_t REG_MODE_CONFIG     = 0x09;
    static constexpr uint8_t REG_SPO2_CONFIG     = 0x0A;
    static constexpr uint8_t REG_LED1_PA         = 0x0C;
    static constexpr uint8_t REG_LED2_PA         = 0x0D;
    static constexpr uint8_t REG_LED3_PA         = 0x0E;
    static constexpr uint8_t REG_PROX_INT_THRESH = 0x30;
    static constexpr uint8_t REG_MULTI_LED1      = 0x11;
    static constexpr uint8_t REG_MULTI_LED2      = 0x12;
    static constexpr uint8_t REG_TEMP_INT        = 0x1F;
    static constexpr uint8_t REG_TEMP_FRAC       = 0x20;
    static constexpr uint8_t REG_TEMP_CONFIG     = 0x21;
    static constexpr uint8_t REG_PART_ID         = 0xFF;
    static constexpr uint8_t REG_REV_ID          = 0xFE;

    // Acceso I2C de bajo nivel
    bool writeRegister(uint8_t reg, uint8_t val);
    bool readRegister(uint8_t reg, uint8_t &val);
    bool burstRead(uint8_t reg, uint8_t *buf, uint8_t len);

    // Modificar solo ciertos bits sin afectar el resto
    bool bitMask(uint8_t reg, uint8_t mask, uint8_t value);

    // Registrar error interno
    void setError(ErrorCode e) { _error = e; }
};

#endif // __LIB_MAX30102_H__
