// ESP32 + Potentiometer → Car ESC (neutral = 1500 us)
// Requires: ESP32 boards package, ESP32Servo library
// Wiring:
//  ESC: GND -> ESP32 GND, +5V(BEC) -> ESP32 5V/Vin, SIG -> GPIO18
//  Pot: 3.3V -> pot end, GND -> other end, wiper -> GPIO34 (ADC1)

#include <Arduino.h>
#include <ESP32Servo.h>

/// --------- USER SETTINGS ----------
const int ESC_PIN        = 18;     // ESC signal out
const int POT_ADC_PIN    = 34;     // potentiometer wiper (ADC1)
const int ARM_MS         = 4000;   // neutral hold to arm ESC (ms)
const int DEAD_PCT       = 3;      // neutral deadband ±% around 0 (e.g., 3%)
const int RAMP_US_PER_T  = 4;      // µs per loop step toward target (smooth)
const int LOOP_DELAY_MS  = 2;      // loop delay (ms)
const bool INVERT_POT    = false;  // set true if your pot is reversed
/// ----------------------------------

const int MIN_US = 1000;
const int NEUTRAL_US = 1500;
const int MAX_US = 2000;

Servo esc;

// Simple moving average for ADC smoothing
const int AVG_N = 8;
int avgBuf[AVG_N];
int avgIdx = 0;
bool filled = false;

int currentUs = NEUTRAL_US;
int targetUs  = NEUTRAL_US;

int readADCaveraged() {
  int sum = 0;
  int count = filled ? AVG_N : (avgIdx + 1);
  for (int i = 0; i < count; ++i) sum += avgBuf[i];
  return sum / count;
}

// Map ADC (0..4095) to -100..100 with center ~2048 == 0%
int adcToPercent(int raw) {
  if (INVERT_POT) raw = 4095 - raw;

  // Normalize around center
  const int center = 2048;
  int delta = raw - center;

  // Scale: full span ±2048 -> ±100%
  float pct = (delta / 2048.0f) * 100.0f;

  // Clamp
  if (pct > 100) pct = 100;
  if (pct < -100) pct = -100;

  // Deadband around neutral
  if (fabs(pct) < DEAD_PCT) pct = 0;

  return (int)roundf(pct);
}

int pctToUs(int pct) {
  if (pct < -100) pct = -100;
  if (pct >  100) pct =  100;
  if (pct == 0) return NEUTRAL_US;

  if (pct > 0) {
    // 0..+100% -> 1500..2000
    return NEUTRAL_US + (int)((MAX_US - NEUTRAL_US) * (pct / 100.0f));
  } else {
    // 0..-100% -> 1500..1000
    return NEUTRAL_US + (int)((MIN_US - NEUTRAL_US) * (pct / 100.0f));
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // Prepare ADC
  analogReadResolution(12);  // 0..4095 (ADC1)
  // Optionally: analogSetPinAttenuation(POT_ADC_PIN, ADC_11db);

  // Attach ESC with explicit pulse range
  esc.attach(ESC_PIN, MIN_US, MAX_US);

  // Arm at neutral
  esc.writeMicroseconds(NEUTRAL_US);
  Serial.println("\nArming ESC at neutral (1500 us)...");
  delay(ARM_MS);

  // Prime the moving average buffer with current readings
  for (int i = 0; i < AVG_N; ++i) {
    avgBuf[i] = analogRead(POT_ADC_PIN);
    delay(2);
  }
  avgIdx = AVG_N - 1;
  filled = true;

  currentUs = NEUTRAL_US;
  targetUs  = NEUTRAL_US;

  Serial.println("Ready. Turn the potentiometer to drive throttle.");
  Serial.println("Center ≈ neutral, clockwise = forward (unless INVERT_POT = true).");
}

void loop() {
  // Update moving average
  avgIdx = (avgIdx + 1) % AVG_N;
  avgBuf[avgIdx] = analogRead(POT_ADC_PIN);

  int rawAvg = readADCaveraged();
  int pct    = adcToPercent(rawAvg);
  targetUs   = pctToUs(pct);

  // Ramp output gently to target
  if (currentUs < targetUs) {
    currentUs += RAMP_US_PER_T;
    if (currentUs > targetUs) currentUs = targetUs;
  } else if (currentUs > targetUs) {
    currentUs -= RAMP_US_PER_T;
    if (currentUs < targetUs) currentUs = targetUs;
  }

  esc.writeMicroseconds(currentUs);

  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 100) {
    Serial.printf("ADC:%4d  pct:%4d  us:%4d\n", rawAvg, pct, currentUs);
    lastPrint = millis();
  }

  delay(LOOP_DELAY_MS);
}
