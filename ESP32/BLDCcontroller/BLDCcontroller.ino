// ESP32 → car ESC (neutral = 1500 us)
// Wiring:
//   ESC GND  -> ESP32 GND
//   ESC +5V  -> ESP32 5V/Vin  (NOT 3V3)
//   ESC SIG  -> ESP32 GPIO18  (or 19/23/etc.)
// Serial Monitor: 115200 baud, line ending = Newline

#include <Arduino.h>
#include <ESP32Servo.h>

const int ESC_PIN = 18;
const int MIN_US = 1000;
const int NEUTRAL_US = 1500;
const int MAX_US = 2000;
const int ARM_MS = 4000;      // many car ESCs want ~3–5 s at neutral

Servo esc;
int currentUs = NEUTRAL_US;
int targetUs  = NEUTRAL_US;

// Map -100..100% around NEUTRAL
int pctToUs(int pct) {
  if (pct < -100) pct = -100;
  if (pct >  100) pct =  100;
  if (pct == 0) return NEUTRAL_US;
  if (pct > 0)  return NEUTRAL_US + (int)((MAX_US - NEUTRAL_US) * (pct / 100.0f));
  // pct < 0
  return NEUTRAL_US + (int)((MIN_US - NEUTRAL_US) * (pct / 100.0f));
}

void setup() {
  Serial.begin(115200);
  delay(200);

  esc.attach(ESC_PIN, MIN_US, MAX_US);

  // Arm at neutral
  esc.writeMicroseconds(NEUTRAL_US);
  Serial.println("\nArming at neutral (1500 us)...");
  delay(ARM_MS);

  // Start at +30%
  targetUs  = pctToUs(30);
  currentUs = NEUTRAL_US;
  Serial.println("Armed. Starting at +30%.");
}

void loop() {
  // Console: -100..100 (optionally with %)
  if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
    s.trim();
    if (s.endsWith("%")) s.remove(s.length()-1);

    if (s.length()) {
      bool ok = true;
      for (size_t i = 0; i < s.length(); ++i) {
        char c = s[i];
        if (!isDigit(c) && c != '-' && c != '.') { ok = false; break; }
      }
      if (ok) {
        float v = s.toFloat();
        if (v < -100) v = -100;
        if (v >  100) v =  100;
        targetUs = pctToUs((int)roundf(v));
        Serial.printf("Target: %.0f%%  -> %d us\n", v, targetUs);
      } else {
        Serial.println("Enter -100..100 (e.g., -30, 0, 25, 80).");
      }
    }
  }

  // Gentle 1 us ramp to avoid jerks
  if (currentUs < targetUs) currentUs++;
  else if (currentUs > targetUs) currentUs--;
  esc.writeMicroseconds(currentUs);
  delay(2);
}
