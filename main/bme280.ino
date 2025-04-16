#include <Wire.h>
#include <Adafruit_BME280.h>

// #define PH_SENSOR_PIN 4
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

    DEBUG_PORT.print("Temperature: ");
    DEBUG_PORT.println(temp_f);
    DEBUG_PORT.print("Humidity: ");
    DEBUG_PORT.println(hum_f);
    DEBUG_PORT.print("Pressure: ");
    DEBUG_PORT.println(pres_f);
}

// bool buildPacket_BME280() {
//     int i = 0;
//     DEBUG_PORT.println("BME280 Reading.");

//     txBuffer[i++] = temperature >> 24;
//     txBuffer[i++] = temperature >> 16;
//     txBuffer[i++] = temperature >> 8;
//     txBuffer[i++] = temperature;
//     txBuffer[i++] = humidity >> 24;
//     txBuffer[i++] = humidity >> 16;
//     txBuffer[i++] = humidity >> 8;
//     txBuffer[i++] = humidity;
//     txBuffer[i++] = pressure >> 24;
//     txBuffer[i++] = pressure >> 16;
//     txBuffer[i++] = pressure >> 8;
//     txBuffer[i++] = pressure;

//     return true;
// }