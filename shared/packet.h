#pragma once
#include <stdint.h>
#include <stddef.h>

/*
 * ExtPacket — Data packet from the exterior station
 * Sent via ESP-NOW every 5 minutes.
 * Max 250 bytes (ESP-NOW limit). Current: 27 bytes.
 */
struct __attribute__((packed)) ExtPacket {
    uint8_t  stationId;       // Station ID (default 1)
    float    temperature;     // AHT21 °C
    float    humidity;        // AHT21 %RH
    uint16_t eco2;            // ENS160 ppm
    uint16_t tvoc;            // ENS160 ppb
    float    windSpeed;       // m/s from the anemometer
    float    rainfall;        // Accumulated mm of rain since last transmission
    uint16_t batteryMv;       // Battery voltage in mV
    uint32_t rtcTimestamp;    // Unix timestamp from the DS3231
    uint8_t  checksum;        // XOR of all previous bytes
};

// Calculates the XOR checksum of all packet bytes (excluding the checksum field itself)
inline uint8_t calcChecksum(const ExtPacket& pkt) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&pkt);
    uint8_t cs = 0;
    // sizeof(ExtPacket) - 1 to exclude the checksum field (last byte)
    for (size_t i = 0; i < sizeof(ExtPacket) - 1; i++) {
        cs ^= data[i];
    }
    return cs;
}

// Validates the checksum of a received packet
inline bool validateChecksum(const ExtPacket& pkt) {
    return pkt.checksum == calcChecksum(pkt);
}

// ESP-NOW communication constants
constexpr uint8_t  ESPNOW_CHANNEL   = 1;     // WiFi channel (align with your router's channel)
constexpr uint8_t  STATION_ID       = 1;
