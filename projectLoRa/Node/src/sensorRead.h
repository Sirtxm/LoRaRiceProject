#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>
#include <Adafruit_INA219.h>
// #define SEALEVELPRESSURE_HPA (1013.25)

// Libraly VL53L1X
#include "Adafruit_VL53L1X.h"
#define IRQ_PIN 2
#define XSHUT_PIN 3
Adafruit_VL53L1X vl53 = Adafruit_VL53L1X(XSHUT_PIN, IRQ_PIN);
//

#ifndef SENSOR_READ_H
#define SENSOR_READ_H

// SoftwareSerial mySerial(16, 17); // RX, TX เชื่อมต่อสายสีเหลืองกับ 17 และสายขาวกับ 16
//Ultrasonic ultrasonic(16, 17);
extern byte triggerPin = 17;
extern byte echoPin = 16;
extern double distance = 0;
extern double zeroDistance = 0;  // ค่า zero ที่จะกำหนด
extern double adjustedDistance = 0;// ระยะทางที่ปรับ zero แล้ว
////////////////////////////////////////////////
extern float temp = 0;
extern float hum = 0;
////////////////////////////////////////////////
#endif // SENSOR_READ_H
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;

Adafruit_BME280 bme;
Adafruit_INA219 ina219;

unsigned char data[4] = {};

float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float loadvoltage = 0;
float power_mW = 0;
////////////////////////////////////////////////
void updateSensorTemp_Hum(){
  unsigned long currentMillis1 = millis();
  if (currentMillis1 - previousMillis1 >= 1000) {
    previousMillis1 = currentMillis1;
    // temp = bme.readTemperature();
    // hum = bme.readHumidity();
    // Serial.print(temp);
    // Serial.print("        ");
    // Serial.print(hum);
    // Serial.println("        ");
  }
}

void updateSensorLaser(){
  unsigned long currentMillis2 = millis();
  if (currentMillis2 - previousMillis2 >= 1000) {
    previousMillis2 = currentMillis2;
    int16_t distancesValue = vl53.distance();
    distance = distancesValue/10;
    adjustedDistance = 50 - distance;
    if (vl53.dataReady()) {
      // new measurement for the taking!
      distance = vl53.distance();
      if (distance == -1) {
        // something went wrong!
        Serial.print(F("Couldn't get distance: "));
        Serial.println(vl53.vl_status);
        return;
      }
    }

    // Serial.print("zeroDistance: ");
    // Serial.print(zeroDistance);
    // Serial.print("adjustedDistance: ");
    // Serial.println(adjustedDistance);
  }
}

// void updateSensorUltra(){
//   unsigned long currentMillis2 = millis();
//   if (currentMillis2 - previousMillis2 >= 1000) {
//     previousMillis2 = currentMillis2;
//     for (int i = 0; i < 4; i++) {
//       while (mySerial.available() == 0) {
//       }
//       data[i] = mySerial.read();
//     }
//     mySerial.flush();
//     if (data[0] == 0xff) {
//       int sum;
//       sum = (data[0] + data[1] + data[2]) & 0x00FF;
//       if (sum == data[3]) {
//         distance = (data[1] << 8) + data[2];
//         distance = distance / 10;
//         adjustedDistance = distance - zeroDistance;   //คำนวณและแสดงผลระยะทางที่ปรับ zero แล้ว
//         Serial.print("zeroDistance: ");
//         Serial.print(zeroDistance);
//         Serial.print("adjustedDistance: ");
//         adjustedDistance = -adjustedDistance;
//         Serial.println(adjustedDistance);
//       }
//     }
//   }
// }
void updateSensorVoltage(){
  unsigned long currentMillis3 = millis();
  if (currentMillis3 - previousMillis3 >= 1000) {
    previousMillis3 = currentMillis3;
    shuntvoltage = ina219.getShuntVoltage_mV();
    busvoltage = ina219.getBusVoltage_V();
    current_mA = ina219.getCurrent_mA();
    power_mW = ina219.getPower_mW();
    loadvoltage = busvoltage + (shuntvoltage / 1000);
  }
  // Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  // Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  // Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  // Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  // Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
// }