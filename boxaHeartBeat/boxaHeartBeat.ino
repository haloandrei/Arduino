#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;

// Heart frame (12x8) for the UNO R4 WiFi LED matrix
// Source: Arduino examples – draws a heart shape
const uint32_t heart_on[] = {
  0x3184a444,
  0x44042081,
  0x100a0040
};

// All pixels off
const uint32_t heart_off[] = {
  0x00000000,
  0x00000000,
  0x00000000
};

// Helper: set sound level and heart state together
void setBeat(uint8_t level) {
  analogWrite(9, level);

  if (level == 0) {
    matrix.loadFrame(heart_off);  // heart off on silence
  } else {
    matrix.loadFrame(heart_on);   // heart on when "beating"
  }
}

void setup() {
  pinMode(9, OUTPUT);

  matrix.begin();         // init LED matrix
  matrix.loadFrame(heart_off); // start with heart off
}

void loop() {
  // baseline small sound + faint heart
  setBeat(1);

  // --- first part: 3 slow "breathing" beats ---
  for (int x = 1; x <= 3; x++) {
    // rise
    for (int i = 1; i <= 10; i++) {
      setBeat(i);
      delay(21);
    }
    // fall
    for (int i = 10; i >= 1; i--) {
      setBeat(i);
      delay(10);
    }
  }

  // --- second part: 5 quick heart pulses ---
  for (int d = 1; d <= 5; d++) {
    // silent + heart off
    setBeat(0);
    delay(100);

    // short strong beat + heart on
    setBeat(d * 2);  // gets a bit “stronger” each pulse
    delay(50);
  }
}
