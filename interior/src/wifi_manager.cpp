#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>
#include <time.h>

static unsigned long lastReconnectAttempt = 0;
static const unsigned long RECONNECT_INTERVAL = 30000; // 30s between attempts

void wifi_init() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  Serial.printf("[WIFI] Connecting to %s...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait for connection (10s timeout)
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(250);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WIFI] Connected! IP: %s\n",
                  WiFi.localIP().toString().c_str());

    // Configure NTP
    configTime(NTP_GMT_OFFSET, NTP_DAYLIGHT, NTP_SERVER);
    Serial.println(F("[WIFI] Initialized"));

    // Wait for the first synchronization
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
      Serial.printf("[WIFI] Time: %02d:%02d:%02d\n", timeinfo.tm_hour,
                    timeinfo.tm_min, timeinfo.tm_sec);
    }
  } else {
    Serial.println(F("\n[WIFI] Could Not Connect!"));
  }
}

bool wifi_is_connected() { return WiFi.status() == WL_CONNECTED; }

String wifi_get_ip() {
  if (WiFi.status() != WL_CONNECTED)
    return "---";
  return WiFi.localIP().toString();
}

String wifi_get_time() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 100)) {
    return "--:--";
  }
  char buf[9];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", timeinfo.tm_hour,
           timeinfo.tm_min, timeinfo.tm_sec);
  return String(buf);
}

uint32_t wifi_get_epoch() {
  time_t now;
  time(&now);
  return (uint32_t)now;
}

void wifi_update() {
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
      lastReconnectAttempt = now;
      Serial.println(F("[WIFI] Reconnecting..."));
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  }
}
