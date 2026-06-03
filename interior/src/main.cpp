/*
 * Weather Station — Interior Unit
 * ESP32-S3 N16R8
 *
 * Modules: Display, Encoder, NeoPixel, DHT11, ESP-NOW RX,
 *          SD Card, WiFi+NTP, Web Server
 */

#include <Arduino.h>
#include "config.h"
#include "spi_manager.h"
#include "display.h"
#include "encoder.h"
#include "neopixel.h"
#include "sensors.h"
#include "storage.h"
#include "radio.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "../../shared/packet.h"

// Forward declaration of web_server.cpp function
extern void web_server_update_ext_data(const ExtPacket& pkt);

// --- Global Data ---
static ExtPacket lastExtPacket = {};
static bool extDataValid = false;
static unsigned long lastDisplayUpdate = 0;
static unsigned long lastNeopixelUpdate = 0;
static unsigned long lastStorageLog = 0;
static const unsigned long STORAGE_LOG_INTERVAL = 300000;  // 5 min

// --- Encoder Callbacks ---
static void onEncoderRotate(int direction) {
    if (direction > 0) {
        display_next_screen();
    } else {
        display_prev_screen();
    }
}

static void onEncoderPress() {
    // Button press: cycle NeoPixel mode
    neopixel_next_mode();
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println(F("\n===================================="));
    Serial.println(F("  METEO STATION - Interior Unit"));
    Serial.println(F("====================================\n"));

    // --- 1. CRITICAL: SPI Manager first (CS pins HIGH + mutex) ---
    Serial.println(F("[MAIN] 1/8 Initializing SPI Manager..."));
    spi_manager_init();

    // --- 2. Display (uses SPI) ---
    Serial.println(F("[MAIN] 2/8 Initializing Display..."));
    display_init();
    delay(500);  // Allow screen to stabilize

    // --- 3. Encoder ---
    Serial.println(F("[MAIN] 3/8 Initializing Encoder..."));
    encoder_init();
    encoder_on_rotate(onEncoderRotate);
    encoder_on_press(onEncoderPress);

    // --- 4. NeoPixel ---
    Serial.println(F("[MAIN] 4/8 Initializing NeoPixel..."));
    neopixel_init();

    // --- 5. Local Sensors (DHT11) ---
    Serial.println(F("[MAIN] 5/8 Initializing Sensors..."));
    sensors_init();

    // --- 6. SD Card (uses SPI) ---
    Serial.println(F("[MAIN] 6/8 Initializing SD Card..."));
    storage_init();

    // --- 7. WiFi + Web Server (BEFORE radio to establish the channel) ---
    Serial.println(F("[MAIN] 7/8 Initializing WiFi..."));
    wifi_init();
    web_server_init();

    // --- 8. ESP-NOW Radio (AFTER WiFi, using its channel) ---
    Serial.println(F("[MAIN] 8/8 Initializing ESP-NOW..."));
    radio_init();

    Serial.println(F("\n[MAIN] ====== INITIALIZATION COMPLETE ======\n"));
}

void loop() {
    unsigned long now = millis();

    // --- Encoder: process events ---
    encoder_update();

    // --- Radio: check for new packets ---
    if (radio_available()) {
        ExtPacket pkt;
        if (radio_read(pkt)) {
            lastExtPacket = pkt;
            extDataValid = true;
            web_server_update_ext_data(pkt);
        }
    }

    // --- Display: update every DISPLAY_INTERVAL ---
    if (now - lastDisplayUpdate >= DISPLAY_INTERVAL) {
        lastDisplayUpdate = now;

        IndoorData indoor = sensors_read();

        DisplayData dd = {};
        // Indoor
        dd.indoorTemp = indoor.temperature;
        dd.indoorHum  = indoor.humidity;
        // Outdoor
        dd.outdoorTemp  = lastExtPacket.temperature;
        dd.outdoorHum   = lastExtPacket.humidity;
        dd.eco2         = lastExtPacket.eco2;
        dd.tvoc         = lastExtPacket.tvoc;
        dd.windSpeed    = lastExtPacket.windSpeed;
        dd.rainfall     = lastExtPacket.rainfall;
        dd.batteryMv    = lastExtPacket.batteryMv;
        dd.outdoorStale = !extDataValid ||
                          (radio_last_received_ms() > 0 &&
                           now - radio_last_received_ms() > RADIO_STALE_MS);
        // Status
        dd.wifiConnected = wifi_is_connected();
        dd.sdOk          = storage_is_ok();
        dd.radioOk       = radio_is_ok();
        dd.ipAddress     = wifi_get_ip();
        dd.timeStr       = wifi_get_time();
        dd.uptime        = now / 1000;

        display_update(dd);
    }

    // --- NeoPixel: animation ---
    if (now - lastNeopixelUpdate >= NEOPIXEL_INTERVAL) {
        lastNeopixelUpdate = now;
        float temp = extDataValid ? lastExtPacket.temperature : 20.0f;
        uint16_t co2 = extDataValid ? lastExtPacket.eco2 : 400;
        float rain = extDataValid ? lastExtPacket.rainfall : 0.0f;
        neopixel_update(temp, co2, rain);
    }

    // --- Storage: log every 5 minutes ---
    if (now - lastStorageLog >= STORAGE_LOG_INTERVAL) {
        lastStorageLog = now;

        IndoorData indoor = sensors_read();
        HistoryRecord rec = {};
        rec.timestamp   = wifi_get_epoch();
        rec.indoorTemp  = indoor.temperature;
        rec.indoorHum   = indoor.humidity;
        rec.outdoorTemp = lastExtPacket.temperature;
        rec.outdoorHum  = lastExtPacket.humidity;
        rec.eco2        = lastExtPacket.eco2;
        rec.tvoc        = lastExtPacket.tvoc;
        rec.windSpeed   = lastExtPacket.windSpeed;
        rec.rainfall    = lastExtPacket.rainfall;
        rec.batteryMv   = lastExtPacket.batteryMv;

        storage_log(rec);
        Serial.println(F("[MAIN] Data saved to SD + buffer"));
    }

    // --- WiFi: reconnection ---
    wifi_update();

    // Yield for WDT and async tasks
    yield();
}
