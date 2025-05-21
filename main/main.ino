#include "configuration.h"
#include "rom/rtc.h"
#include <Wire.h>
#include <Adafruit_BME280.h>

#define LOCAL_ALTITUDE 7.0 // Antwerp altitude in meters
#define LAT 51.213583  // N 51.213583°
#define LON 4.423684   // E 4.423684°

// External BME280 object defined in bme280.ino
extern Adafruit_BME280 bme;

// Global variables
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR uint32_t lastBootTime = 0;
esp_sleep_source_t wakeCause;
static uint8_t txBuffer[20]; // 20 bytes for sensor data and coordinates
bool packetSent = false, packetQueued = false;

// Function prototypes
void bme280_setup();
void bme280_loop();
void mq135_setup();
void mq135_loop(float bme280_temp);
bool buildPacket();
void doDeepSleep(uint64_t msecToWake);
void sleep();
void callback(uint8_t message);
void initDeepSleep();

bool buildPacket() {
    int i = 0;
    DEBUG_PORT.println("Building 20-byte packet with BME280, MQ-135, and coordinates.");

    // Temperature (2 bytes, 0.01°C resolution)
    int16_t temp_scaled = (int16_t)(temp_f * 100);
    txBuffer[i++] = temp_scaled >> 8;
    txBuffer[i++] = temp_scaled;

    // Humidity (2 bytes, 0.01% resolution)
    uint16_t hum_scaled = (uint16_t)(hum_f * 100);
    txBuffer[i++] = hum_scaled >> 8;
    txBuffer[i++] = hum_scaled;

    // Pressure (2 bytes, 0.1 hPa resolution)
    uint16_t pres_scaled = (uint16_t)(pres_f * 10);
    txBuffer[i++] = pres_scaled >> 8;
    txBuffer[i++] = pres_scaled;

    // CO2 (2 bytes, 0.1 ppm resolution)
    uint16_t co2_scaled = (uint16_t)(co2_ppm * 10);
    txBuffer[i++] = co2_scaled >> 8;
    txBuffer[i++] = co2_scaled;

    // NOx (2 bytes, 0.001 ppm resolution)
    uint16_t nox_scaled = (uint16_t)(nox_ppm * 1000);
    txBuffer[i++] = nox_scaled >> 8;
    txBuffer[i++] = nox_scaled;

    // Alcohol (2 bytes, 0.01 ppm resolution)
    uint16_t alcohol_scaled = (uint16_t)(alcohol_ppm * 100);
    txBuffer[i++] = alcohol_scaled >> 8;
    txBuffer[i++] = alcohol_scaled;

    // Benzene (2 bytes, 0.001 ppm resolution)
    uint16_t benzene_scaled = (uint16_t)(benzene_ppm * 1000);
    txBuffer[i++] = benzene_scaled >> 8;
    txBuffer[i++] = benzene_scaled;

    // Latitude (2 bytes, 0.001° resolution)
    int16_t lat_scaled = (int16_t)(LAT * 1000);
    txBuffer[i++] = lat_scaled >> 8;
    txBuffer[i++] = lat_scaled;

    // Longitude (2 bytes, 0.001° resolution)
    int16_t lon_scaled = (int16_t)(LON * 1000);
    txBuffer[i++] = lon_scaled >> 8;
    txBuffer[i++] = lon_scaled;

    // Debug: Print packet bytes
    DEBUG_PORT.print("Packet bytes: ");
    for (int j = 0; j < sizeof(txBuffer); j++) {
        DEBUG_PORT.printf("%02X ", txBuffer[j]);
    }
    DEBUG_PORT.println();

    return true;
}

bool trySend() {
    packetSent = false;
    packetQueued = true;

    bme280_loop();
    float bme280_temp = temp_f; // Get BME280 temperature
    delay(30000); // 30-second warm-up for MQ-135
    mq135_loop(bme280_temp);
    if (!buildPacket()) {
        return false;
    }

    bool confirmed = (LORAWAN_CONFIRMED_EVERY > 0 && ttn_get_count() % LORAWAN_CONFIRMED_EVERY == 0);
    ttn_send(txBuffer, sizeof(txBuffer), LORAWAN_PORT, confirmed);
    DEBUG_PORT.println("Sending 20-byte packet...");
    Serial.flush();
    return true;
}

void doDeepSleep(uint64_t msecToWake) {
    DEBUG_PORT.printf("Preparing to enter deep sleep for %llu milliseconds (%llu seconds)\n", msecToWake, msecToWake / 1000);
    LMIC_shutdown();
    Serial.flush();
    sleep_millis(msecToWake);
}

void sleep() {
#if SLEEP_BETWEEN_MESSAGES
    delay(MESSAGE_TO_SLEEP_DELAY);
    doDeepSleep(SEND_INTERVAL);
#endif
}

void callback(uint8_t message) {
    if (EV_JOINED == message) DEBUG_PORT.println("Joined TTN!");
    else if (EV_JOINING == message) DEBUG_PORT.println("Joining TTN...");
    else if (EV_TXCOMPLETE == message && packetQueued) {
        DEBUG_PORT.println("Packet sent successfully");
        packetQueued = false;
        packetSent = true;
    } else if (EV_PENDING == message) DEBUG_PORT.println("Message discarded");
    else if (EV_QUEUED == message) DEBUG_PORT.println("Message queued");
}

void initDeepSleep() {
    bootCount++;
    wakeCause = esp_sleep_get_wakeup_cause();
    uint32_t currentBootTime = millis();
    if (lastBootTime > 0 && wakeCause == ESP_SLEEP_WAKEUP_TIMER) {
        uint32_t timeSinceLastBoot = currentBootTime + SEND_INTERVAL; // Adjust for deep sleep duration
        if (timeSinceLastBoot < 1000000) {
            DEBUG_PORT.printf("Time since last boot: %u ms\n", timeSinceLastBoot);
        } else {
            DEBUG_PORT.println("Warning: Invalid time since last boot");
        }
    }
    lastBootTime = currentBootTime;
    DEBUG_PORT.printf("Booted, wake cause %d (boot count %d)\n", wakeCause, bootCount);
    DEBUG_PORT.printf("Boot time: %u ms\n", currentBootTime);
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    Serial.flush();
    delay(500);
    while (!Serial);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    Serial.println("Initializing BME280...");
    bme280_setup();
    Serial.println("BME280 OK!");

    Serial.println("Initializing MQ-135 sensor...");
    mq135_setup();
    Serial.println("MQ-135 sensor OK!");

    initDeepSleep();

    DEBUG_MSG(APP_NAME " " APP_VERSION "\n");
    DEBUG_MSG("SEND_INTERVAL: %lu ms\n", (unsigned long)SEND_INTERVAL);
    if (!ttn_setup()) {
        DEBUG_PORT.println("LoRa not found");
        if (REQUIRE_RADIO) sleep_forever();
    } else {
        ttn_register(callback);
        ttn_join();
        ttn_adr(LORAWAN_ADR);
        DEBUG_PORT.println("Setup complete!");
    }
}

void loop() {
    ttn_loop();
    if (packetSent) {
        packetSent = false;
        sleep();
    }

    static uint32_t last = 0;
    if (millis() - last >= SEND_INTERVAL) {
        if (trySend()) {
            last = millis();
            DEBUG_PORT.println("TRANSMITTED");
        }
    }
}