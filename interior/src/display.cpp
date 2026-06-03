#include "display.h"
#include "config.h"
#include "spi_manager.h"
#include <Arduino.h>

// --- U8g2 Constructor ---
// ST7565 ERC12864 ALT, 4-wire HW SPI, full buffer (1KB RAM)
// We use the constructor with the SPI bus already initialized by spi_manager
static U8G2_ST7565_ERC12864_ALT_F_4W_HW_SPI u8g2(
    U8G2_R0,          // Rotation: normal
    PIN_DISP_CS,      // CS = 10
    PIN_DISP_DC,      // DC = 9
    PIN_DISP_RST      // RST = 8
);

static DisplayScreen currentScreen = SCREEN_SUMMARY;
static bool backlightOn = true;

// --- Custom Icons (XBM 8x8) ---
static const uint8_t icon_temp[] = {0x08, 0x14, 0x14, 0x14, 0x22, 0x5D, 0x41, 0x3E};
static const uint8_t icon_drop[] = {0x08, 0x08, 0x14, 0x22, 0x41, 0x41, 0x41, 0x3E};
static const uint8_t icon_wind[] = {0x00, 0x7E, 0x01, 0x7E, 0x80, 0x7E, 0x01, 0x00};
static const uint8_t icon_wifi[] = {0x3C, 0x42, 0x18, 0x24, 0x00, 0x18, 0x18, 0x00};

void display_init() {
    // Backlight
    pinMode(PIN_DISP_BL, OUTPUT);
    digitalWrite(PIN_DISP_BL, HIGH);

    // Initialize U8g2 (will use SPI already configured by spi_manager)
    spi_acquire();
    u8g2.begin();
    u8g2.setContrast(DISP_CONTRAST);
    u8g2.setFont(u8g2_font_6x10_tf);

    // Welcome Screen
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.drawStr(10, 24, "Meteo Station");
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(20, 42, "Initializing...");
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.sendBuffer();
    spi_release();

    Serial.println(F("[DISPLAY] Initialized OK"));
}

// --- Draw Summary Screen ---
static void draw_summary(const DisplayData& d) {
    u8g2.setFont(u8g2_font_6x10_tf);

    // Top line: time and WiFi
    u8g2.drawStr(0, 9, d.timeStr.c_str());
    if (d.wifiConnected) {
        u8g2.drawXBM(118, 1, 8, 8, icon_wifi);
    }
    u8g2.drawHLine(0, 11, 128);

    // Indoor/Outdoor Temperature
    u8g2.drawXBM(0, 14, 8, 8, icon_temp);
    char buf[32];
    snprintf(buf, sizeof(buf), "In:%.1fC Out:%.1fC", d.indoorTemp, d.outdoorTemp);
    u8g2.drawStr(10, 23, buf);

    // Humidity
    u8g2.drawXBM(0, 26, 8, 8, icon_drop);
    snprintf(buf, sizeof(buf), "In:%.0f%% Out:%.0f%%", d.indoorHum, d.outdoorHum);
    u8g2.drawStr(10, 35, buf);

    // Air Quality
    snprintf(buf, sizeof(buf), "CO2:%uppm VOC:%uppb", d.eco2, d.tvoc);
    u8g2.drawStr(0, 47, buf);

    // Wind and Rain
    u8g2.drawXBM(0, 50, 8, 8, icon_wind);
    snprintf(buf, sizeof(buf), "%.1fm/s", d.windSpeed);
    u8g2.drawStr(10, 59, buf);
    u8g2.drawXBM(60, 50, 8, 8, icon_drop);
    snprintf(buf, sizeof(buf), "%.1fmm", d.rainfall);
    u8g2.drawStr(70, 59, buf);

    // Stale indicator
    if (d.outdoorStale) {
        u8g2.drawStr(100, 59, "[!]");
    }
}

// --- Draw Indoor Screen ---
static void draw_indoor(const DisplayData& d) {
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.drawStr(0, 12, "Indoor");
    u8g2.drawHLine(0, 15, 128);

    u8g2.setFont(u8g2_font_helvB14_tf);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", d.indoorTemp);
    u8g2.drawStr(10, 38, buf);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(70, 38, "\xb0""C");

    u8g2.setFont(u8g2_font_helvB14_tf);
    snprintf(buf, sizeof(buf), "%.0f", d.indoorHum);
    u8g2.drawStr(10, 60, buf);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(70, 60, "%RH");
}

// --- Draw Outdoor Screen ---
static void draw_outdoor(const DisplayData& d) {
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.drawStr(0, 12, "Outdoor");
    if (d.outdoorStale) {
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(70, 12, "(stale)");
    }
    u8g2.drawHLine(0, 15, 128);

    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];

    snprintf(buf, sizeof(buf), "Temp: %.1f C", d.outdoorTemp);
    u8g2.drawStr(0, 27, buf);

    snprintf(buf, sizeof(buf), "Hum:  %.0f %%", d.outdoorHum);
    u8g2.drawStr(0, 38, buf);

    snprintf(buf, sizeof(buf), "CO2:  %u ppm", d.eco2);
    u8g2.drawStr(0, 49, buf);

    snprintf(buf, sizeof(buf), "Wind: %.1f m/s  Rain: %.1fmm", d.windSpeed, d.rainfall);
    u8g2.drawStr(0, 60, buf);
}

// --- Draw Status Screen ---
static void draw_status(const DisplayData& d) {
    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.drawStr(0, 12, "Status");
    u8g2.drawHLine(0, 15, 128);

    u8g2.setFont(u8g2_font_6x10_tf);
    char buf[32];

    snprintf(buf, sizeof(buf), "WiFi: %s", d.wifiConnected ? "OK" : "DISC");
    u8g2.drawStr(0, 27, buf);

    if (d.wifiConnected) {
        snprintf(buf, sizeof(buf), "IP: %s", d.ipAddress.c_str());
        u8g2.drawStr(0, 38, buf);
    }

    snprintf(buf, sizeof(buf), "SD: %s  Radio: %s",
             d.sdOk ? "OK" : "ERR", d.radioOk ? "OK" : "ERR");
    u8g2.drawStr(0, 49, buf);

    uint32_t h = d.uptime / 3600;
    uint32_t m = (d.uptime % 3600) / 60;
    snprintf(buf, sizeof(buf), "Bat: %umV  Up: %uh%02um", d.batteryMv, h, m);
    u8g2.drawStr(0, 60, buf);
}

void display_update(const DisplayData& data) {
    spi_acquire();

    u8g2.clearBuffer();

    switch (currentScreen) {
        case SCREEN_SUMMARY:  draw_summary(data); break;
        case SCREEN_INDOOR:   draw_indoor(data);  break;
        case SCREEN_OUTDOOR:  draw_outdoor(data); break;
        case SCREEN_STATUS:   draw_status(data);  break;
        default:              draw_summary(data); break;
    }

    // Page indicator (dots at the bottom right)
    for (int i = 0; i < SCREEN_COUNT; i++) {
        if (i == currentScreen) {
            u8g2.drawDisc(110 + i * 5, 62, 2);
        } else {
            u8g2.drawCircle(110 + i * 5, 62, 2);
        }
    }

    u8g2.sendBuffer();
    spi_release();
}

void display_next_screen() {
    currentScreen = static_cast<DisplayScreen>((currentScreen + 1) % SCREEN_COUNT);
}

void display_prev_screen() {
    currentScreen = static_cast<DisplayScreen>((currentScreen - 1 + SCREEN_COUNT) % SCREEN_COUNT);
}

void display_set_screen(DisplayScreen screen) {
    if (screen < SCREEN_COUNT) {
        currentScreen = screen;
    }
}

DisplayScreen display_get_screen() {
    return currentScreen;
}

void display_set_backlight(bool on) {
    backlightOn = on;
    digitalWrite(PIN_DISP_BL, on ? HIGH : LOW);
}
