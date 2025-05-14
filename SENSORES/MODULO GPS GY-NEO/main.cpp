#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// --- Configuración de pines UART2 ---
static const int RXPin = 16;  // RX2: recibe datos (TX del GPS)
static const int TXPin = 17;  // TX2: solo por convención (no se conecta al GPS)
static const uint32_t GPSBaud = 9600;  // Velocidad estándar del GY-NEO6MV2

// --- Instancia de GPS y Serial hardware ---
TinyGPSPlus gps;
HardwareSerial GPS_Serial(2);  // UART2

void setup() {
  // Inicia el monitor serie para debug
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  // Inicia UART2 para el GPS
  GPS_Serial.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);

  Serial.println();
  Serial.println(F("=== Iniciando GY-NEO6MV2 en UART2 (GPIO16) ==="));
  Serial.print(F("Baudrate: "));
  Serial.println(GPSBaud);
  Serial.println(F("Esperando datos GPS..."));
  Serial.println();
}

void loop() {
  // Lee todos los bytes disponibles del GPS y los pasa a TinyGPS++
  while (GPS_Serial.available() > 0) {
    gps.encode(GPS_Serial.read());
  }

  // Cuando haya nueva posición válida, la mostramos
  if (gps.location.isUpdated()) {
    Serial.print(F("Latitud:  "));
    Serial.println(gps.location.lat(), 6);
    Serial.print(F("Longitud: "));
    Serial.println(gps.location.lng(), 6);
    Serial.print(F("Altitud:  "));
    if (gps.altitude.isValid()) {
      Serial.print(gps.altitude.meters());
      Serial.println(F(" m"));
    } else {
      Serial.println(F("N/A"));
    }
    Serial.print(F("Velocidad: "));
    if (gps.speed.isValid()) {
      Serial.print(gps.speed.kmph());
      Serial.println(F(" km/h"));
    } else {
      Serial.println(F("N/A"));
    }
    Serial.print(F("Satélites: "));
    if (gps.satellites.isValid()) {
      Serial.println(gps.satellites.value());
    } else {
      Serial.println(F("N/A"));
    }

    // Fecha y hora GPS (UTC)
    if (gps.date.isValid() && gps.time.isValid()) {
      Serial.print(F("Fecha UTC: "));
      Serial.print(gps.date.day());
      Serial.print(F("/"));
      Serial.print(gps.date.month());
      Serial.print(F("/"));
      Serial.print(gps.date.year());
      Serial.print(F("  Hora UTC: "));
      if (gps.time.hour() < 10) Serial.print('0');
      Serial.print(gps.time.hour());
      Serial.print(':');
      if (gps.time.minute() < 10) Serial.print('0');
      Serial.print(gps.time.minute());
      Serial.print(':');
      if (gps.time.second() < 10) Serial.print('0');
      Serial.print(gps.time.second());
      Serial.println();
    }

    Serial.println(F("---------------------------"));
  }
}

