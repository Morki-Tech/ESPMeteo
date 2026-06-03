#pragma once

struct IndoorData {
    float temperature;
    float humidity;
    bool  valid;
};

void sensors_init();
IndoorData sensors_read();
