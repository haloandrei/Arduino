#include <Adafruit_NeoPixel.h>
#include <avr/pgmspace.h>

// --- CONFIG ---
#define PIN         2
#define PANELS      3
#define PANEL_W     16
#define PANEL_H     16
#define BRIGHT_CAP  50

const uint8_t WIDTH  = 48;
const uint8_t HEIGHT = 16;
const uint16_t NUM_LEDS = 768;

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// --- PALETTE ---
uint32_t palette[5]; // build in setup after strip.begin()

// --- INITIAL IMAGE IN FLASH ---
// Edit these numbers (0..4) however you want.
// Tip: each row must contain 48 values.
const uint8_t canvasInit[HEIGHT][WIDTH] PROGMEM = {
  // y = 0
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2 },
  // y = 1
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 2
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 3
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 4
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 5
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 6
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 7
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 8
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 9
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 10
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 11
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 12
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 13
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 14
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2 },
  // y = 15
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2 }
};

// --- RUNTIME CANVAS IN RAM (this is what fill modifies) ---
uint8_t canvas[HEIGHT][WIDTH];

// --- VERTICAL TOPOLOGY MAPPING ---
uint16_t xyToIndex(uint8_t x, uint8_t y) {
  if (x >= WIDTH || y >= HEIGHT) return 0;
  uint8_t panel = x / PANEL_W;
  uint8_t lx = x % PANEL_W;
  uint8_t ly = y;

  uint16_t colBase = (uint16_t)lx * PANEL_H;
  uint16_t localIndex;
  if (lx % 2 == 0) localIndex = colBase + ly;
  else             localIndex = colBase + (PANEL_H - 1 - ly);
  return (uint16_t)panel * 256 + localIndex;
}

void loadCanvasFromProgmem() {
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      canvas[y][x] = pgm_read_byte(&(canvasInit[y][x]));
    }
  }
}

void drawCanvas() {
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      uint8_t c = canvas[y][x];
      if (c > 4) c = 0; // safety clamp
      strip.setPixelColor(xyToIndex(x, y), palette[c]);
    }
  }
  strip.show();
}

uint8_t canvas2[16][48] = {

  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},

  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,3,3,3,3,0,0,0,0,0},

  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,3,0,0,0,3,0,0,0,0,0},

  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,3,0,0,0,0,0},

  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,3,3,0,0,0,3,3,3,3,3,0,0,0,0,0},

  {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,3,0,0,0,0,0,3,3,0,3,3,3,3,0},

  {0,0,0,0,0,0,0,0,0,1,1,1,0,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,0,3,0,0,3,0,0,0,3,3,0,0,0,0,0,0,0,0},

  {0,0,0,0,0,0,0,0,0,1,0,0,1,1,0,0,1,1,0,0,0,0,1,0,0,0,0,1,0,1,0,3,3,3,3,0,0,3,3,0,0,0,0,0,0,0,0,0},

  {0,0,0,0,0,0,0,0,0,1,1,0,1,1,0,0,0,1,0,0,0,1,1,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,2,0},

  {0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,1,1,1,0,0,0,1,0,0,0,1,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,0,2,2,0,0},

  {0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,1,1,1,0,0,1,1,1,1,1,0,0,1,0,0,0,0,0,0,0,0,0,2,2,0,2,2,0,0,0},

  {0,0,0,0,0,0,0,0,1,1,0,1,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,1,0,0,0,0,0,0,2,2,2,2,0,0,2,0,0,0,0},

  {0,0,0,0,0,0,0,0,1,0,0,0,1,0,1,1,1,1,1,0,1,0,1,0,0,0,0,0,0,1,1,0,0,0,0,2,0,0,2,2,0,0,2,0,0,0,0,0},

  {0,0,0,0,0,0,0,1,0,0,0,1,1,1,1,1,0,0,0,0,1,1,0,0,0,0,0,1,1,0,0,0,0,0,0,2,0,0,2,2,0,0,2,0,0,0,0,0},

  {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,0,2,0,2,0,0,0,0,0},

  {0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,0,0,2,2,0,0,0,0,0,0}

};

void draw(){
  strip.clear();
  Serial.println("Am ajuns aici");
  for (int i=0;i<16;i++)
    for (int j=0;j<48;j++)
      strip.setPixelColor(xyToIndex(i, j), strip.ColorHSV(palette[canvas2[i][j]], 255, BRIGHT_CAP));
  strip.show();
}

// --- FLOOD FILL (recursive) ---
void recursiveFill(int x, int y, uint8_t targetColor, uint8_t replaceColor) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
  if (canvas[y][x] != targetColor) return;

  canvas[y][x] = replaceColor;
  strip.setPixelColor(xyToIndex((uint8_t)x, (uint8_t)y), palette[replaceColor]);
  strip.show();
  delay(30);

  recursiveFill(x + 1, y, targetColor, replaceColor);
  recursiveFill(x - 1, y, targetColor, replaceColor);
  recursiveFill(x, y + 1, targetColor, replaceColor);
  recursiveFill(x, y - 1, targetColor, replaceColor);
}

void setup() {
  Serial.begin(115200);

  strip.begin();
  strip.setBrightness(BRIGHT_CAP);
  strip.show();

  Serial.println("Incepe programul");
  // Build palette after strip is initialized
  palette[0] = strip.Color(0, 0, 0);
  palette[1] = strip.Color(BRIGHT_CAP, 0, 0);
  palette[2] = strip.Color(0, 0, BRIGHT_CAP);
  palette[3] = strip.Color(0, BRIGHT_CAP, 0);
  palette[4] = strip.Color(BRIGHT_CAP, BRIGHT_CAP, 0);
Serial.println("Incepe desenul");
  //loadCanvasFromProgmem();
  draw();

  delay(1000);

  // Example: fill starting at (0,0) replacing 0 with 4
  //recursiveFill(0, 0, 0, 4);
}

void loop() {
  // Hold result
  delay(1000);
}
