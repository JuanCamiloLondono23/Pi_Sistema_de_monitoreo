#ifndef __COMP_SPO2_H__
#define __COMP_SPO2_H__

#include <Arduino.h>

class SpO2Calculator {
public:
    // Constructor: se puede ajustar la frecuencia de muestreo
    SpO2Calculator(uint16_t sampleRate = 25);

    // Inicializa o reinicia los estados internos
    void begin();

    // Agrega una nueva muestra de IR y Rojo
    void addSample(uint32_t irSample, uint32_t redSample);

    // Calcula el valor de SpO2 y ritmo cardíaco
    bool compute(int32_t &spo2, int32_t &heartRate);

    // Obtiene el último valor de SpO2 calculado
    int32_t getLastSpO2() const;

    // Obtiene el último ritmo cardíaco calculado
    int32_t getLastHeartRate() const;

    // Devuelve true si los resultados actuales son válidos
    bool isValid() const;

private:
    static const uint16_t BUFFER_SIZE = 100; // Tamaño del buffer de muestras

    uint32_t _irBuffer[BUFFER_SIZE];
    uint32_t _redBuffer[BUFFER_SIZE];
    uint16_t _bufferIndex;
    uint16_t _sampleRate;

    int32_t _lastSpO2;
    int32_t _lastHeartRate;
    bool _validResult;

    // Funciones internas auxiliares
    void reset();
    void findPeaks(int32_t *locs, int32_t &numPeaks, int32_t *data, int32_t size, int32_t minHeight, int32_t minDistance);
    void sortAscending(int32_t *data, int32_t size);
};

#endif // __COMP_SPO2_H__
