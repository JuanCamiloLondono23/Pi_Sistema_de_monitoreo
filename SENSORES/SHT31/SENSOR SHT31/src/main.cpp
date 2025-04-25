#include <Arduino.h>
#include "LIB_SHT31.h"

SHT31 sensor;

void setup() {
  Serial.begin(115200);
  delay(100);

  if (!sensor.begin()) {
    Serial.print("Error al iniciar el sensor: ");
    Serial.println(sensor.getErrorMessage());
    while (true) delay(1000); 
  }

  Serial.println("Sensor iniciado correctamente.");
}

void loop() {
  float temperature, humidity;

  if (sensor.read(temperature, humidity)) {
    Serial.print("Temperatura: ");
    Serial.print(temperature, 2);
    Serial.print(" °C  Humedad: ");
    Serial.print(humidity, 2);
    Serial.println(" %RH");
  } else {
    Serial.print("Error de lectura: ");
    Serial.println(sensor.getErrorMessage());
  }

  delay(1000);
}
