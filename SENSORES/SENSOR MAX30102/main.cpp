#include <Arduino.h>
#include "LIB_MAX30102.h"
#include "COMP_RITMO_CARDIACO.h"
#include "COMP_SPO2.h"

// Finger‐presence thresholds (with hysteresis)
constexpr uint32_t FINGER_TH_ON  = 30000;  // rawIR above → finger placed
constexpr uint32_t FINGER_TH_OFF = 20000;  // rawIR below → finger removed

// Serial update parameters
constexpr uint32_t SERIAL_UPDATE_INTERVAL = 1000;  // ms
constexpr uint8_t  LINE_CLEAR_WIDTH       = 40;    // chars

// DC removal constant
constexpr float DC_ALPHA = 0.95f;

MAX30102           sensor;
HeartRateProcessor hrProcessor;
SpO2Processor      spo2Processor;

uint32_t lastSerialPrint = 0;
bool     fingerPresent   = false;
float    dcIR            = 0.0f;
float    dcRed           = 0.0f;

// Last valid BPM for plausibility filtering
static float lastValidBPM = 0.0f;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!sensor.begin()) {
    Serial.println(F("ERROR: MAX30102 not found."));
    while (true) delay(100);
  }
  sensor.setup();

  hrProcessor.reset();
  spo2Processor.reset();
  lastSerialPrint = millis();
}

void loop() {
  // 1) Read all new samples (no delay if empty)
  std::vector<std::pair<uint32_t,uint32_t>> samples;
  if (!sensor.readAllFIFO(samples)) {
    return; // try again immediately
  }

  uint32_t now = millis();

  // 2) Process each sample
  for (auto &p : samples) {
    uint32_t rawRed = p.first;
    uint32_t rawIR  = p.second;

    // 2a) Finger‐presence with hysteresis
    if (!fingerPresent && rawIR > FINGER_TH_ON) {
      Serial.println(F("\n-- Finger placed, starting measurements --"));
      fingerPresent = true;
      lastSerialPrint = now;
    }
    else if (fingerPresent && rawIR < FINGER_TH_OFF) {
      Serial.println(F("\n-- Finger removed, pausing --"));
      fingerPresent = false;
      hrProcessor.reset();
      spo2Processor.reset();
      continue;  // skip further processing until finger returns
    }
    if (!fingerPresent) {
      continue;
    }

    // 2b) DC removal (EMA)
    dcIR  = DC_ALPHA * dcIR  + (1.0f - DC_ALPHA) * rawIR;
    dcRed = DC_ALPHA * dcRed + (1.0f - DC_ALPHA) * rawRed;
    float acIR  = float(rawIR)  - dcIR;
    float acRed = float(rawRed) - dcRed;

    // 2c) Beat detection
    bool beat = hrProcessor.update(acIR, now);

    // 2d) SpO2 calculation
    spo2Processor.update(acIR, acRed, beat);
  }

  // 3) Periodic Serial update
  if (fingerPresent && (now - lastSerialPrint >= SERIAL_UPDATE_INTERVAL)) {
    // 3a) Get raw BPM and apply plausibility filter (40–180 BPM)
    float rawBPM = hrProcessor.getBPM();
    if (rawBPM >= 40.0f && rawBPM <= 180.0f) {
      lastValidBPM = rawBPM;
    }
    float displayBPM = (lastValidBPM > 0.0f ? lastValidBPM : rawBPM);

    uint8_t spo2v = spo2Processor.getSpO2();

    // 3b) Clear previous line
    Serial.print('\r');
    for (uint8_t i = 0; i < LINE_CLEAR_WIDTH; ++i) Serial.print(' ');
    Serial.print('\r');

    // 3c) Print BPM and SpO2
    Serial.print(F("BPM: "));
    if (displayBPM > 0.0f) Serial.print(displayBPM, 1);
    else                   Serial.print(F("--"));

    Serial.print(F("   SpO2: "));
    if (spo2v > 0) {
      Serial.print(spo2v);
      Serial.print('%');
    } else {
      Serial.print(F("--"));
    }

    lastSerialPrint = now;
  }
}