#include <Adafruit_NeoPixel.h>

#define LED_PIN    2
#define NUM_LEDS   400 

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  // Move the 10-light window across the strip
  for (int i = 0; i < NUM_LEDS; i++) {
    
    // 1. Turn on a block of 10 LEDs
    for (int j = 0; j < 10; j++) {
      if (i + j < NUM_LEDS) { // Ensure we don't go past the end of the strip
        strip.setPixelColor(i + j, strip.Color(0, 0, 180)); 
      }
    }
    
    strip.show();
    delay(150); // Walkable speed

    // 2. Turn off the block (or just the trailing pixel) to create movement
    // Cleaning the whole block here prepares it for the next 'i' increment
    for (int j = 0; j < 10; j++) {
      if (i + j < NUM_LEDS) {
        strip.setPixelColor(i + j, strip.Color(0, 0, 0)); 
      }
    }
    delay(30);
  }
}