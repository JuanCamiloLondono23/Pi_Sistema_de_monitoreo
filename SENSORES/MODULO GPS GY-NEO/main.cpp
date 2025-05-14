#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <TimeLib.h>        // Para manejo de fecha/hora avanzado

// --- Configuración de pines UART2 ---
static const int RXPin     = 16;      // RX2: recibe datos (TX del GPS)
static const int TXPin     = 17;      // TX2: no se conecta al GPS
static const uint32_t GPSBaud = 9600; // Velocidad estándar GY‑NEO6MV2

// --- Offset de zona horaria (Colombia: UTC–5) ---
const int UTC_OFFSET_SECONDS = -5 * 3600;  // en segundos

TinyGPSPlus gps;
HardwareSerial GPS_Serial(2);  // UART2

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  GPS_Serial.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);

  Serial.println();
  Serial.println(F("=== GPS con hora local (Colombia) ==="));
  Serial.println(F("Instaladas librerías: TinyGPSPlus, TimeLib"));
  Serial.println();
}

void loop() {
  // 1) Leer datos del GPS
  while (GPS_Serial.available() > 0) {
    gps.encode(GPS_Serial.read());
  }

  // 2) Cuando tengamos fecha y hora válidas del GPS:
  if (gps.date.isValid() && gps.time.isValid()) {
    // Sincronizamos el reloj interno de TimeLib con UTC del GPS
    setTime(
      gps.time.hour(),
      gps.time.minute(),
      gps.time.second(),
      gps.date.day(),
      gps.date.month(),
      gps.date.year()
    );

    // Aplicamos el offset de zona horaria (puede mover fecha)
    adjustTime(UTC_OFFSET_SECONDS);

    // 3) Imprimir fecha y hora ya ajustadas
    Serial.print(F("Fecha (Colombia): "));
      if (day()   < 10) Serial.print('0'); Serial.print(day());
      Serial.print('/');
      if (month() < 10) Serial.print('0'); Serial.print(month());
      Serial.print('/');
      Serial.print(year());
    Serial.print(F("  Hora (Colombia): "));
      if (hour()   < 10) Serial.print('0'); Serial.print(hour());
      Serial.print(':');
      if (minute() < 10) Serial.print('0'); Serial.print(minute());
      Serial.print(':');
      if (second() < 10) Serial.print('0'); Serial.print(second());
    Serial.println();

    // 4) Imprimir datos de posición y satélites
    Serial.print(F("Latitud:  "));
    Serial.println(gps.location.lat(),  6);
    Serial.print(F("Longitud: "));
    Serial.println(gps.location.lng(),  6);
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

    Serial.println(F("---------------------------"));

    // Pequeño delay antes de la siguiente lectura
    delay(1000);
  }
}

