
#include <FastLED.h>
#include "driver/gpio.h" // Include GPIO driver header for ESP32

#define NUM_LEDS 299
#define DATA_PIN 2
#define PIN_BUTTON GPIO_NUM_1 // Use GPIO_NUM_4 as it matches the type gpio_num_t

CRGB* leds;

int currentAnimation = 0; // Tracks the current animation (0, 1, or 2)
bool buttonState = false; // Tracks if the button is pressed
bool lastButtonState = false; // Tracks the last state of the button

// Variables for non-blocking animations
int animStep = 0; // Tracks the current step of the animation
unsigned long lastUpdate = 0; // Tracks the last time the animation updated
const unsigned long animInterval = 5; // Interval between animation updates (ms)

void setup() {
  // Allocate LED array in PSRAM
  leds = (CRGB*)heap_caps_malloc(NUM_LEDS * sizeof(CRGB), MALLOC_CAP_SPIRAM);
  if (!leds) {
    Serial.println("Failed to allocate LED array in PSRAM!");
    while (1); // Halt
  }

  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  pinMode(PIN_BUTTON, INPUT); 

  Serial.begin(9600);
  Serial.println("Setup complete");
}

void loop() {
  // Cooldown variables
  static unsigned long lastButtonPress = 0; // Tracks the last button press time
  const unsigned long buttonCooldown = 1000; // Cooldown duration in milliseconds

  // Read button state
  buttonState = digitalRead(PIN_BUTTON) == LOW; // Button is active LOW

  // Detect a button press with cooldown
  if (buttonState && !lastButtonState) {
    unsigned long now = millis();
    if (now - lastButtonPress > buttonCooldown) { // Check if cooldown has elapsed
      currentAnimation = (currentAnimation + 1) % 4; // Cycle through animations
      Serial.println("Button pressed, switching animation");
      Serial.println(currentAnimation);
      animStep = 0; // Reset animation step
      lastButtonPress = now; // Update the last button press time
    }
  }
  lastButtonState = buttonState;

  // Execute the current animation
  switch (currentAnimation) {
    case 0:
      animation1();
      break;
    case 1:
      animation2();
      break;
    case 2:
      animation3();
      break;
    case 3:
      animation4();
      break;
  }
}

void animation1() {
  unsigned long now = millis();
  if (now - lastUpdate > animInterval) {
    lastUpdate = now;
    int center = NUM_LEDS / 2;
    static int k = 0; // Tracks expansion of the animation
    if (animStep <= center) {
      if (animStep < 30) k++;
      if (animStep > center - 30) k--;
      for (int i = 0; i <= center; i++) {
        leds[center - i] = CHSV(0, 0, 0);
        leds[center + i] = CHSV(0, 0, 0);
      }
      for (int i = animStep; i <= animStep + k; i++) {
        leds[center - i] = CRGB(255, 70 + (i -animStep) * 3, (i -animStep) * 2);
        leds[center + i] = CRGB(255, 70 + (i -animStep) * 3, (i -animStep) * 2);
      }
      FastLED.show();
      animStep++;
    } else {
      animStep = 0; // Reset animation
    }
  }
}

void animation2() {
  unsigned long now = millis();
  static int blinkStep = 0; // Tracks the current blink step (outermost, closer, middle)
  static bool ledsOn = false; // Tracks whether LEDs are on or off
  static int stepCounter = 0; // Tracks the duration of step 2
  
  // Tempo-based timing
  const unsigned long slowInterval = 370; // Duration for slow parts (ms)
  const unsigned long fastInterval = 185; // Duration for fast parts (ms)
  unsigned long currentInterval = (stepCounter < 2) ? fastInterval :  slowInterval;


  int segmentLength = NUM_LEDS / 4; // Length of each quarter

  if (now - lastUpdate > currentInterval) {
    lastUpdate = now;

    if (ledsOn) {
      // Turn all LEDs off
      FastLED.clear();
      FastLED.show();
      ledsOn = false;
    } else {
      // Light LEDs based on the current blink step
      if (blinkStep == 0) {
        // Outermost quarters
        for (int i = 0; i < segmentLength; i++) {
          leds[i] = CHSV(160 + (i * 95 / segmentLength), 255, 200); // Blue to violet
          leds[NUM_LEDS - 1 - i] = CHSV(160 + (i * 95 / segmentLength), 255, 200);
        }
      } else if (blinkStep == 1) {
        // Second quarters
        for (int i = segmentLength * 0.7; i < 1.6 * segmentLength; i++) {
          leds[i] = CHSV(160 + ((i - segmentLength) * 95 / segmentLength), 255, 200); // Blue to violet
          leds[NUM_LEDS - 1 - i] = CHSV(160 + ((i - segmentLength) * 95 / segmentLength), 255, 200);
        }
      } else if (blinkStep == 2) {
        // Middle quarters
        for (int i = 2 * segmentLength; i < 3 * segmentLength; i++) {
          leds[i] = CHSV(160 + ((i - 2 * segmentLength) * 95 / segmentLength), 255, 200); // Blue to violet
          leds[NUM_LEDS - 1 - i] = CHSV(160 + ((i - 2 * segmentLength) * 95 / segmentLength), 255, 200);
        }
      }
      FastLED.show();
      ledsOn = true;

     
      blinkStep = (blinkStep + 1) % 3;
    }
  }
}




void animation3() {
  unsigned long now = millis();
  if (now - lastUpdate > animInterval) {
    lastUpdate = now;
    if (animStep < NUM_LEDS) {
      leds[animStep] = CHSV(animStep * 5, 255, 200); // Gradient effect
      FastLED.show();
      animStep++;
    } else {
      animStep = 0; // Reset animation
      FastLED.clear();
      FastLED.show();
    }
  }
}

void animation4() {
  unsigned long now = millis();
  static int stepCounter = 0; // Tracks the current step (0 to 5 for the rhythm)
  static bool ledsOn = false; // Tracks whether LEDs are on or off
  
  // Timing intervals
  const unsigned long slowInterval = 400; // Duration for slow parts (ms)
  const unsigned long fastInterval = 150; // Duration for fast parts (ms)
  unsigned long currentInterval = (stepCounter < 4) ? slowInterval : fastInterval;

  int segmentLength = NUM_LEDS / 4; // Length of each quarter

  if (now - lastUpdate > currentInterval) {
    lastUpdate = now;

    if (ledsOn) {
      // Turn all LEDs off
      FastLED.clear();
      FastLED.show();
      ledsOn = false;
    } else {
      // Light LEDs based on the current step
      switch (stepCounter) {
        case 0: // Outermost segments
          for (int i = 0; i < segmentLength; i++) {
            leds[i] = CHSV(160 + (i * 95 / segmentLength), 255, 200); // Blue to violet
            leds[NUM_LEDS - 1 - i] = CHSV(160 + (i * 95 / segmentLength), 255, 200);
          }
          break;
        case 1: // Second quarters
          for (int i = segmentLength; i < 2 * segmentLength; i++) {
            leds[i] = CHSV(160 + ((i - segmentLength) * 95 / segmentLength), 255, 200); // Blue to violet
            leds[NUM_LEDS - 1 - i] = CHSV(160 + ((i - segmentLength) * 95 / segmentLength), 255, 200);
          }
          break;
        case 2: // Middle quarters
          for (int i = 2 * segmentLength; i < 3 * segmentLength; i++) {
            leds[i] = CHSV(160 + ((i - 2 * segmentLength) * 95 / segmentLength), 255, 200); // Blue to violet
            leds[NUM_LEDS - 1 - i] = CHSV(160 + ((i - 2 * segmentLength) * 95 / segmentLength), 255, 200);
          }
          break;
        case 3: // Full middle
          for (int i = 3 * segmentLength; i < NUM_LEDS; i++) {
            leds[i] = CHSV(160 + ((i - 3 * segmentLength) * 95 / segmentLength), 255, 200); // Blue to violet
          }
          break;
        case 4: // First fast clap
          for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CHSV(200, 255, 255); // Bright blue for a strong clap
          }
          break;
        case 5: // Second fast clap
          for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CHSV(240, 255, 255); // Bright violet for a strong clap
          }
          break;
      }
      FastLED.show();
      ledsOn = true;

      // Increment stepCounter and loop back after 6 steps
      stepCounter = (stepCounter + 1) % 6;
    }
  }
}
