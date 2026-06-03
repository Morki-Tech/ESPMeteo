#pragma once
#include <FastLED.h>

// Lighting modes
enum NeopixelMode {
    NEO_OFF = 0,
    NEO_TEMPERATURE,     // Blue→red gradient based on temperature
    NEO_AIR_QUALITY,     // Green→yellow→red based on eCO2
    NEO_RAIN,            // Blue rain droplet effect
    NEO_MODE_COUNT
};

void neopixel_init();
void neopixel_update(float temperature, uint16_t eco2, float rainfall);
void neopixel_set_mode(NeopixelMode mode);
NeopixelMode neopixel_get_mode();
void neopixel_next_mode();
