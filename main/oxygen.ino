#include <Wire.h>
#include <math.h>

#define DO_PIN 39         // ADC1 pin (GPIO36 on FireBeetle)
#define VREF 3300         // 3.3V in millivolts
#define ADC_RES 4095      // 12-bit ADC resolution
#define CAL1_V 1600       // Calibration voltage in mV
#define CAL1_T 25         // Calibration temperature in °C

// DO saturation table (mg/L * 1000) for 0°C to 40°C
const uint16_t Do_Table[41] = {
  14460, 13940, 13500, 13090, 12700, 12310, 12010, 11710, 11440, 11170,
  10920, 10690, 10460, 10250, 10040, 9840, 9650, 9460, 9270, 9080,
  8910, 8740, 8570, 8410, 8250, 8090, 7930, 7780, 7630, 7490,
  7350, 7210, 7070, 6940, 6810, 6680, 6560, 6440, 6320, 6200,
  6090
};

float doValue; // Global variable to store DO value
extern float temp_f; // From bme280.ino

void oxygen_setup() {
  pinMode(DO_PIN, INPUT);
}

void oxygen_loop() {
  int rawDO = analogRead(DO_PIN);
  float voltage = (rawDO * VREF) / float(ADC_RES);
  
  // Use BME280 temperature, constrain to 0-40°C
  int waterTemp = constrain(int(temp_f + 0.5), 0, 40);
  float saturation = Do_Table[waterTemp] / 1000.0;
  
  // Calculate DO in mg/L
  doValue = (voltage / CAL1_V) * saturation;
  
  DEBUG_PORT.print("Dissolved Oxygen (mg/L): ");
  DEBUG_PORT.println(doValue, 2);
}