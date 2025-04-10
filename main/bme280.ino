// //#include <Wire.h>
// //#include <Adafruit_BME280.h>  // BME280 Library
 
// #define PH_SENSOR_PIN 4
// #define LOCAL_ALTITUDE 35.0  // Your altitude in meters (adjust if needed)

// //Adafruit_BME280 bme;  // I2C (default: 0x77)

// float temp_f;
// float hum_f;
// float pres_f;

// uint32_t temperature;
// uint32_t humidity;
// uint32_t pressure;

// void bme280_loop() {
//   //Serial.println();
 
//   // Read and print temperature
//   //Serial.print("Temperature (C): ");
//   //Serial.println(bme.readTemperature(), 2);
//   temp_f = bme.readTemperature();

//   // Read and print pressure (in hPa)
//   //Serial.print("Pressure (hPa): ");
//   //Serial.println(bme.readPressure() / 100.0, 2);  // Convert Pa to hPa
//   pres_f = bme.readPressure() / 100.0;

//   // Read and print humidity
//   //Serial.print("Humidity (%RH): ");
//   //Serial.println(bme.readHumidity(), 2);
//   hum_f = bme.readHumidity();

//   // Calculate sea-level pressure (for accurate altitude)
//   //float seaLevelPressure = bme.readPressure() / pow(1 - (LOCAL_ALTITUDE / 44330.0), 5.255);
//   //Serial.print("Sea-level Pressure (hPa): ");
//   //Serial.println(seaLevelPressure / 100.0, 2);

//   // Read altitude (calibrated to your location)
//   //Serial.print("Altitude (m): ");
//   //Serial.println(bme.readAltitude(seaLevelPressure / 100.0), 2);

//   // Read PH sensor (calibrate properly)
//   //int phRaw = analogRead(PH_SENSOR_PIN);
//   //float phValue = map(phRaw, 0, 1023, 0, 140) / 10.0;  // Basic calibration
//   //Serial.print("PH: ");
//   //Serial.println(phValue, 1);
  
//   // Scaled and stored as uint32_t
//   temperature = (uint32_t)(temp_f * 100);
//   humidity = (uint32_t)(hum_f * 100);
//   pressure = (uint32_t)(pres_f * 100);

//   DEBUG_PORT.print("Temperture: ");
//   DEBUG_PORT.println(temp_f);
//   DEBUG_PORT.print("Humidity: ");
//   DEBUG_PORT.println(hum_f);
//   DEBUG_PORT.print("Pressure: ");
//   DEBUG_PORT.println(pres_f);

//   //delay(2000);  // Wait 2 seconds
// }

// bool buildPacket_BME280()
// {
//     int i = 0;
//     DEBUG_PORT.println("BME280 Reanding.");

//     txBuffer[i] = temperature >> 24;
//     txBuffer[i+1] = temperature >> 16;
//     txBuffer[i+2] = temperature >> 8;
//     txBuffer[i+3] = temperature;
//     txBuffer[i+4] = humidity >> 24;
//     txBuffer[i+5] = humidity >> 16;
//     txBuffer[i+6] = humidity >> 8;
//     txBuffer[i+7] = humidity;
//     txBuffer[i+8] = pressure >> 24;
//     txBuffer[i+9] = pressure >> 16;
//     txBuffer[i+10] = pressure >> 8;
//     txBuffer[i+11] = pressure;
//    // txBuffer[i++] = longitude >> 8;
//    // txBuffer[i++] = longitude;
//    // txBuffer[i++] = altitude >> 16;
//    // txBuffer[i++] = altitude >> 8;
//    // txBuffer[i++] = altitude;
//    // txBuffer[i++] = sats;
//     //txBuffer[i++] = (char)'t';

//    // isDataFresh = false;
//     return true;
// }







#include <Wire.h>
#include <Adafruit_BME280.h>

#define PH_SENSOR_PIN 4
#define LOCAL_ALTITUDE 35.0

extern Adafruit_BME280 bme;  // Declared in main.ino

float temp_f;
float hum_f;
float pres_f;

uint32_t temperature;
uint32_t humidity;
uint32_t pressure;

void bme280_loop() {
    temp_f = bme.readTemperature();
    pres_f = bme.readPressure() / 100.0;
    hum_f = bme.readHumidity();

    temperature = (uint32_t)(temp_f * 100);
    humidity = (uint32_t)(hum_f * 100);
    pressure = (uint32_t)(pres_f * 100);

    DEBUG_PORT.print("Temperture: ");
    DEBUG_PORT.println(temp_f);
    DEBUG_PORT.print("Humidity: ");
    DEBUG_PORT.println(hum_f);
    DEBUG_PORT.print("Pressure: ");
    DEBUG_PORT.println(pres_f);
}

bool buildPacket_BME280() {
    int i = 0;
    DEBUG_PORT.println("BME280 Reading.");

    txBuffer[i++] = temperature >> 24;
    txBuffer[i++] = temperature >> 16;
    txBuffer[i++] = temperature >> 8;
    txBuffer[i++] = temperature;
    txBuffer[i++] = humidity >> 24;
    txBuffer[i++] = humidity >> 16;
    txBuffer[i++] = humidity >> 8;
    txBuffer[i++] = humidity;
    txBuffer[i++] = pressure >> 24;
    txBuffer[i++] = pressure >> 16;
    txBuffer[i++] = pressure >> 8;
    txBuffer[i++] = pressure;

    return true;
}