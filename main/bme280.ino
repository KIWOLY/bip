#include <Wire.h>
#include <Adafruit_BME280.h>

#define LOCAL_ALTITUDE 35.0
extern Adafruit_BME280 bme;

float temp_f, hum_f, pres_f;
uint32_t temperature, humidity, pressure;

void bme280_setup() {
    if (!bme.begin(0x77)) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }
}

void bme280_loop() {
    temp_f = bme.readTemperature();
    hum_f = bme.readHumidity();
    pres_f = bme.readPressure() / 100.0F;

    if (isnan(temp_f) || isnan(hum_f) || isnan(pres_f)) {
        Serial.println("Error: Failed to read from BME280 sensor!");
        return;
    }

    pres_f = bme.seaLevelForAltitude(LOCAL_ALTITUDE, pres_f);

    temperature = (uint32_t)(temp_f * 100);
    humidity = (uint32_t)(hum_f * 100);
    pressure = (uint32_t)(pres_f * 100);

    Serial.print("Temperature: ");
    Serial.print(temp_f);
    Serial.println(" °C");
    Serial.print("Humidity: ");
    Serial.print(hum_f);
    Serial.println(" %");
    Serial.print("Pressure: ");
    Serial.print(pres_f);
    Serial.println(" hPa");
    Serial.flush();
}