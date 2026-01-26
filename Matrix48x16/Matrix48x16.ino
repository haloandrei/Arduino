#include <Adafruit_NeoPixel.h>

// --- CONFIGURATION ---
#define PIN         12       // ESP32 Pin connected to Data In
#define PANELS      3       // Number of panels
#define PANEL_W     16      // Width of one panel
#define PANEL_H     16      // Height of one panel
#define BRIGHT_CAP  15      // Max brightness per channel (Safety Wrapper)

// Calculated Dimensions (48 x 16)
const uint8_t WIDTH  = PANEL_W * PANELS;
const uint8_t HEIGHT = PANEL_H;
const uint16_t NUM_LEDS = WIDTH * HEIGHT;

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// --- 1. SAFETY WRAPPER (Prevents blinding/melting) ---
uint32_t safeColor(uint8_t r, uint8_t g, uint8_t b) {
  if (r > BRIGHT_CAP) r = BRIGHT_CAP;
  if (g > BRIGHT_CAP) g = BRIGHT_CAP;
  if (b > BRIGHT_CAP) b = BRIGHT_CAP;
  return strip.Color(r, g, b);
}

uint16_t xyToIndex(uint8_t x, uint8_t y) {
  if (x >= WIDTH || y >= HEIGHT) return 0; 

  uint8_t panel = x / PANEL_W;      // Panel 0, 1, or 2
  uint8_t lx = x % PANEL_W;         // Local X (0-15)
  uint8_t ly = y;                   // Local Y (0-15)

  // Calculate the base index for this column
  // (Each column has 16 pixels)
  uint16_t colBase = lx * PANEL_H; 

  uint16_t localIndex;
  
  // Vertical Zig-Zag Logic
  if (lx % 2 == 0) {
    // Even Columns: DOWN (Top -> Bottom)
    // Index increases as Y increases
    localIndex = colBase + ly;
  } else {
    // Odd Columns: UP (Bottom -> Top)
    // Index decreases as Y increases
    localIndex = colBase + (PANEL_H - 1 - ly);
  }

  return (panel * 256) + localIndex;
}

// --- 3. YOUR FUNCTION: Spiral Rainbow ---
void drawSpiralRainbow() {
  strip.clear();
  
  int top = 0, bottom = HEIGHT - 1;
  int left = 0, right = WIDTH - 1;
  long pixelHue = 0; // Stores color (0 to 65535)

  // Loop until boundaries meet
  while (top <= bottom && left <= right) {
    
    // Top Row (Left to Right)
    for (int i = left; i <= right; i++) {
      strip.setPixelColor(xyToIndex(i, top), strip.ColorHSV(pixelHue, 255, BRIGHT_CAP));
      strip.show();
      pixelHue += 100; // Shift color slightly
      delay(5);
    }
    top++;

    // Right Column (Top to Bottom)
    for (int i = top; i <= bottom; i++) {
      strip.setPixelColor(xyToIndex(right, i), strip.ColorHSV(pixelHue, 255, BRIGHT_CAP));
      strip.show();
      pixelHue += 100;
      delay(5);
    }
    right--;

    // Bottom Row (Right to Left)
    if (top <= bottom) {
      for (int i = right; i >= left; i--) {
        strip.setPixelColor(xyToIndex(i, bottom), strip.ColorHSV(pixelHue, 255, BRIGHT_CAP));
        strip.show();
        pixelHue += 100;
        delay(5);
      }
      bottom--;
    }

    // Left Column (Bottom to Top)
    if (left <= right) {
      for (int i = bottom; i >= top; i--) {
        strip.setPixelColor(xyToIndex(left, i), strip.ColorHSV(pixelHue, 255, BRIGHT_CAP));
        strip.show();
        pixelHue += 100;
        delay(5);
      }
      left++;
    }
  }
}

// --- SETUP & LOOP ---
void setup() {
  strip.begin();
  strip.setBrightness(255); // We control brightness via safeColor/ColorHSV 'v' parameter
  strip.show(); 
  
  // Run once to test
  drawSpiralRainbow();
}

void loop() {
  // Empty loop
}