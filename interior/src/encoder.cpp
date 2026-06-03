#include "encoder.h"
#include "config.h"
#include <Arduino.h>

// --- Internal State ---
static volatile int  encoderDelta = 0;
static volatile bool buttonPressed = false;

static EncoderRotateCallback rotateCb = nullptr;
static EncoderPressCallback  pressCb  = nullptr;

static uint8_t lastState = 0;
static unsigned long lastButtonTime = 0;
static const unsigned long DEBOUNCE_MS = 200;

// --- ISR for Rotary Encoder ---
static void IRAM_ATTR encoderISR() {
    uint8_t clk = digitalRead(PIN_ENC_CLK);
    uint8_t dt  = digitalRead(PIN_ENC_DT);
    uint8_t state = (clk << 1) | dt;

    // Valid transition: detect direction
    // CW Sequence:  00 → 01 → 11 → 10
    // CCW Sequence: 00 → 10 → 11 → 01
    if (lastState == 0b00) {
        if (state == 0b01) encoderDelta++;
        else if (state == 0b10) encoderDelta--;
    }
    lastState = state;
}

// --- ISR for Button ---
static void IRAM_ATTR buttonISR() {
    unsigned long now = millis();
    if (now - lastButtonTime > DEBOUNCE_MS) {
        buttonPressed = true;
        lastButtonTime = now;
    }
}

void encoder_init() {
    pinMode(PIN_ENC_CLK, INPUT_PULLUP);
    pinMode(PIN_ENC_DT, INPUT_PULLUP);
    pinMode(PIN_ENC_SW, INPUT_PULLUP);

    lastState = (digitalRead(PIN_ENC_CLK) << 1) | digitalRead(PIN_ENC_DT);

    attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_DT), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_SW), buttonISR, FALLING);

    Serial.println(F("[ENCODER] Initialized OK"));
}

void encoder_update() {
    // Process accumulated rotation
    noInterrupts();
    int delta = encoderDelta;
    encoderDelta = 0;
    bool pressed = buttonPressed;
    buttonPressed = false;
    interrupts();

    if (delta != 0 && rotateCb) {
        rotateCb(delta > 0 ? 1 : -1);
    }

    if (pressed && pressCb) {
        pressCb();
    }
}

void encoder_on_rotate(EncoderRotateCallback cb) {
    rotateCb = cb;
}

void encoder_on_press(EncoderPressCallback cb) {
    pressCb = cb;
}
