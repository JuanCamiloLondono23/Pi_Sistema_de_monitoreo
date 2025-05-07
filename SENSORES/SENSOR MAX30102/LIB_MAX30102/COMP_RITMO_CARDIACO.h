#ifndef __COMP_RITMO_CARDIACO_H__
#define __COMP_RITMO_CARDIACO_H__

#include <Arduino.h>

class RitmoCardiaco {
public:
    // Constructor: permite configurar el tamaño de la ventana y la frecuencia de muestreo (Hz)
    RitmoCardiaco(uint16_t windowSize = 32, uint16_t sampleRate = 25);

     // Destructor: libera la memoria que hayas reservado
     ~RitmoCardiaco();

    // Inicia o reinicia todos los contadores internos
    void begin();

    // Procesa una muestra nueva del sensor IR
    // Devuelve true si detecta un latido en esta muestra
    bool processSample(int32_t irSample);

    // Obtiene el ritmo cardíaco calculado en BPM
    int16_t getBPM() const;

    // Devuelve true si el BPM calculado es válido
    bool isValidBPM() const;

    // Resetea todos los estados internos
    void reset();

private:
    uint16_t _windowSize;       // Número de muestras en la ventana de cálculo
    uint16_t _sampleRate;       // Frecuencia de muestreo en Hz
    int32_t *_irBuffer;         // Buffer para las muestras de IR
    uint16_t _bufferIndex;      // Índice actual dentro del buffer

    // Variables de estado para detección de latidos
    int32_t _irMean;            // Estimación del nivel DC
    int16_t _previousAC;        // Valor AC previo
    int16_t _currentAC;         // Valor AC actual
    uint16_t _beatCount;        // Número de latidos detectados

    uint32_t _lastBeatTime;     // Tiempo (ms) del último latido detectado
    int16_t _bpm;               // Último BPM calculado
    bool _validBPM;             // Bandera si el BPM actual es válido

    // Métodos privados auxiliares
    int16_t estimateDC(int32_t sample);
    int16_t lowPassFilter(int16_t value);
};

#endif // __COMP_RITMO_CARDIACO_H__
