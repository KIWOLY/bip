#include "configuration.h"
#include "rom/rtc.h"
#include <Wire.h>
#include <Adafruit_BME280.h>

#define LOCAL_ALTITUDE 35.0
Adafruit_BME280 bme;

bool packetSent, packetQueued;
static uint8_t txBuffer[16];  // 16 bytes: 12 for BME280 + 4 for DO

RTC_DATA_ATTR int bootCount = 0;
esp_sleep_source_t wakeCause;

extern float temp_f, hum_f, pres_f;  // From bme280.ino
extern uint32_t temperature, humidity, pressure;
extern float doValue;  // From oxygen.ino

void bme280_loop();  // From bme280.ino
void oxygen_setup(); // From oxygen.ino
void oxygen_loop();  // From oxygen.ino

bool buildPacket() {
    int i = 0;
    DEBUG_PORT.println("Building packet with BME280 and Oxygen data.");

    // BME280 data
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

    // Oxygen data (DO in mg/L * 100)
    uint32_t do_mgL = (uint32_t)(doValue * 100);
    txBuffer[i++] = do_mgL >> 24;
    txBuffer[i++] = do_mgL >> 16;
    txBuffer[i++] = do_mgL >> 8;
    txBuffer[i++] = do_mgL;

    return true;
}

bool trySend() {
    packetSent = false;
    packetQueued = true;

    bme280_loop();  // Update BME280 readings
    oxygen_loop();  // Update oxygen readings
    if (!buildPacket()) {
        return false;
    }

    bool confirmed = (LORAWAN_CONFIRMED_EVERY > 0 && ttn_get_count() % LORAWAN_CONFIRMED_EVERY == 0);
    ttn_send(txBuffer, sizeof(txBuffer), LORAWAN_PORT, confirmed);
    DEBUG_PORT.println("Sending packet...");
    return true;
}

void doDeepSleep(uint64_t msecToWake) {
    DEBUG_PORT.printf("Entering deep sleep for %llu seconds\n", msecToWake / 1000);
    LMIC_shutdown();
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    uint64_t gpioMask = (1ULL << BUTTON_PIN);
    gpio_pullup_en((gpio_num_t)BUTTON_PIN);
    esp_sleep_enable_ext1_wakeup(gpioMask, ESP_EXT1_WAKEUP_ALL_LOW);
    esp_sleep_enable_timer_wakeup(msecToWake * 1000ULL);
    esp_deep_sleep_start();
}

void sleep() {
#if SLEEP_BETWEEN_MESSAGES
    uint32_t sleep_for = (millis() < SEND_INTERVAL) ? SEND_INTERVAL - millis() : SEND_INTERVAL;
    doDeepSleep(sleep_for);
#endif
}

void callback(uint8_t message) {
    if (EV_JOINED == message) DEBUG_PORT.println("Joined TTN!");
    else if (EV_JOINING == message) DEBUG_PORT.println("Joining TTN...");
    else if (EV_TXCOMPLETE == message && packetQueued) {
        DEBUG_PORT.println("Message sent");
        packetQueued = false;
        packetSent = true;
    } else if (EV_PENDING == message) DEBUG_PORT.println("Message discarded");
    else if (EV_QUEUED == message) DEBUG_PORT.println("Message queued");
}

void initDeepSleep() {
    bootCount++;
    wakeCause = esp_sleep_get_wakeup_cause();
    DEBUG_PORT.printf("Booted, wake cause %d (boot count %d)\n", wakeCause, bootCount);
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial);
    delay(1000);

    Serial.println("Initializing BME280...");
    if (!bme.begin(0x77)) {  // Try 0x76 if this fails
        Serial.println("BME280 not found! Check wiring.");
        while (1);
    }
    Serial.println("BME280 OK!");

    Serial.println("Initializing Oxygen sensor...");
    oxygen_setup();
    Serial.println("Oxygen sensor OK!");

    initDeepSleep();
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    DEBUG_MSG(APP_NAME " " APP_VERSION "\n");
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
    if (millis() - last > SEND_INTERVAL) {
        if (trySend()) {
            last = millis();
            DEBUG_PORT.println("TRANSMITTED");
        }
    }
}