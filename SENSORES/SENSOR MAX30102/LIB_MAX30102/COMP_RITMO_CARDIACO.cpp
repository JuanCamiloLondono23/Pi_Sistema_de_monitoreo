#include "COMP_RITMO_CARDIACO.h"

RitmoCardiaco::RitmoCardiaco(uint16_t windowSize, uint16_t sampleRate)
    : _windowSize(windowSize), _sampleRate(sampleRate), _irBuffer(nullptr), _bufferIndex(0),
      _irMean(0), _previousAC(0), _currentAC(0), _beatCount(0), _lastBeatTime(0),
      _bpm(0), _validBPM(false)
{
    _irBuffer = new int32_t[_windowSize]; // Reserva memoria para el buffer
}

void RitmoCardiaco::begin() {
    _bufferIndex = 0;
    _beatCount = 0;
    _irMean = 0;
    _previousAC = 0;
    _currentAC = 0;
    _bpm = 0;
    _validBPM = false;
    _lastBeatTime = millis();
    for (uint16_t i = 0; i < _windowSize; i++) {
        _irBuffer[i] = 0;
    }
}

bool RitmoCardiaco::processSample(int32_t irSample) {
    if (!_irBuffer) return false; // Verifica que haya memoria

    _irBuffer[_bufferIndex] = irSample;
    _bufferIndex = (_bufferIndex + 1) % _windowSize;

    // Estimar el valor DC
    _irMean = estimateDC(irSample);

    // Calcular componente AC
    _previousAC = _currentAC;
    _currentAC = lowPassFilter(irSample - _irMean);

    bool detected = false;

    // Detectar cruce por cero en pendiente positiva (subida de pulso)
    if (_previousAC < 0 && _currentAC >= 0) {
        uint32_t currentTime = millis();
        uint32_t interval = currentTime - _lastBeatTime;

        if (interval > 300 && interval < 2000) { // latidos normales entre 30 y 200 bpm
            _bpm = (60000 / interval);
            _validBPM = true;
            _lastBeatTime = currentTime;
            detected = true;
        } else {
            _validBPM = false;
        }
    }

    return detected;
}

int16_t RitmoCardiaco::getBPM() const {
    return _bpm;
}

bool RitmoCardiaco::isValidBPM() const {
    return _validBPM;
}

void RitmoCardiaco::reset() {
    if (_irBuffer) {
        for (uint16_t i = 0; i < _windowSize; i++) {
            _irBuffer[i] = 0;
        }
    }
    _bufferIndex = 0;
    _beatCount = 0;
    _irMean = 0;
    _previousAC = 0;
    _currentAC = 0;
    _bpm = 0;
    _validBPM = false;
    _lastBeatTime = millis();
}

int16_t RitmoCardiaco::estimateDC(int32_t sample) {
    static int64_t dcEstimate = 0;
    dcEstimate += ((int64_t)sample - dcEstimate) >> 5; // filtro pasa bajos IIR
    return (int16_t)(dcEstimate >> 0); // retornar parte entera
}

int16_t RitmoCardiaco::lowPassFilter(int16_t value) {
    static int32_t acc = 0;
    acc += value - (acc >> 4); // filtro pasa bajos simple
    return (int16_t)(acc >> 0);
}

// Destructor para liberar memoria
RitmoCardiaco::~RitmoCardiaco() {
    if (_irBuffer) {
        delete[] _irBuffer;
        _irBuffer = nullptr;
    }
}
