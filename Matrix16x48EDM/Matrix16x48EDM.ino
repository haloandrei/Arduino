#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define PIN 12

// --- MATRIX CONFIGURATION ---
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(16, 16, 3, 1, PIN,
  NEO_TILE_TOP   + NEO_TILE_LEFT   + NEO_TILE_ROWS   + NEO_TILE_PROGRESSIVE +
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB + NEO_KHZ800);

// --- YOUR TEXT STRINGS ---
char message[] = "67 INFOEL 67";
char sneakyMessage[] = "67 INFOHEL 67"; 

const int textWidthSize2 = 12 * 12; // 144 pixels
const int textWidthSize1 = 12 * 6;  // 72 pixels

// Text color locked to Pure Blue
const uint16_t textColor = matrix.Color(0, 0, 255); 
const uint16_t shadowColor = matrix.Color(0, 0, 0); // Pure Black

// --- TIMING & STATE VARIABLES ---
unsigned long previousScroll = 0;
unsigned long previousStrobe = 0;
unsigned long pulseStartTime = 0;

const int scrollSpeed = 15;   
const int strobeSpeed = 136;  

int x;
bool strobeState = false;

uint16_t currentRWB[3];

enum AnimationState { SCROLLING, PULSATING };
AnimationState currentState = SCROLLING;

void setup() {
  matrix.begin();
  matrix.setBrightness(100); 
  matrix.setTextWrap(false);
  x = matrix.width(); 
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 1. BANGARANG STROBE LOGIC ---
  if (currentMillis - previousStrobe >= strobeSpeed) {
    previousStrobe = currentMillis;
    strobeState = !strobeState; 

    // PERMUTATION GENERATOR
    if (!strobeState) {
      uint16_t rwb[3] = {
        matrix.Color(255, 0, 0),     
        matrix.Color(255, 255, 255), 
        matrix.Color(0, 0, 255)      
      };
      
      // Fisher-Yates Shuffle
      for (int i = 2; i > 0; i--) {
        int j = random(i + 1);
        uint16_t temp = rwb[i];
        rwb[i] = rwb[j];
        rwb[j] = temp;
      }

      currentRWB[0] = rwb[0];
      currentRWB[1] = rwb[1];
      currentRWB[2] = rwb[2];
    }
  }

  // --- 2. CRAZY BACKGROUND GENERATION ---
  if (strobeState) {
    for (int panel = 0; panel < 3; panel++) {
      uint16_t panelColor = matrix.Color(random(20, 255), random(20, 255), random(20, 255));
      matrix.fillRect(panel * 16, 0, 16, 16, panelColor);
      
      for(int noise = 0; noise < 8; noise++) {
        matrix.drawPixel((panel * 16) + random(16), random(16), matrix.Color(255, 255, 255));
        matrix.drawPixel((panel * 16) + random(16), random(16), shadowColor);
      }
    }
  } else {
    for (int panel = 0; panel < 3; panel++) {
      matrix.fillRect(panel * 16, 0, 16, 16, currentRWB[panel]);
    }
  }

  // --- 3. ANIMATION STATE MACHINE ---
  if (currentState == SCROLLING) {
    matrix.setTextSize(2);
    
    // Step A: Draw the Black Drop Shadow (Offset by +1 X and +1 Y)
    matrix.setTextColor(shadowColor); 
    matrix.setCursor(x + 1, 2); 
    matrix.print(message);

    // Step B: Draw the Solid Blue Text directly over it
    matrix.setTextColor(textColor); 
    matrix.setCursor(x, 1);
    matrix.print(message);

    if (currentMillis - previousScroll >= scrollSpeed) {
      previousScroll = currentMillis;
      x--; 
      
      if (x < -textWidthSize2) {
        currentState = PULSATING;       
        pulseStartTime = currentMillis; 
      }
    }
    
  } else if (currentState == PULSATING) {
    matrix.setTextSize(1);
    unsigned long elapsedPulse = currentMillis - pulseStartTime;
    bool showSneaky = (elapsedPulse > 450 && elapsedPulse < 550);

    int currentX;
    if (showSneaky) {
      currentX = (matrix.width() - (strlen(sneakyMessage) * 6)) / 2;
    } else {
      currentX = (matrix.width() - textWidthSize1) / 2; 
    }

    // Drop Shadow for the Pulsating Text
    matrix.setTextColor(shadowColor); 
    matrix.setCursor(currentX + 1, 5); 
    if (showSneaky) matrix.print(sneakyMessage);
    else matrix.print(message);

    // Rapid Pulsate Effect (Flashes between White and Blue)
    if ((currentMillis / 40) % 2 == 0) {
      matrix.setTextColor(matrix.Color(255, 255, 255)); 
    } else {
      matrix.setTextColor(textColor);                
    }
    
    matrix.setCursor(currentX, 4);
    if (showSneaky) matrix.print(sneakyMessage);
    else matrix.print(message);

    if (elapsedPulse >= 1000) {
      currentState = SCROLLING; 
      x = matrix.width();       
    }
  }

  matrix.show();
}