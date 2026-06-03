#include "spi_manager.h"
#include "config.h"
#include <Arduino.h>

SemaphoreHandle_t spiMutex = nullptr;

void spi_manager_init() {
    // --- CRITICAL: Create mutex BEFORE any SPI operation ---
    spiMutex = xSemaphoreCreateMutex();
    if (spiMutex == nullptr) {
        Serial.println(F("[SPI] ERROR: Could not create mutex!"));
    }

    // --- CRITICAL: Set ALL CS pins to HIGH BEFORE initializing SPI ---
    // This prevents devices from "listening" to SPI traffic not intended for them
    pinMode(PIN_DISP_CS, OUTPUT);
    digitalWrite(PIN_DISP_CS, HIGH);

    pinMode(PIN_SD_CS, OUTPUT);
    digitalWrite(PIN_SD_CS, HIGH);

    // --- Initialize SPI bus with shared pins ---
    SPI.begin(PIN_SPI_CLK, PIN_SPI_MISO, PIN_SPI_MOSI);

    Serial.println(F("[SPI] Bus initialized, all CS pins HIGH, mutex created"));
}

bool spi_acquire(TickType_t timeout) {
    if (spiMutex == nullptr) return true;  // Fallback if no mutex exists
    return xSemaphoreTake(spiMutex, timeout) == pdTRUE;
}

void spi_release() {
    if (spiMutex == nullptr) return;
    xSemaphoreGive(spiMutex);
}
