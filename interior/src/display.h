#pragma once
#include <U8g2lib.h>

// Menu screens
enum DisplayScreen {
    SCREEN_SUMMARY = 0,   // Summary of all data
    SCREEN_INDOOR,        // Indoor data (DHT11)
    SCREEN_OUTDOOR,       // Outdoor data (radio)
    SCREEN_STATUS,        // System status
    SCREEN_COUNT          // Total number of screens
};

// Data required by the display
struct DisplayData {
    // Interior
    float  indoorTemp;
    float  indoorHum;
    // Exterior
    float  outdoorTemp;
    float  outdoorHum;
    uint16_t eco2;
    uint16_t tvoc;
    float  windSpeed;
    float  rainfall;
    uint16_t batteryMv;
    bool   outdoorStale;    // true if no recent outdoor data is available
    // Status
    bool   wifiConnected;
    bool   sdOk;
    bool   radioOk;
    String ipAddress;
    String timeStr;
    uint32_t uptime;        // in seconds
};

void display_init();
void display_update(const DisplayData& data);
void display_next_screen();
void display_prev_screen();
void display_set_screen(DisplayScreen screen);
DisplayScreen display_get_screen();
void display_set_backlight(bool on);
