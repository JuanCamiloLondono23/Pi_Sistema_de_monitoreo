#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

TinyGPSPlus gps;
HardwareSerial gpsSerial(2); // UART2 en ESP32

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
  Serial.println("Iniciando GPS...");
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());

    if (gps.location.isUpdated()) {
      Serial.println("----- Datos GPS -----");
      Serial.print("Latitud: ");
      Serial.println(gps.location.lat(), 6);
      Serial.print("Longitud: ");
      Serial.println(gps.location.lng(), 6);
      Serial.print("Altitud: ");
      Serial.println(gps.altitude.meters());
      Serial.print("Satélites: ");
      Serial.println(gps.satellites.value());

      // Fecha
      if (gps.date.isValid()) {
        Serial.print("Fecha (DD/MM/AAAA): ");
        Serial.print(gps.date.day());
        Serial.print("/");
        Serial.print(gps.date.month());
        Serial.print("/");
        Serial.println(gps.date.year());
      } else {
        Serial.println("Fecha no válida.");
      }

      // Hora
      if (gps.time.isValid()) {
        Serial.print("Hora (UTC): ");
        if (gps.time.hour() < 10) Serial.print('0');
        Serial.print(gps.time.hour());
        Serial.print(":");
        if (gps.time.minute() < 10) Serial.print('0');
        Serial.print(gps.time.minute());
        Serial.print(":");
        if (gps.time.second() < 10) Serial.print('0');
        Serial.println(gps.time.second());
      } else {
        Serial.println("Hora no válida.");
      }

      Serial.println("----------------------");
    }
  }
}
