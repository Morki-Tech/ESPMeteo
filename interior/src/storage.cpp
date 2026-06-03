#include "storage.h"
#include "config.h"
#include "spi_manager.h"
#include <SD.h>
#include <Arduino.h>
#include <time.h>

static bool sdOk = false;

// Circular buffer in RAM for recent data
static HistoryRecord historyBuffer[HISTORY_SIZE];
static int historyHead = 0;
static int historyCount = 0;

bool storage_init() {
    spi_acquire();
    sdOk = SD.begin(PIN_SD_CS);
    spi_release();

    if (!sdOk) {
        Serial.println(F("[STORAGE] SD card not found!"));
        return false;
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("[STORAGE] SD card OK, %lluMB\n", cardSize);

    // Create data directory if it doesn't exist
    spi_acquire();
    if (!SD.exists("/meteo")) {
        SD.mkdir("/meteo");
    }
    spi_release();

    return true;
}

bool storage_log(const HistoryRecord& record) {
    // --- 1. Save to circular buffer (always) ---
    historyBuffer[historyHead] = record;
    historyHead = (historyHead + 1) % HISTORY_SIZE;
    if (historyCount < HISTORY_SIZE) historyCount++;

    // --- 2. Save to SD card (if available) ---
    if (!sdOk) return false;

    // Daily file name: /meteo/2026-05-09.csv
    struct tm timeinfo;
    time_t t = (time_t)record.timestamp;
    localtime_r(&t, &timeinfo);

    char filename[32];
    snprintf(filename, sizeof(filename), "/meteo/%04d-%02d-%02d.csv",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);

    spi_acquire();
    bool isNew = !SD.exists(filename);
    File file = SD.open(filename, FILE_APPEND);
    spi_release();

    if (!file) {
        Serial.printf("[STORAGE] Error opening %s\n", filename);
        return false;
    }

    // Write header if this is a new file
    if (isNew) {
        file.println(F("timestamp,temp_int,hum_int,temp_ext,hum_ext,eco2,tvoc,wind,rain,batt"));
    }

    // Write data
    char line[128];
    snprintf(line, sizeof(line), "%u,%.1f,%.1f,%.1f,%.1f,%u,%u,%.2f,%.2f,%u",
             record.timestamp,
             record.indoorTemp, record.indoorHum,
             record.outdoorTemp, record.outdoorHum,
             record.eco2, record.tvoc,
             record.windSpeed, record.rainfall,
             record.batteryMv);

    spi_acquire();
    file.println(line);
    file.close();
    spi_release();

    return true;
}

int storage_get_history(HistoryRecord* buffer, int maxRecords, int hours) {
    // Return records from circular buffer within hour range
    time_t now;
    time(&now);
    uint32_t cutoff = (uint32_t)now - (hours * 3600);

    int count = 0;
    int idx = (historyHead - historyCount + HISTORY_SIZE) % HISTORY_SIZE;

    for (int i = 0; i < historyCount && count < maxRecords; i++) {
        if (historyBuffer[idx].timestamp >= cutoff) {
            buffer[count++] = historyBuffer[idx];
        }
        idx = (idx + 1) % HISTORY_SIZE;
    }

    return count;
}

int storage_get_history_count() {
    return historyCount;
}

bool storage_is_ok() {
    return sdOk;
}
