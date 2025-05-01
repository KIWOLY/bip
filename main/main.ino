#include "configuration.h"
#include "rom/rtc.h"
#include <Wire.h>
#include <Adafruit_BME280.h>

#define LOCAL_ALTITUDE 35.0
Adafruit_BME280 bme;

// Global variables
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR float savedR0 = 0;
RTC_DATA_ATTR uint32_t lastBootTime = 0;
esp_sleep_source_t wakeCause;
static uint8_t txBuffer[28];
bool packetSent = false, packetQueued = false;

extern float temp_f, hum_f, pres_f;
extern uint32_t temperature, humidity, pressure;
extern float co2_ppm, nox_ppm, alcohol_ppm, benzene_ppm;
extern uint32_t co2, nox, alcohol, benzene;

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
    DEBUG_PORT.println("Building 28-byte packet with BME280 and MQ-135 data.");

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
    txBuffer[i++] = co2 >> 24;
    txBuffer[i++] = co2 >> 16;
    txBuffer[i++] = co2 >> 8;
    txBuffer[i++] = co2;
    txBuffer[i++] = nox >> 24;
    txBuffer[i++] = nox >> 16;
    txBuffer[i++] = nox >> 8;
    txBuffer[i++] = nox;
    txBuffer[i++] = benzene >> 24;
    txBuffer[i++] = benzene >> 16;
    txBuffer[i++] = benzene >> 8;
    txBuffer[i++] = benzene;
    txBuffer[i++] = alcohol >> 24;
    txBuffer[i++] = alcohol >> 16;
    txBuffer[i++] = alcohol >> 8;
    txBuffer[i++] = alcohol;

    return true;
}

bool trySend() {
    packetSent = false;
    packetQueued = true;

    bme280_loop();
    float bme280_temp = temp_f; // Get BME280 temperature
    delay(5000); // 5-second warm-up for MQ-135
    mq135_loop(bme280_temp);
    if (!buildPacket()) {
        return false;
    }

    bool confirmed = (LORAWAN_CONFIRMED_EVERY > 0 && ttn_get_count() % LORAWAN_CONFIRMED_EVERY == 0);
    ttn_send(txBuffer, sizeof(txBuffer), LORAWAN_PORT, confirmed);
    DEBUG_PORT.println("Sending 28-byte packet...");
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