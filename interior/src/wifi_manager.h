#pragma once
#include <Arduino.h>

void wifi_init();
bool wifi_is_connected();
String wifi_get_ip();
String wifi_get_time();
uint32_t wifi_get_epoch();
void wifi_update(); // reconnect
