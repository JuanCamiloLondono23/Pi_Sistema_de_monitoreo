#include <Arduino.h>
#include "LIB_MAX30102.h"
#include "COMP_RITMO_CARDIACO.h"
#include "COMP_SPO2.h"

// Instancias de tus clases
MAX30102        sensor;
RitmoCardiaco   cardio;
SpO2Calculator  spo2;

unsigned long lastPrintTime = 0;
const unsigned long printInterval = 1000; // Cada segundo

void setup() {
  Serial.begin(115200);
  delay(500);

  if (!sensor.begin()) {
    Serial.print("Error al iniciar MAX30102: ");
    Serial.println(sensor.getErrorStr());
    while (true) delay(1000);
  }

  sensor.softReset();
  sensor.setLEDMode(0x03);
  sensor.setADCRange(0x03);
  sensor.setSampleRate(0x00);
  sensor.setPulseWidth(0x02);
  sensor.setPulseAmplitudeRed(0x1F);
  sensor.setPulseAmplitudeIR(0x1F);
  sensor.clearFIFO();

  cardio.begin();
  spo2.begin();

  Serial.println("=== Mediciones HR & SpO2 ===");
}

void loop() {
  uint32_t redRaw, irRaw;

  // Leer datos crudos
  if (sensor.readFIFO(redRaw, irRaw)) {
    // Detectar latido y calcular BPM
    bool beatDetected = cardio.processSample(irRaw);

    // Guardar muestras para SpO2
    spo2.addSample(irRaw, redRaw);
  } else {
    Serial.println("Error leyendo sensor");
  }

  // Imprimir cada 1 segundo
  if (millis() - lastPrintTime >= printInterval) {
    lastPrintTime = millis();

    // Mostrar HR
    if (cardio.isValidBPM()) {
      Serial.print("HR: ");
      Serial.print(cardio.getBPM());
      Serial.print(" bpm\t");
    } else {
      Serial.print("HR: -- bpm\t");
    }

    // Mostrar SpO2
    int32_t spo2Val, hrDummy;
    if (spo2.compute(spo2Val, hrDummy)) {
      Serial.print("SpO2: ");
      Serial.print(spo2Val);
      Serial.println(" %");
    } else {
      Serial.println("SpO2: -- %");
    }
  }

  delay(40);
}
