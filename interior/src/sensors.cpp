#include "sensors.h"
#include "config.h"
#include <DHT.h>
#include <Arduino.h>

static DHT dht(PIN_DHT, DHT_TYPE);
static unsigned long lastRead = 0;
static IndoorData lastData = {0, 0, false};

void sensors_init() {
    dht.begin();
    Serial.println(F("[SENSORS] DHT11 initialized"));
}

IndoorData sensors_read() {
    unsigned long now = millis();

    // DHT11 requires at least 2s between reads
    if (now - lastRead < DHT_READ_INTERVAL && lastData.valid) {
        return lastData;
    }

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
        Serial.println(F("[SENSORS] DHT11 invalid read"));
        // Return last valid read if available
        return lastData;
    }

    lastData.temperature = t;
    lastData.humidity    = h;
    lastData.valid       = true;
    lastRead = now;

    return lastData;
}
