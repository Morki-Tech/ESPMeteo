#pragma once

// =====================================================
// Weather Station — Exterior Unit — Configuration
// =====================================================

// --- I2C Pins (ENS160 + AHT21 + DS3231) ---
#define PIN_I2C_SDA 8
#define PIN_I2C_SCL 9

// --- ESP-NOW (uses built-in WiFi, no external hardware needed) ---
// No additional pins required

// --- Wind and Rain Sensors ---
#define PIN_RAIN_GAUGE 4 // KY-003 rain gauge (magnetic pulse)
#define PIN_ANEMOMETER 5 // KY-003 anemometer (magnetic pulse)

// --- Rain Gauge Constants ---
#define RAIN_MM_PER_PULSE 0.20413f // mm of rain per pulse

// --- Anemometer Constants ---
#define ANEMO_RADIUS 0.08f                               // radius in meters
#define ANEMO_FACTOR (2.0f * 3.14159265f * ANEMO_RADIUS) // circumference

// --- Deep Sleep ---
#define SLEEP_DURATION_US (5ULL * 60ULL * 1000000ULL) // 5 minutes in microseconds

// --- Battery ---
// ADC pin for battery voltage reading (voltage divider)
// Adjust according to actual hardware
#define PIN_BATTERY_ADC 6
#define BATTERY_DIVIDER 2.0f // Voltage divider factor

// --- Anemometer Measurement Time ---
#define ANEMO_MEASURE_MS 3000 // 3 seconds of pulse counting
