#include "web_server.h"
#include "config.h"
#include "sensors.h"
#include "storage.h"
#include "radio.h"
#include "wifi_manager.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

static AsyncWebServer server(80);

// Shared outdoor data (updated from main)
static ExtPacket extData = {};
static bool extDataValid = false;
static unsigned long extDataAge = 0;

// Function to update outdoor data (called from main.cpp)
void web_server_update_ext_data(const ExtPacket& pkt) {
    extData = pkt;
    extDataValid = true;
    extDataAge = millis();
}

void web_server_init() {
    // Initialize LittleFS to serve web files
    if (!LittleFS.begin(true)) {
        Serial.println(F("[WEB] LittleFS mount FAILED!"));
    } else {
        Serial.println(F("[WEB] LittleFS OK"));
    }

    // --- API: Current data ---
    server.on("/api/current", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;

        // Indoor
        IndoorData indoor = sensors_read();
        doc["indoor"]["temp"] = round(indoor.temperature * 10) / 10.0;
        doc["indoor"]["hum"]  = round(indoor.humidity * 10) / 10.0;
        doc["indoor"]["valid"] = indoor.valid;

        // Outdoor
        doc["outdoor"]["temp"]     = round(extData.temperature * 10) / 10.0;
        doc["outdoor"]["hum"]      = round(extData.humidity * 10) / 10.0;
        doc["outdoor"]["eco2"]     = extData.eco2;
        doc["outdoor"]["tvoc"]     = extData.tvoc;
        doc["outdoor"]["wind"]     = round(extData.windSpeed * 100) / 100.0;
        doc["outdoor"]["rain"]     = round(extData.rainfall * 100) / 100.0;
        doc["outdoor"]["battery"]  = extData.batteryMv;
        doc["outdoor"]["valid"]    = extDataValid;
        doc["outdoor"]["stale"]    = extDataValid && (millis() - extDataAge > RADIO_STALE_MS);

        doc["time"] = wifi_get_time();
        doc["epoch"] = wifi_get_epoch();

        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    // --- API: History ---
    server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *request) {
        int hours = 24;
        if (request->hasParam("hours")) {
            hours = request->getParam("hours")->value().toInt();
            if (hours < 1) hours = 1;
            if (hours > 168) hours = 168;  // Max 1 week
        }

        // Get records from buffer
        HistoryRecord records[HISTORY_SIZE];
        int count = storage_get_history(records, HISTORY_SIZE, hours);

        // Construct JSON array
        JsonDocument doc;
        JsonArray arr = doc["data"].to<JsonArray>();

        for (int i = 0; i < count; i++) {
            JsonObject obj = arr.add<JsonObject>();
            obj["t"]    = records[i].timestamp;
            obj["ti"]   = round(records[i].indoorTemp * 10) / 10.0;
            obj["hi"]   = round(records[i].indoorHum * 10) / 10.0;
            obj["to"]   = round(records[i].outdoorTemp * 10) / 10.0;
            obj["ho"]   = round(records[i].outdoorHum * 10) / 10.0;
            obj["co2"]  = records[i].eco2;
            obj["voc"]  = records[i].tvoc;
            obj["w"]    = round(records[i].windSpeed * 100) / 100.0;
            obj["r"]    = round(records[i].rainfall * 100) / 100.0;
            obj["bat"]  = records[i].batteryMv;
        }
        doc["count"] = count;

        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    // --- API: System status ---
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["wifi"]   = wifi_is_connected();
        doc["ip"]     = wifi_get_ip();
        doc["sd"]     = storage_is_ok();
        doc["radio"]  = radio_is_ok();
        doc["uptime"] = millis() / 1000;
        doc["heap"]   = ESP.getFreeHeap();
        doc["time"]   = wifi_get_time();

        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    // --- Serve static files from LittleFS ---
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // --- 404 handler ---
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not Found");
    });

    server.begin();
    Serial.printf("[WEB] Server started on port 80\n");
}
