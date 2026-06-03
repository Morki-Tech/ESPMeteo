#include "neopixel.h"
#include "config.h"
#include <Arduino.h>

static CRGB leds[NEOPIXEL_COUNT];
static NeopixelMode currentMode = NEO_TEMPERATURE;
static uint8_t animFrame = 0;

void neopixel_init() {
    FastLED.addLeds<WS2812B, PIN_NEOPIXEL, GRB>(leds, NEOPIXEL_COUNT);
    FastLED.setBrightness(40);  // Moderate brightness
    FastLED.clear();
    FastLED.show();
    Serial.println(F("[NEOPIXEL] Initialized OK"));
}

// --- Temperature Mode: gradient from blue (cold) → red (hot) ---
static void mode_temperature(float temp) {
    // Map -10°C..40°C to hue 160 (blue) → 0 (red)
    float t = constrain(temp, -10.0f, 40.0f);
    uint8_t hue = map((int)(t * 10), -100, 400, 160, 0);

    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        // Slight gradient along the ring
        uint8_t h = hue + (i * 2);
        leds[i] = CHSV(h, 240, 200);
    }
}

// --- Air Quality Mode: green → yellow → red based on eCO2 ---
static void mode_air_quality(uint16_t eco2) {
    // eCO2 ranges:
    // < 600 ppm = Excellent (green)
    // 600-1000   = Good (yellow-green)
    // 1000-1500  = Moderate (yellow)
    // 1500-2000  = Bad (orange)
    // > 2000     = Very bad (red)
    uint8_t hue;
    if (eco2 < 600) {
        hue = 96;  // Green
    } else if (eco2 < 1500) {
        hue = map(eco2, 600, 1500, 96, 32);  // Green → Yellow
    } else {
        hue = map(constrain(eco2, 1500, 3000), 1500, 3000, 32, 0);  // Yellow → Red
    }

    // Number of active LEDs proportional to level
    int litCount = map(constrain(eco2, 400, 3000), 400, 3000, 1, NEOPIXEL_COUNT);

    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        if (i < litCount) {
            leds[i] = CHSV(hue, 240, 200);
        } else {
            leds[i] = CRGB::Black;
        }
    }
}

// --- Rain Mode: falling blue droplet effect ---
static void mode_rain(float rainfall) {
    // General fade out
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
        leds[i].fadeToBlackBy(40);
    }

    // Generate random "droplets", more frequent with more rain
    uint8_t chance = rainfall > 0.0f ? constrain((int)(rainfall * 20), 10, 200) : 5;
    if (random8() < chance) {
        int pos = random8(NEOPIXEL_COUNT);
        leds[pos] = CHSV(160 + random8(20), 200, 255);  // Blue with variation
    }

    // Rotate the effect
    animFrame++;
    if (animFrame % 3 == 0) {
        CRGB temp = leds[NEOPIXEL_COUNT - 1];
        for (int i = NEOPIXEL_COUNT - 1; i > 0; i--) {
            leds[i] = leds[i - 1];
        }
        leds[0] = temp;
    }
}

void neopixel_update(float temperature, uint16_t eco2, float rainfall) {
    switch (currentMode) {
        case NEO_TEMPERATURE:
            mode_temperature(temperature);
            break;
        case NEO_AIR_QUALITY:
            mode_air_quality(eco2);
            break;
        case NEO_RAIN:
            mode_rain(rainfall);
            break;
        case NEO_OFF:
        default:
            FastLED.clear();
            break;
    }
    FastLED.show();
}

void neopixel_set_mode(NeopixelMode mode) {
    currentMode = mode;
    if (mode == NEO_OFF) {
        FastLED.clear();
        FastLED.show();
    }
}

NeopixelMode neopixel_get_mode() {
    return currentMode;
}

void neopixel_next_mode() {
    currentMode = static_cast<NeopixelMode>((currentMode + 1) % NEO_MODE_COUNT);
    animFrame = 0;
    Serial.printf("[NEOPIXEL] Mode: %d\n", currentMode);
}
