#include <Arduino.h>
#include "LIB_MAX30102.h"
#include "COMP_RITMO_CARDIACO.h"
#include "COMP_SPO2.h"

// Threshold for detecting finger presence (adjust as needed)
constexpr uint32_t FINGER_ON_THRESHOLD = 50000;

// How often to update Serial output (ms)
constexpr uint32_t SERIAL_UPDATE_INTERVAL = 1000;

MAX30102 sensor;
HeartRateProcessor hrProcessor;
SpO2Processor spo2Processor;

// Simple DC removal (EMA) constants
constexpr float DC_ALPHA = 0.95f;
float dcIR = 0, dcRed = 0;

uint32_t lastSerialPrint = 0;
bool fingerPresent = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!sensor.begin()) {
    Serial.println(F("MAX30102 not found. Check wiring."));
    while (1) delay(100);
  }
  sensor.setup();

  hrProcessor.reset();
  spo2Processor.reset();
  lastSerialPrint = millis();
}

void loop() {
  // Read all available samples from FIFO
  std::vector<std::pair<uint32_t,uint32_t>> samples;
  if (!sensor.readAllFIFO(samples)) {
    // No new data
    delay(10);
    return;
  }

  uint32_t now = millis();

  for (auto &p : samples) {
    uint32_t rawRed = p.first;
    uint32_t rawIR  = p.second;

    // Finger presence detection
    bool nowPresent = rawIR > FINGER_ON_THRESHOLD;

    if (!nowPresent) {
      // Finger removed: reset and notify once
      if (fingerPresent) {
        Serial.println(F("\n-- Remove finger to pause measurement --"));
        hrProcessor.reset();
        spo2Processor.reset();
      }
      fingerPresent = false;
      continue;
    }

    // First time finger placed
    if (!fingerPresent) {
      Serial.println(F("\n-- Finger detected, starting measurements --"));
      lastSerialPrint = now;
    }
    fingerPresent = true;

    // DC removal (EMA)
    dcIR  = DC_ALPHA * dcIR  + (1.0f - DC_ALPHA) * rawIR;
    dcRed = DC_ALPHA * dcRed + (1.0f - DC_ALPHA) * rawRed;
    float acIR  = float(rawIR)  - dcIR;
    float acRed = float(rawRed) - dcRed;

    // Heart-rate detection
    bool beat = hrProcessor.update(acIR, now);

    // SpO2 processing
    spo2Processor.update(acIR, acRed, beat);
  }

  // Dynamic serial update once per interval
  if (fingerPresent && (now - lastSerialPrint >= SERIAL_UPDATE_INTERVAL)) {
    float bpm = hrProcessor.getBPM();
    uint8_t spo2 = spo2Processor.getSpO2();

    // Overwrite previous line
    Serial.print("\rBPM: ");
    if (bpm > 0.0f) {
      Serial.print(bpm, 1);
    } else {
      Serial.print("--");
    }
    Serial.print("   SpO2: ");
    if (spo2 > 0) {
      Serial.print(spo2);
      Serial.print("%");
    } else {
      Serial.print("--");
    }
    lastSerialPrint = now;
  }
}