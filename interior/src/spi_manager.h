#pragma once
#include <SPI.h>
#include <freertos/semphr.h>

// Global mutex for thread-safe SPI access
extern SemaphoreHandle_t spiMutex;

// Initializes all CS pins to HIGH and configures the SPI bus
void spi_manager_init();

// Acquires the SPI mutex (blocking, 1s timeout)
bool spi_acquire(TickType_t timeout = pdMS_TO_TICKS(1000));

// Releases the SPI mutex
void spi_release();
