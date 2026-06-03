#include "radio.h"
#include "config.h"
#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

// Receive buffer protected by mutex (ISR callback → loop)
static ExtPacket lastPacket = {};
static volatile bool newPacketAvailable = false;
static unsigned long lastReceivedMs = 0;
static bool radioOk = false;

// Callback triggered when an ESP-NOW packet is received
// Signature for ESP-IDF v4.x (arduino-esp32 v2.x)
static void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
    // Verify packet size
    if (len != sizeof(ExtPacket)) {
        Serial.printf("[RADIO] Packet with incorrect size: %d (expected %d)\n", len, sizeof(ExtPacket));
        return;
    }

    // Copy data to a temporary structure
    ExtPacket pkt;
    memcpy(&pkt, data, sizeof(ExtPacket));

    // Validate checksum
    if (!validateChecksum(pkt)) {
        Serial.println(F("[RADIO] INVALID checksum, packet discarded"));
        return;
    }

    // Verify station ID
    if (pkt.stationId != STATION_ID) {
        Serial.printf("[RADIO] Station ID %d does not match (expected %d)\n", pkt.stationId, STATION_ID);
        return;
    }

    // Valid packet!
    lastPacket = pkt;
    newPacketAvailable = true;
    lastReceivedMs = millis();

    Serial.printf("[RADIO] Packet received! T:%.1f H:%.1f CO2:%u Wind:%.1f Rain:%.2f Bat:%umV\n",
                  pkt.temperature, pkt.humidity, pkt.eco2,
                  pkt.windSpeed, pkt.rainfall, pkt.batteryMv);
}

bool radio_init() {
    // NOTE: WiFi must already be initialized by wifi_manager
    // ESP-NOW uses the same channel as the WiFi STA connection

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println(F("[RADIO] ESP-NOW init FAILED"));
        return false;
    }

    // Register receive callback
    esp_now_register_recv_cb(onDataRecv);

    radioOk = true;
    Serial.println(F("[RADIO] ESP-NOW RX initialized"));
    return true;
}

bool radio_is_ok() {
    return radioOk;
}

bool radio_available() {
    return newPacketAvailable;
}

bool radio_read(ExtPacket& pkt) {
    if (!newPacketAvailable) return false;
    pkt = lastPacket;
    newPacketAvailable = false;
    return true;
}

unsigned long radio_last_received_ms() {
    return lastReceivedMs;
}
