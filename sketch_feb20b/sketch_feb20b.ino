#include <Adafruit_NeoPixel.h>

#define LED_PIN 2    // GPIO pin connected to the LED strip
#define NUM_LEDS 150   // Total number of LEDs in the strip

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
int n, max1, poz, nr;
void setup() {
  strip.begin();            // Initialize the LED strip
  strip.show();             // Turn off all LEDs initially
  strip.setBrightness(50);  // Adjust brightness (0 to 255)

  Serial.begin(115200);
  Serial.println("LEDs ready to be controlled by index.");
}

void aprinde_becul(int i, int red, int green, int blue) {
  strip.setPixelColor(i, strip.Color(red, green, blue));  // Set LED at index i to red (RGB: 255,0,0)
  strip.show();                                           // Update the LEDs to apply the change
}
void inchide_becul(int i) {
  strip.setPixelColor(i, strip.Color(0, 0, 0));
  strip.show();
}

int prim(int n) {
  int eprim = 1;
  for (int i = 2; i < n; i++)
    if (n % i == 0)
      eprim = 0;
  return eprim;
}

int x = 0;
void loop() {
  int a = 3, b = 120;
  for (int i = 1; i <= b; i++) {
    aprinde_becul((a * i + x) % NUM_LEDS, 255, 165, 0);
  }
  delay(100);  // Wait 10 seconds
  for (int i = 0; i < NUM_LEDS; i++) {
    inchide_becul(i);  // Turn off all LEDs
  }

  delay(100);  // Short delay before the loop repeats
}
