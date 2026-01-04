#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- HARDWARE CONFIGURATION ---
#define SCREEN_WIDTH 128 // We use standard size to ensure compatibility
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 
#define SDA_PIN       5  // ESP32-C3 SuperMini I2C Pins
#define SCL_PIN       6

// Initialize Display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- SENSOR CONFIGURATION ---
const int sensorPin = 0;   // GPIO 0
const float RL = 10000.0;  // Load Resistor (10k)
float R0 = 10000.0;        // Default R0 (will be overwritten by calibration)
bool calibrated = false;

// --- GRAPHING VARIABLES ---
// We store the last 128 readings to create the scrolling line
float history[SCREEN_WIDTH]; 
int historyIndex = 0;

void setup() {
  Serial.begin(115200);

  // 1. Initialize OLED
  // Address 0x3C is standard for these modules
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  // Clear and prepare screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Warming Up...");
  display.display();

  // 2. Initialize Sensor
  analogReadResolution(12); // 0-4095
  
  // Fill history with "Clean Air" (1.0) values initially
  for(int i=0; i<SCREEN_WIDTH; i++) {
    history[i] = 1.0; 
  }

  // 3. Quick Calibration (Blocking)
  Serial.println("Calibrating...");
  float totalVoltage = 0;
  // Take 20 quick readings
  for(int i=0; i<20; i++) {
    int raw = analogRead(sensorPin);
    float pinVolts = raw * (3.3 / 4095.0);
    totalVoltage += (pinVolts * 2.0); // x2 for voltage divider
    delay(100);
  }
  float avgVoltage = totalVoltage / 20.0;
  if(avgVoltage < 0.1) avgVoltage = 0.1;
  
  // Calculate R0
  R0 = (5.0 * RL / avgVoltage) - RL;
  calibrated = true;
  
  display.clearDisplay();
}

void loop() {
  if (!calibrated) return;

  // --- READ SENSOR ---
  int raw = analogRead(sensorPin);
  float pinVoltage = raw * (3.3 / 4095.0);
  float sensorVoltage = pinVoltage * 2.0; // Compensate for divider
  if(sensorVoltage < 0.01) sensorVoltage = 0.01;
  
  float Rs = (5.0 * RL / sensorVoltage) - RL;
  float ratio = Rs / R0;

  // --- UPDATE HISTORY ARRAY (Shift Left) ---
  // Move every point one step to the left
  for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
    history[i] = history[i+1];
  }
  // Add new reading at the end (Right side)
  history[SCREEN_WIDTH - 1] = ratio;

  // --- DRAW THE GRAPH ---
  display.clearDisplay();

  // 1. Draw Reference Lines
  // Top (Ratio 1.0 - Clean) is Y=0
  // Middle (Ratio 0.5 - Warning) is Y=32
  // Bottom (Ratio 0.0 - Danger) is Y=64
  
  // Dotted Line for "Warning Level" (0.5 Ratio)
  for(int i=0; i<SCREEN_WIDTH; i+=4) {
    display.drawPixel(i, SCREEN_HEIGHT/2, SSD1306_WHITE); 
  }

  // 2. Plot the Heartbeat Line
  for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
    // Map Ratio (0.0 to 1.5) to Screen Y (Height to 0)
    // We clamp the ratio to keep the line on screen
    float val1 = history[i];
    float val2 = history[i+1];

    // Constrain values for display purposes (so it doesn't wrap around)
    if(val1 > 1.2) val1 = 1.2;
    if(val2 > 1.2) val2 = 1.2;
    
    // Math: Y = Height - (Ratio * Height)
    // If Ratio is 1.0, Y = 0 (Top)
    // If Ratio is 0.5, Y = 32 (Middle)
    int y1 = SCREEN_HEIGHT - (val1 * SCREEN_HEIGHT);
    int y2 = SCREEN_HEIGHT - (val2 * SCREEN_HEIGHT);

    display.drawLine(i, y1, i+1, y2, SSD1306_WHITE);
  }

  // 3. Display numerical value in corner (Optional)
  display.setCursor(0, 0);
  display.print("R:");
  display.println(ratio, 2);

  // If danger, flash a warning text
  if(ratio < 0.4) {
    display.setCursor(50, 0);
    display.print("GAS!");
  }

  display.display();
  
  // Faster update for smoother scrolling (100ms)
  delay(100); 
}