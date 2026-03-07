#include <Adafruit_NeoPixel.h>

// --- CONFIGURATION ---
#define PIN         12       // ESP32 Pin connected to Data In
#define PANELS      3       // Number of panels
#define PANEL_W     16      // Width of one panel
#define PANEL_H     16      // Height of one panel
#define BRIGHT_CAP  10      // Max brightness per channel (Safety Wrapper)

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

uint8_t canvas[16][48] = {0};
int iMos,jMos;

uint16_t palette[] = {
  1,              // 0: Negru (Fundal)
  355,     // 1: Rosu
  235000,     // 2: Albastru
  23500,     // 3: Verde
  2850 // 4: Galben (Culoarea de umplere)
};
// --- 3. YOUR FUNCTION: Spiral Rainbow ---
void drawSpiralRainbow() {
  strip.clear();

  
    for (int i = 0; i < 16; i++)
    for (int j= 0; j< 48; j++)
     {
      Serial.println(canvas[i][j]);
      Serial.println(palette[canvas[i][j]]);
      if (canvas[i][j]==0)
        strip.setPixelColor(xyToIndex(j, i), strip.ColorHSV(1, 255, 0));
      else if (canvas[i][j]==2)
        strip.setPixelColor(xyToIndex(j, i), strip.ColorHSV(palette[canvas[i][j]], 255, 3));
      else if (canvas[i][j]<=4)
        strip.setPixelColor(xyToIndex(j, i), strip.ColorHSV(palette[canvas[i][j]], 255, BRIGHT_CAP));
      else 
        strip.setPixelColor(xyToIndex(j, i), strip.ColorHSV(canvas[i][j], 255, BRIGHT_CAP));
    }
      strip.show();
      delay(5);
  
}

void gasesteMosul(){
  for (int i = 0; i < 16; i++)
    for (int j= 0; j< 48; j++)
     {
      if(canvas[i][j]==1) {
        iMos = i;
        jMos = j;
      }
     }
}

int di[] = {-1, 0, 1, 0} ;
int dj[] = {0, 1, 0, -1} ;

int spiridusi = 0;

bool ok(int i, int j)
{
  if(i < 0 || i > 15 || j < 0 || j > 47)
    return false;
  else
    if(canvas[i][j] == 0 || canvas[i][j] > 4) return false;
  return true;
}

void fill(int ipoz, int jpoz, int dist){
  if(canvas[ipoz][jpoz] == 1) spiridusi++;
  canvas[ipoz][jpoz]=dist;
  drawSpiralRainbow();
  delay(100);
  for (int k = 0; k < 4; k++){
    int iv = ipoz + di[k];
    int jv = jpoz + dj[k];
    if (ok(iv,jv)==true)
    {
      fill(iv, jv, dist+1024);
    }
  }


}

void coloreaza(int pozx, int pozy, int dist){
   if (pozx < 0 || pozy < 0 || pozx > 15 || pozy > 47 || canvas[pozx][pozy] !=0) return;
   canvas[pozx][pozy] = dist;
   drawSpiralRainbow();
   delay(100);
   coloreaza(pozx+1, pozy, dist+1);
   coloreaza(pozx, pozy+1, dist+1);
   coloreaza(pozx, pozy-1, dist+1);
   coloreaza(pozx-1, pozy, dist+1);
}

// --- SETUP & LOOP ---
void setup() {
  strip.begin();
  strip.setBrightness(255); // We control brightness via safeColor/ColorHSV 'v' parameter
  strip.show(); 
  

}

void loop() {
  //gasesteMosul();
  //fill(iMos, jMos, 1024);
  coloreaza(8, 20, 1);
  delay(1000);
}