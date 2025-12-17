
#include <Adafruit_NeoPixel.h>

#define LED_PIN   7
#define NUM_LEDS  256        // 16 × 16 matrix
int n=16, m=16;
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Matrix size
const uint8_t WIDTH  = 16;
const uint8_t HEIGHT = 16;

void setup() {
  strip.begin();
  strip.show(); // clear
  Serial.begin(115200);
  //showStaticPattern(strip.Color(3, 0, 0));
}

void loop() {
  // drawMatrix(smiley, palette);
  // delay(200);
  // for (int i = 0; i < HEIGHT; i++)
  // for (int j = 0; j < WIDTH; j++)
  //    strip.setPixelColor(i*HEIGHT+j, strip.Color(0, 00, 0));
  //    strip.show();
  // delay(100);
  
  for (int j = 0; j <250; j++)
  strip.setPixelColor(j, strip.Color(0, 30, 0));
  strip.show();

}