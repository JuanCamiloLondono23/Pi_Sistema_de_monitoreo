#include <Arduino.h>
#include "LIB_SHT31.h"
#include "LIB_MAX30102.h"
#include "COMP_RITMO_CARDIACO.h"
#include "COMP_SPO2.h"
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <TimeLib.h>
#include <Wire.h>

// --- Configuración de umbrales ---
constexpr float TEMP_ALERT_THRESHOLD     = 37.5f;  // °C
constexpr float HR_ALERT_HIGH_THRESHOLD  = 120.0f; // BPM
constexpr float HR_ALERT_LOW_THRESHOLD   =  50.0f; // BPM

// --- Intervalos y temporizadores ---
constexpr uint32_t READING_INTERVAL_MS   = 60000; // Periodo de análisis (1 minuto)
static uint32_t lastReadingTimestamp = 0;

// --- SHT31 ---
SHT31 sht31;

// --- MAX30102 ---
MAX30102 maxSensor;
HeartRateProcessor hrProcessor;
SpO2Processor spo2Processor;
float dcIR = 0.0f;
float dcRed = 0.0f;
bool fingerPresent = false;
uint32_t lastSerialPrint = 0;
static float lastValidBPM = 0.0f;

// --- GPS (NEO6MV2) ---
static const int RXPin = 16;
static const int TXPin = 17;
static const uint32_t GPSBaud = 9600;
const int UTC_OFFSET_SECONDS = -5 * 3600;
TinyGPSPlus gps;
HardwareSerial GPS_Serial(2);

// --- Constantes MAX30102 internos ---
constexpr uint32_t FINGER_TH_ON  = 30000;
constexpr uint32_t FINGER_TH_OFF = 20000;
constexpr float DC_ALPHA        = 0.95f;
constexpr uint32_t SERIAL_UPDATE_INTERVAL = 1000;
constexpr uint8_t LINE_CLEAR_WIDTH = 40;

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  // Inicializa bus I2C y sensores
  Wire.begin();
  if (!sht31.begin()) {
    Serial.println("Error al iniciar SHT31: " + String(sht31.getErrorMessage()));
    while (true) delay(1000);
  }
  Serial.println("SHT31 iniciado correctamente.");

  if (!maxSensor.begin()) {
    Serial.println("Error: MAX30102 no encontrado.");
    while (true) delay(100);
  }
  maxSensor.setup();
  hrProcessor.reset();
  spo2Processor.reset();
  lastSerialPrint = millis();
  Serial.println("MAX30102 iniciado correctamente.");

  GPS_Serial.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);
  Serial.println("GPS iniciado correctamente.");
}

void loop() {
  uint32_t now = millis();
  // Ejecutar lógica de monitoreo en intervalos definidos
  if (now - lastReadingTimestamp < READING_INTERVAL_MS) {
    // Actualizar lectura continua del GPS y sensor de pulso
    readGPS();
    processMAX30102();
    return;
  }
  lastReadingTimestamp = now;

  // 1) Lectura SHT31
  float temperature, humidity;
  bool okTemp = sht31.read(temperature, humidity);

  // 2) Lectura frecuencia cardíaca y SpO2
  processMAX30102();
  float currentBPM = lastValidBPM;

  // 3) Actualizar GPS antes de enviar resultado
  readGPS();

  // 4) Evaluar condiciones de alerta
  bool alertTemp = okTemp && (temperature >= TEMP_ALERT_THRESHOLD);
  bool alertHR   = (currentBPM >= HR_ALERT_HIGH_THRESHOLD) ||
                   (currentBPM > 0 && currentBPM <= HR_ALERT_LOW_THRESHOLD);

  // 5) Obtener timestamp formateado
  char bufferTime[20];
  sprintf(bufferTime, "%02d/%02d/%04d %02d:%02d:%02d", day(), month(), year(), hour(), minute(), second());

  // 6) Mensaje de salida
  if (alertTemp || alertHR) {
    Serial.println("*** ALERTA DE SALUD ***");
    if (alertTemp) Serial.printf("Temperatura alta: %.2f °C\n", temperature);
    if (alertHR)   Serial.printf("Frecuencia cardiaca anómala: %.1f BPM\n", currentBPM);
  } else {
    Serial.println("Estado estable.");
    if (okTemp) Serial.printf("Temp: %.2f °C, ", temperature);
    else        Serial.print("Temp: N/A, ");
    Serial.printf("BPM: %.1f, SpO2: %u%%\n", currentBPM, spo2Processor.getSpO2());
  }
  // 7) Datos adicionales: hora y ubicación
  Serial.printf("Timestamp: %s\n", bufferTime);
  Serial.printf("Ubicación: Lat %.6f, Lon %.6f\n", gps.location.lat(), gps.location.lng());
  Serial.println("-------------------------------");
}

void processMAX30102() {
  std::vector<std::pair<uint32_t,uint32_t>> samples;
  if (!maxSensor.readAllFIFO(samples)) return;
  uint32_t t = millis();
  for (auto &p : samples) {
    uint32_t rawRed = p.first;
    uint32_t rawIR  = p.second;
    // Detección de dedo
    if (!fingerPresent && rawIR > FINGER_TH_ON) {
      fingerPresent = true;
      hrProcessor.reset(); spo2Processor.reset();
    } else if (fingerPresent && rawIR < FINGER_TH_OFF) {
      fingerPresent = false;
      hrProcessor.reset(); spo2Processor.reset();
      return;
    }
    if (!fingerPresent) return;
    // Eliminación DC y procesamiento
    dcIR  = DC_ALPHA*dcIR  + (1.0f-DC_ALPHA)*rawIR;
    dcRed = DC_ALPHA*dcRed + (1.0f-DC_ALPHA)*rawRed;
    float acIR  = float(rawIR)  - dcIR;
    float acRed = float(rawRed) - dcRed;
    bool beat = hrProcessor.update(acIR, t);
    spo2Processor.update(acIR, acRed, beat);
    float rawBPM = hrProcessor.getBPM();
    if (rawBPM >= 40.0f && rawBPM <= 180.0f) lastValidBPM = rawBPM;
  }
}

void readGPS() {
  while (GPS_Serial.available() > 0) gps.encode(GPS_Serial.read());
  if (gps.date.isValid() && gps.time.isValid()) {
    setTime(gps.time.hour(), gps.time.minute(), gps.time.second(),
            gps.date.day(), gps.date.month(), gps.date.year());
    adjustTime(UTC_OFFSET_SECONDS);
  }
}
