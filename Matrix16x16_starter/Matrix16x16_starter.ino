
#include <Adafruit_NeoPixel.h>

#define LED_PIN   2
#define NUM_LEDS  400        // 16 × 16 matrix

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);



void setup() {
  strip.begin();
  strip.show(); // clear
  Serial.begin(115200);
}

void loop() {

  for (int j = 0; j <400; j++)
  strip.setPixelColor(j, strip.Color(0, 30, 0));
  strip.show();

}