#pragma once
#include <stdint.h>
#include <stddef.h>

bool radio_init();
bool radio_send(const uint8_t* data, size_t len);
void radio_sleep();
