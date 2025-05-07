#include "COMP_SPO2.h"

SpO2Calculator::SpO2Calculator(uint16_t sampleRate)
    : _bufferIndex(0), _sampleRate(sampleRate), _lastSpO2(-1), _lastHeartRate(-1), _validResult(false) {
}

void SpO2Calculator::begin() {
    reset();
}

void SpO2Calculator::reset() {
    _bufferIndex = 0;
    _lastSpO2 = -1;
    _lastHeartRate = -1;
    _validResult = false;
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        _irBuffer[i] = 0;
        _redBuffer[i] = 0;
    }
}

void SpO2Calculator::addSample(uint32_t irSample, uint32_t redSample) {
    _irBuffer[_bufferIndex] = irSample;
    _redBuffer[_bufferIndex] = redSample;
    _bufferIndex = (_bufferIndex + 1) % BUFFER_SIZE;
}

bool SpO2Calculator::compute(int32_t &spo2, int32_t &heartRate) {
    int32_t irMean = 0;
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        irMean += _irBuffer[i];
    }
    irMean /= BUFFER_SIZE;

    int32_t acIR[BUFFER_SIZE];
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        acIR[i] = (int32_t)_irBuffer[i] - irMean;
    }

    int32_t peaks[15] = {0};
    int32_t numPeaks = 0;
    findPeaks(peaks, numPeaks, acIR, BUFFER_SIZE, 5000, _sampleRate / 2);

    if (numPeaks >= 2) {
        int32_t intervalSum = 0;
        for (int32_t i = 1; i < numPeaks; i++) {
            intervalSum += (peaks[i] - peaks[i - 1]);
        }
        int32_t avgInterval = intervalSum / (numPeaks - 1);
        heartRate = (_sampleRate * 60) / avgInterval;
    } else {
        heartRate = -1;
    }

    int32_t dcIR = 0, dcRed = 0;
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        dcIR += _irBuffer[i];
        dcRed += _redBuffer[i];
    }
    dcIR /= BUFFER_SIZE;
    dcRed /= BUFFER_SIZE;

    int32_t acIRsqSum = 0, acRedSqSum = 0;
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        acIRsqSum += (_irBuffer[i] - dcIR) * (_irBuffer[i] - dcIR);
        acRedSqSum += (_redBuffer[i] - dcRed) * (_redBuffer[i] - dcRed);
    }

    if (acIRsqSum == 0) {
        spo2 = -1;
        _validResult = false;
        return false;
    }

    float ratio = (float)acRedSqSum / acIRsqSum;
    spo2 = 110 - (25 * ratio);
    if (spo2 > 100) spo2 = 100;
    if (spo2 < 0) spo2 = 0;

    _lastSpO2 = spo2;
    _lastHeartRate = heartRate;
    _validResult = (spo2 > 0 && heartRate > 0);

    return _validResult;
}

int32_t SpO2Calculator::getLastSpO2() const {
    return _lastSpO2;
}

int32_t SpO2Calculator::getLastHeartRate() const {
    return _lastHeartRate;
}

bool SpO2Calculator::isValid() const {
    return _validResult;
}

void SpO2Calculator::findPeaks(int32_t *locs, int32_t &numPeaks, int32_t *data, int32_t size, int32_t minHeight, int32_t minDistance) {
    numPeaks = 0;
    for (int32_t i = 1; i < size - 1; i++) {
        if (data[i] > minHeight && data[i] > data[i - 1] && data[i] > data[i + 1]) {
            if (numPeaks == 0 || (i - locs[numPeaks - 1]) > minDistance) {
                locs[numPeaks++] = i;
                if (numPeaks >= 15) break;
            }
        }
    }
}

void SpO2Calculator::sortAscending(int32_t *data, int32_t size) {
    for (int32_t i = 1; i < size; i++) {
        int32_t temp = data[i];
        int32_t j = i - 1;
        while (j >= 0 && data[j] > temp) {
            data[j + 1] = data[j];
            j--;
        }
        data[j + 1] = temp;
    }
}
