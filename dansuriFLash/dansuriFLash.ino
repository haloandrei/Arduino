#include <Adafruit_NeoPixel.h>
#include <math.h>

#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

// ---------------- NeoPixel ----------------
#define LED_PIN    2
#define NUM_LEDS   256   // 16×16 matrix
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// duration of one full pink→blue→pink cycle (in ms)
const unsigned long DUR_MS = 2000;

// endpoint colors
const uint8_t PINK_R = 255, PINK_G =  20, PINK_B = 147;
const uint8_t BLUE_R = 173, BLUE_G = 216, BLUE_B = 230;

unsigned long startTime;

void setup() {
  strip.begin();
  strip.show();      // initialize all pixels to “off”
  startTime = millis();
}

void loop() {
  // how many ms since we started
  unsigned long elapsed = millis() - startTime;
  // map elapsed → [0 … 1) every DUR_MS
  float phase = float(elapsed % DUR_MS) / float(DUR_MS);
  // full sine wave from 0→2π
  float angle = phase * 2.0f * M_PI;
  // u goes smoothly from 0→1→0
  float u = (sin(angle) + 1.0f) * 0.5f;

  // interpolate each channel: C = C_pink*(1-u) + C_blue*u
  uint8_t r = uint8_t(PINK_R * (1.0f - u) + BLUE_R * u);
  uint8_t g = uint8_t(PINK_G * (1.0f - u) + BLUE_G * u);
  uint8_t b = uint8_t(PINK_B * (1.0f - u) + BLUE_B * u);

  // set entire matrix at once
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();

  delay(20);
}
