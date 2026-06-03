#pragma once
#include "../../shared/packet.h"

// Historical record structure
struct HistoryRecord {
    uint32_t timestamp;
    float    indoorTemp;
    float    indoorHum;
    float    outdoorTemp;
    float    outdoorHum;
    uint16_t eco2;
    uint16_t tvoc;
    float    windSpeed;
    float    rainfall;
    uint16_t batteryMv;
};

bool storage_init();
bool storage_log(const HistoryRecord& record);
int  storage_get_history(HistoryRecord* buffer, int maxRecords, int hours);
int  storage_get_history_count();
bool storage_is_ok();
