#include "sensors.h"
#include "config.h"
#include <Wire.h>
#include <Arduino.h>

// --- Sensor Objects ---
static Adafruit_AHTX0     aht;
static ScioSense_ENS160   ens160;
static RTC_DS3231         rtc;

// --- Pulse Counters (persisted during deep sleep) ---
RTC_DATA_ATTR static uint32_t rainPulseCount = 0;
RTC_DATA_ATTR static uint32_t lastRainPulseCount = 0;

// --- Volatile variables for ISR ---
static volatile uint32_t anemoCounter = 0;

// --- Sensor State ---
static bool _ahtOk  = false;
static bool _ensOk  = false;
static bool _rtcOk  = false;

// --- ISR for Anemometer ---
static void IRAM_ATTR anemoISR() {
    anemoCounter++;
}

// --- ISR for Rain Gauge ---
static void IRAM_ATTR rainISR() {
    rainPulseCount++;
}

void sensors_init() {
    // Initialize I2C
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(100000);  // 100kHz for stability

    // --- AHT21 ---
    _ahtOk = aht.begin(&Wire);
    if (!_ahtOk) {
        Serial.println(F("[SENSORS] AHT21 not found!"));
    } else {
        Serial.println(F("[SENSORS] AHT21 OK"));
    }

    // --- ENS160 (ScioSense API) ---
    ens160.setI2C(PIN_I2C_SDA, PIN_I2C_SCL);
    _ensOk = ens160.begin();
    if (!_ensOk || !ens160.available()) {
        _ensOk = false;
        Serial.println(F("[SENSORS] ENS160 not found!"));
    } else {
        ens160.setMode(ENS160_OPMODE_STD);
        Serial.println(F("[SENSORS] ENS160 OK (STD mode)"));
    }

    // --- DS3231 RTC ---
    _rtcOk = rtc.begin(&Wire);
    if (!_rtcOk) {
        Serial.println(F("[SENSORS] DS3231 not found!"));
    } else {
        if (rtc.lostPower()) {
            Serial.println(F("[SENSORS] DS3231 lost power, adjusting to build time"));
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
        Serial.println(F("[SENSORS] DS3231 OK"));
    }

    // --- Anemometer (interrupt) ---
    pinMode(PIN_ANEMOMETER, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_ANEMOMETER), anemoISR, FALLING);

    // --- Rain Gauge (interrupt) ---
    pinMode(PIN_RAIN_GAUGE, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_RAIN_GAUGE), rainISR, FALLING);
}

uint16_t sensors_read_battery() {
    analogReadResolution(12);
    uint32_t raw = 0;
    for (int i = 0; i < 16; i++) {
        raw += analogReadMilliVolts(PIN_BATTERY_ADC);
    }
    raw /= 16;
    return (uint16_t)(raw * BATTERY_DIVIDER);
}

SensorData sensors_read() {
    SensorData d = {};

    // --- AHT21: Temperature + Humidity ---
    d.ahtOk = _ahtOk;
    if (_ahtOk) {
        sensors_event_t humEvent, tempEvent;
        aht.getEvent(&humEvent, &tempEvent);
        d.temperature = tempEvent.temperature;
        d.humidity    = humEvent.relative_humidity;

        // Compensate ENS160 with real data
        if (_ensOk) {
            ens160.set_envdata(d.temperature, d.humidity);
        }
    }

    // --- ENS160: eCO2 + TVOC (ScioSense API) ---
    d.ensOk = _ensOk;
    if (_ensOk) {
        if (ens160.measure()) {
            d.eco2 = ens160.geteCO2();
            d.tvoc = ens160.getTVOC();
        }
    }

    // --- DS3231: Timestamp ---
    d.rtcOk = _rtcOk;
    if (_rtcOk) {
        DateTime now = rtc.now();
        d.rtcTimestamp = now.unixtime();
    }

    // --- Anemometer: Wind Speed ---
    anemoCounter = 0;
    delay(ANEMO_MEASURE_MS);
    uint32_t pulses = anemoCounter;
    float measureTimeSec = ANEMO_MEASURE_MS / 1000.0f;
    d.windSpeed = (pulses * ANEMO_FACTOR) / measureTimeSec;

    // --- Rain Gauge: Accumulated Rain ---
    uint32_t newPulses = rainPulseCount - lastRainPulseCount;
    d.rainfall = newPulses * RAIN_MM_PER_PULSE;

    // --- Battery ---
    d.batteryMv = sensors_read_battery();

    return d;
}

void sensors_reset_rain() {
    lastRainPulseCount = rainPulseCount;
}
