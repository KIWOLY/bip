#pragma once

#include <lmic.h>
void ttn_register(void (*callback)(uint8_t message));
void sleep_forever();

#define APP_NAME "TTN SENSOR NODE"
#define APP_VERSION "1.0.3"

#define SERIAL_BAUD 115200
#define DEBUG_PORT Serial

#define LORAWAN_PORT 10
#define LORAWAN_CONFIRMED_EVERY 0
#define LORAWAN_SF DR_SF7
#define LORAWAN_ADR false
#define SEND_INTERVAL 120000 // 120 seconds (2 minutes)
#define SLEEP_BETWEEN_MESSAGES false
#define MESSAGE_TO_SLEEP_DELAY 1000
#define REQUIRE_RADIO true
#define DUMMY_DATA false

#define I2C_SDA 21
#define I2C_SCL 22
#define BUTTON_PIN 0 // FireBeetle ESP32 boot button

// LoRa SPI for FireBeetle ESP32 with RFM95
#define SCK_GPIO 18
#define MISO_GPIO 19
#define MOSI_GPIO 23
#define NSS_GPIO D3
#define RESET_GPIO D4
#define DIO0_GPIO D2
#define DIO1_GPIO D5
#define DIO2_GPIO LMIC_UNUSED_PIN

#ifdef DEBUG_PORT
#define DEBUG_MSG(...) DEBUG_PORT.printf(__VA_ARGS__)
#define DEBUG_MSG_N(...) DEBUG_PORT.print(__VA_ARGS__)
#else
#define DEBUG_MSG(...)
#endif

#define EV_QUEUED 100
#define EV_PENDING 101
#define EV_ACK 102
#define EV_RESPONSE 103

// Shared global variables
extern float temp_f, hum_f, pres_f;
extern uint32_t temperature, humidity, pressure;
extern float co2_ppm, nox_ppm, alcohol_ppm, benzene_ppm;
extern uint32_t co2, nox, alcohol, benzene;
extern RTC_DATA_ATTR float savedRatioCleanAir;
extern RTC_DATA_ATTR float savedR0;