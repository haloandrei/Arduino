/*************************************************************
  Refactored for ESP32 (non-S3) to blink a 16×16 WS2812B (NeoPixel) 
  matrix when a digital button is pressed.
*************************************************************/

#include <Adafruit_NeoPixel.h>

// ----------------------
// NeoPixel Configuration
// ----------------------
#define LED_PIN    2        // Pin connected to the NeoPixel data-in
#define NUM_LEDS   256      // 16×16 matrix

// ----------------------
// Button Configuration
// ----------------------
#define BUTTON_PIN 4        // GPIO pin connected to button

// Create the Adafruit_NeoPixel object
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Helper variables
bool flashing = false;
unsigned long flashStart = 0;
const unsigned long flashDuration = 100; // duration in ms for the flash

// Button state tracking
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(115200);

  // Initialize NeoPixel strip
  strip.begin();
  strip.show(); // Turn OFF all pixels ASAP

  // Initialize button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("ESP32 WS2812B Button Flash Example Started!");
}

void loop() {
  // Read the button state
  bool buttonState = digitalRead(BUTTON_PIN);

  // Check for button press with debounce
  if (buttonState != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (lastButtonState == HIGH && buttonState == LOW && !flashing) {
      // Begin a flash on button press
      flashing = true;
      flashStart = millis();
      flashMatrix(255, 255, 255); // White flash (R=255, G=255, B=255)
    }
  }

  lastButtonState = buttonState;

  // If currently flashing, check if it's time to turn off
  if (flashing) {
    unsigned long now = millis();
    if (now - flashStart >= flashDuration) {
      flashMatrix(0, 0, 0);
      flashing = false;
    }
  }
}

/**
 * @brief Lights up the entire matrix with a specified color
 *        (or turns it off) and immediately updates the strip.
 * 
 * @param r Red brightness (0-255)
 * @param g Green brightness (0-255)
 * @param b Blue brightness (0-255)
 */
void flashMatrix(uint8_t r, uint8_t g, uint8_t b) {
  for(uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}
