#pragma once
#include "../../shared/packet.h"

bool radio_init();
bool radio_is_ok();
bool radio_available();         // Is there a new packet available?
bool radio_read(ExtPacket& pkt); // Read the last valid packet
unsigned long radio_last_received_ms();
