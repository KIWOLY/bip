#include <MQUnifiedsensor.h>
#include "configuration.h"
#include <math.h>

#define MQ135_PIN 36
#define MQ135_TYPE "MQ-135"
#define VOLTAGE_RESOLUTION 3.3
#define ADC_BIT_RESOLUTION 12
#define RLOAD 5.0

#define CO2_A 1356.35
#define CO2_B -2.769034857
#define NOX_A 0.28
#define NOX_B -2.5
#define ALCOHOL_A 0.43
#define ALCOHOL_B -3.18
#define BENZENE_A 0.06
#define BENZENE_B -3.445

MQUnifiedsensor MQ135("ESP-32", VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ135_PIN, MQ135_TYPE);

float co2_ppm, nox_ppm, alcohol_ppm, benzene_ppm;
uint32_t co2, nox, alcohol, benzene;
RTC_DATA_ATTR float savedRatioCleanAir = 0.0;

extern RTC_DATA_ATTR float savedR0;

void mq135_setup() {
    Serial.println("MQ-135 Calibration (RLOAD = 5.0 kOhm, 3.3V)");
    analogSetAttenuation(ADC_11db);
    MQ135.init();
    Serial.println("MQ-135 Initialized");
    Serial.flush();

    int buttonState = digitalRead(BUTTON_PIN);
    bool forceRecalibration = (buttonState == LOW);
    Serial.print("Button state (GPIO 0): ");
    Serial.println(buttonState == HIGH ? "HIGH (not pressed)" : "LOW (pressed)");
    Serial.print("Saved R0: ");
    Serial.print(savedR0);
    Serial.println(" kOhm");
    Serial.print("Saved RatioCleanAir: ");
    Serial.print(savedRatioCleanAir);
    Serial.println("");
    Serial.print("Force recalibration: ");
    Serial.println(forceRecalibration ? "Yes" : "No");
    Serial.flush();

    if (savedR0 <= 0 || isinf(savedR0) || forceRecalibration) {
        Serial.println("Heating MQ-135 sensor for 60 seconds...");
        delay(60000);
        Serial.flush();

        Serial.println("Calibrating MQ-135 sensor...");
        float calcR0 = 0;
        float calcRs = 0;
        const int max_samples = 60;
        for (int i = 1; i <= max_samples; i++) {
            MQ135.update();
            float voltage = 0;
            const int samples = 10;
            long sum = 0;
            for (int j = 0; j < samples; j++) {
                sum += analogRead(MQ135_PIN);
                delay(10);
            }
            voltage = (sum / samples) * (VOLTAGE_RESOLUTION / 4095.0);
            float rs = ((VOLTAGE_RESOLUTION / voltage) - 1) * RLOAD;
            calcRs += rs;
            if (i % 5 == 0) {
                Serial.print("Calibration iteration ");
                Serial.print(i);
                Serial.print(": Voltage = ");
                Serial.print(voltage);
                Serial.print(" V, Rs = ");
                Serial.print(rs);
                Serial.println(" kOhm");
                Serial.flush();
            }
            delay(1000);
        }
        calcRs /= max_samples;

        // Calculate RATIO_MQ135_CLEAN_AIR for 415 ppm CO2
        float targetCO2 = 415.0;
        float rs_ro_ratio = pow(targetCO2 / CO2_A, 1.0 / CO2_B);
        savedRatioCleanAir = rs_ro_ratio;
        calcR0 = calcRs / savedRatioCleanAir;
        MQ135.setR0(calcR0);

        Serial.print("Calculated RATIO_MQ135_CLEAN_AIR: ");
        Serial.println(savedRatioCleanAir);
        Serial.print("Calculated R0: ");
        Serial.print(calcR0);
        Serial.println(" kOhm");

        Serial.println("Validating calibration for 60 seconds...");
        delay(60000);
        Serial.flush();
        float test_co2_sum = 0;
        const int test_samples = 5;
        for (int i = 0; i < test_samples; i++) {
            MQ135.update();
            float voltage = 0;
            const int samples = 10;
            long sum = 0;
            for (int j = 0; j < samples; j++) {
                sum += analogRead(MQ135_PIN);
                delay(10);
            }
            voltage = (sum / samples) * (VOLTAGE_RESOLUTION / 4095.0);
            float rs = ((VOLTAGE_RESOLUTION / voltage) - 1) * RLOAD;
            float rs_ro_ratio = rs / calcR0;
            float temp_comp = 1.0; // Default during calibration
            rs_ro_ratio /= temp_comp;
            float test_co2 = CO2_A * pow(rs_ro_ratio, CO2_B);
            test_co2_sum += test_co2;
            delay(1000);
        }
        float test_co2 = test_co2_sum / test_samples;
        Serial.print("Calibration test CO2: ");
        Serial.print(test_co2);
        Serial.println(" ppm");

        savedR0 = calcR0;
        Serial.print("Saving R0: ");
        Serial.print(savedR0);
        Serial.println(" kOhm");
        if (test_co2 < 300 || test_co2 > 500) {
            Serial.println("Warning: CO2 reading out of expected clean air range (300–500 ppm). Recalibrate in fresh air.");
        } else {
            Serial.print("Calibration complete. Average R0: ");
            Serial.print(calcR0);
            Serial.println(" kOhm");
        }
        Serial.flush();
    } else {
        Serial.print("Using saved R0: ");
        Serial.print(savedR0);
        Serial.println(" kOhm");
        Serial.print("Using saved RatioCleanAir: ");
        Serial.print(savedRatioCleanAir);
        Serial.println("");
        MQ135.setR0(savedR0);
    }

    Serial.println("MQ-135 ready for gas measurements");
    Serial.flush();
}

void mq135_loop(float bme280_temp) {
    MQ135.update();
    float voltage = 0;
    const int samples = 10;
    long sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += analogRead(MQ135_PIN);
        delay(5);
        if (i == samples - 1) Serial.flush();
    }
    voltage = (sum / samples) * (VOLTAGE_RESOLUTION / 4095.0);

    if (voltage < 0.01) {
        Serial.println("Error: Sensor voltage too low or disconnected");
        Serial.flush();
        return;
    }

    float rs = ((VOLTAGE_RESOLUTION / voltage) - 1) * RLOAD;
    float rs_ro_ratio = rs / MQ135.getR0();
    float temp_comp = (bme280_temp > 0 && bme280_temp < 100) ? 1.0 + ((bme280_temp - 25.0) * 0.0015) : 1.0;
    rs_ro_ratio /= temp_comp;

    Serial.print("Temperature compensation factor: ");
    Serial.println(temp_comp, 5);
    Serial.print("Rs/R0 ratio: ");
    Serial.println(rs_ro_ratio);

    co2_ppm = CO2_A * pow(rs_ro_ratio, CO2_B);
    nox_ppm = NOX_A * pow(rs_ro_ratio, NOX_B);
    alcohol_ppm = ALCOHOL_A * pow(rs_ro_ratio, ALCOHOL_B);
    benzene_ppm = BENZENE_A * pow(rs_ro_ratio, BENZENE_B);

    co2 = (uint32_t)(co2_ppm * 10);
    nox = (uint32_t)(nox_ppm * 1000);
    alcohol = (uint32_t)(alcohol_ppm * 100);
    benzene = (uint32_t)(benzene_ppm * 1000);

    Serial.println("Gas Concentrations:");
    Serial.print("CO2: "); Serial.print(co2_ppm); Serial.println(" ppm"); Serial.flush();
    Serial.print("NOx: "); Serial.print(nox_ppm); Serial.println(" ppm"); Serial.flush();
    Serial.print("Alcohol (VOCs): "); Serial.print(alcohol_ppm); Serial.println(" ppm"); Serial.flush();
    Serial.print("Benzene: "); Serial.print(benzene_ppm); Serial.println(" ppm"); Serial.flush();

    if (co2_ppm < 300 || co2_ppm > 500) {
        Serial.println("Warning: Runtime CO2 reading out of expected clean air range (300–500 ppm). Consider recalibrating.");
        Serial.flush();
    }
}