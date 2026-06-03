/*
 * Weather Station — Exterior Unit
 * ESP32-S3 Nano
 *
 * Cycle: boot → sensors → ESP-NOW TX → deep sleep 5min → repeat
 */

#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "radio.h"
#include "../../shared/packet.h"

// Boot counter (persists during deep sleep)
RTC_DATA_ATTR static uint32_t bootCount = 0;

void setup() {
    Serial.begin(115200);
    delay(100);

    bootCount++;
    Serial.printf("\n============================\n");
    Serial.printf("  EXTERIOR UNIT - Boot #%u\n", bootCount);
    Serial.printf("============================\n");

    // --- 1. Initialize sensors ---
    Serial.println(F("[MAIN] Initializing sensors..."));
    sensors_init();

    // Wait for ENS160 to be ready (needs ~1s after init)
    delay(1000);

    // --- 2. Read all sensors ---
    Serial.println(F("[MAIN] Reading sensors..."));
    SensorData data = sensors_read();

    Serial.printf("[MAIN] Temp: %.1f°C  Hum: %.1f%%\n", data.temperature, data.humidity);
    Serial.printf("[MAIN] eCO2: %u ppm  TVOC: %u ppb\n", data.eco2, data.tvoc);
    Serial.printf("[MAIN] Wind: %.2f m/s  Rain: %.2f mm\n", data.windSpeed, data.rainfall);
    Serial.printf("[MAIN] Battery: %u mV  RTC: %u\n", data.batteryMv, data.rtcTimestamp);

    // --- 3. Build packet ---
    ExtPacket pkt = {};
    pkt.stationId    = STATION_ID;
    pkt.temperature  = data.temperature;
    pkt.humidity     = data.humidity;
    pkt.eco2         = data.eco2;
    pkt.tvoc         = data.tvoc;
    pkt.windSpeed    = data.windSpeed;
    pkt.rainfall     = data.rainfall;
    pkt.batteryMv    = data.batteryMv;
    pkt.rtcTimestamp = data.rtcTimestamp;
    pkt.checksum     = calcChecksum(pkt);

    // --- 4. Initialize radio and send ---
    Serial.println(F("[MAIN] Initializing radio..."));
    if (radio_init()) {
        radio_send(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt));
        radio_sleep();
    }

    // --- 5. Reset rain counter after sending ---
    sensors_reset_rain();

    // --- 6. Deep sleep ---
    Serial.printf("[MAIN] Entering deep sleep (%llu min)...\n",
                  SLEEP_DURATION_US / 60000000ULL);
    Serial.flush();

    esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
    esp_deep_sleep_start();

    // Never reached
}

void loop() {
    // Unused — the cycle is setup-only with deep sleep
}
