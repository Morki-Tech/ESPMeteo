#pragma once

// Callback types
typedef void (*EncoderRotateCallback)(int direction);  // +1 = CW, -1 = CCW
typedef void (*EncoderPressCallback)();

void encoder_init();
void encoder_update();  // Call in loop() to process events
void encoder_on_rotate(EncoderRotateCallback cb);
void encoder_on_press(EncoderPressCallback cb);
