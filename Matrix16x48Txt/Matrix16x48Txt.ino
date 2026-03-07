#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define PIN 12

// --- MATRIX CONFIGURATION ---
// We have 3 tiles, each 16x16 pixels.
// Total dimensions: 48 wide, 16 high.
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(16, 16, 3, 1, PIN,
  // 1. TILE SETTINGS (How the panels are arranged)
  NEO_TILE_TOP   + NEO_TILE_LEFT   + NEO_TILE_ROWS   + NEO_TILE_PROGRESSIVE +
  
  // 2. PIXEL SETTINGS (How pixels are wired inside one panel)
  // Based on your setup: Vertical Columns + ZigZag (Snake)
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  
  NEO_GRB + NEO_KHZ800);

// --- YOUR TEXT TO SCROLL ---
char message[] = "\x03   buna chifla \x03 \x03";

// Colors (R, G, B)
const uint16_t colors[] = {
  matrix.Color(50, 0, 0),   // Red
  matrix.Color(0, 50, 0),   // Green
  matrix.Color(0, 0, 50),   // Blue
  matrix.Color(50, 50, 0),  // Yellow
  matrix.Color(0, 50, 50),  // Cyan
  matrix.Color(50, 0, 50),  // Purple
  matrix.Color(50, 50, 50)  // White
};

void setup() {
  matrix.begin();
  
  // Set brightness low to save power/eyes (Range 0-255)
  matrix.setBrightness(40);
  
  // Setup Text
  matrix.setTextWrap(false); // Allow text to go off-screen
  matrix.setTextColor(colors[0]); // Start with Red
  matrix.setTextSize(1);     // Size 1 = 8 pixels high (standard)
                             // Size 2 = 16 pixels high (fills height)
}

int x = matrix.width(); // Start cursor at the right edge
int pass = 0;           // Cycle through colors

void loop() {
  matrix.fillScreen(0);    // Clear screen
  matrix.setCursor(x, 4);  // Set cursor (X = moving, Y = centered vertically)
  
  // Use a larger font if you want to fill the whole 16px height:
  // matrix.setTextSize(2); 
  // matrix.setCursor(x, 1); 

  matrix.print(message);

  // Move the text left by 1 pixel
  if(--x < -6 * (int)strlen(message)) { // Calculate end of message (~6px per char)
    x = matrix.width(); // Reset to right side
    
    // Change color for next pass
    if(++pass >= 7) pass = 0;
    matrix.setTextColor(colors[pass]);
  }
  matrix.show();
  delay(30); // Scrolling Speed (Lower = Faster)

  message[1] = message[1]+1; 
  message[2] = message[2]+2; 
}