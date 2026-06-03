#include "radio.h"
#include "config.h"
#include "../../shared/packet.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Broadcast address — sends to all ESP-NOW receivers
static const uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Result of the last transmission
static volatile bool sendDone = false;
static volatile bool sendOk = false;

// Callback triggered when transmission is complete
static void onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    sendDone = true;
    sendOk = (status == ESP_NOW_SEND_SUCCESS);
}

bool radio_init() {
    // Initialize WiFi in Station mode (required for ESP-NOW)
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();  // Do not connect to any AP, just use ESP-NOW

    // Set WiFi channel to match the interior receiver
    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println(F("[RADIO] ESP-NOW init FAILED"));
        return false;
    }

    // Register transmission callback
    esp_now_register_send_cb(onDataSent);

    // Add broadcast peer
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, broadcastAddr, 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;

    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println(F("[RADIO] Adding broadcast peer FAILED"));
        return false;
    }

    Serial.printf("[RADIO] ESP-NOW TX initialized (channel %d)\n", ESPNOW_CHANNEL);
    return true;
}

bool radio_send(const uint8_t* data, size_t len) {
    sendDone = false;
    sendOk = false;

    esp_err_t result = esp_now_send(broadcastAddr, data, len);
    if (result != ESP_OK) {
        Serial.printf("[RADIO] esp_now_send FAILED: %d\n", result);
        return false;
    }

    // Wait for callback (100ms timeout)
    unsigned long start = millis();
    while (!sendDone && (millis() - start < 100)) {
        delay(1);
    }

    if (sendOk) {
        Serial.println(F("[RADIO] Packet sent OK"));
    } else {
        Serial.println(F("[RADIO] Transmission FAILED (no ACK or timeout)"));
    }
    return sendOk;
}

void radio_sleep() {
    esp_now_deinit();
    WiFi.mode(WIFI_OFF);
    Serial.println(F("[RADIO] ESP-NOW turned off"));
}
