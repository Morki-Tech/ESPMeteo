#pragma once
#include <Adafruit_AHTX0.h>
#include <ScioSense_ENS160.h>  // ENS160 - Adafruit Fork (ScioSense API)
#include <RTClib.h>

struct SensorData {
    float    temperature;   // °C (AHT21)
    float    humidity;      // %RH (AHT21)
    uint16_t eco2;          // ppm (ENS160)
    uint16_t tvoc;          // ppb (ENS160)
    uint32_t rtcTimestamp;  // Unix epoch (DS3231)
    float    windSpeed;     // m/s
    float    rainfall;      // Accumulated mm
    uint16_t batteryMv;     // mV
    bool     ahtOk;
    bool     ensOk;
    bool     rtcOk;
};

void sensors_init();
SensorData sensors_read();
void sensors_reset_rain();
uint16_t sensors_read_battery();
