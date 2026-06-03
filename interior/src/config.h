#pragma once

// =====================================================
// Weather Station — Interior Unit — Configuration
// =====================================================

// --- WiFi ---
#define WIFI_SSID          "YOUR_WIFI_SSID"
#define WIFI_PASSWORD      "YOUR_WIFI_PASSWORD"

// --- NTP ---
#define NTP_SERVER         "pool.ntp.org"
#define NTP_GMT_OFFSET     3600       // CET = UTC+1
#define NTP_DAYLIGHT       3600       // CEST = UTC+2 (daylight savings)

// --- ST7565 Display (SPI) ---
#define PIN_DISP_CS        10
#define PIN_DISP_DC        9
#define PIN_DISP_RST       8
#define PIN_DISP_BL        7
#define DISP_CONTRAST      40       // 0-255, adjust according to screen

// --- Shared SPI Bus (Display + SD) ---
#define PIN_SPI_CLK        12
#define PIN_SPI_MOSI       11
#define PIN_SPI_MISO       14

// --- Micro SD (SPI) ---
#define PIN_SD_CS          13

// --- KY-040 Encoder ---
#define PIN_ENC_CLK        5
#define PIN_ENC_DT         6
#define PIN_ENC_SW         17

// --- NeoPixel WS2812B ---
#define PIN_NEOPIXEL       48
#define NEOPIXEL_COUNT     16

// --- DHT11 ---
#define PIN_DHT            4
#define DHT_TYPE           DHT11

// --- Intervals (ms) ---
#define DHT_READ_INTERVAL  2000      // DHT11 requires at least 2s between reads
#define DISPLAY_INTERVAL   500       // Display refresh rate
#define NEOPIXEL_INTERVAL  50        // LED animation refresh rate
#define RADIO_STALE_MS     600000    // 10 minutes without data = stale
#define WEB_UPDATE_MS      10000     // Web polling interval

// --- History ---
#define HISTORY_SIZE       288       // 24h * 60min / 5min = 288 records
