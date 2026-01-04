#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- HARDWARE CONFIGURATION ---
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define SDA_PIN 5
#define SCL_PIN 6
#define SCREEN_OFFSET_W 30
#define SCREEN_OFFSET_H 24


// --- SENSOR CONFIGURATION ---
const int sensorPin = 0;   // GPIO 0
const float RL = 10000.0;  // 10k Load Resistor (Standard on breakout)
float R0 = 1000.0;         // Initial guess for R0 (will auto-adjust)

// Display Object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);

  // Initialize I2C and Display
  Wire.begin(SDA_PIN, SCL_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("Display Failed");
    for(;;);
  }

  // Analog Setup
  analogReadResolution(12);

  // Show "Warming Up" message
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("Sensor Heating...");
  display.setCursor(10, 35);
  display.println("Wait 10s...");
  display.display();
  
  // Give it a moment to stabilize initially
  delay(5000); 
}

void loop() {
  // 1. Read Raw Sensor Data
  int raw = analogRead(sensorPin);
  
  // 2. Convert to Sensor Voltage
  // (3.3V reference / 4095 steps) * 2 (Voltage Divider Compensation)
  float sensorVoltage = (raw * (3.3 / 4095.0)) * 2.0;

  // Safety: Prevent division by zero
  if (sensorVoltage < 0.01) sensorVoltage = 0.01;

  // 3. Calculate Sensor Resistance (Rs)
  // Formula: Rs = (Vcc * RL / V_out) - RL
  // Vcc is 5.0V (Sensor Power), RL is 10,000 Ohms
  float Rs = (5.0 * RL / sensorVoltage) - RL;

  // --- AUTO CALIBRATION MAGIC ---
  // If current resistance is HIGHER than R0, it means the air is 
  // "cleaner" than we thought. Update R0 to match this new baseline.
  if (Rs > R0) {
    R0 = Rs;
  }
  // -----------------------------

  // 4. Calculate Ratio (0.0 to 1.0)
  float ratio = Rs / R0;

  // 5. Update Display
  display.clearDisplay();
  
  // Title
  display.setCursor(0+SCREEN_OFFSET_W, 0+SCREEN_OFFSET_H);
  display.setTextSize(1);
  display.print("Air Quality");

  // Main Number
  display.setCursor(0+SCREEN_OFFSET_W, 15+SCREEN_OFFSET_H);
  display.setTextSize(1);
  
  // Show "Clean" if ratio is near 1.0 (e.g. > 0.95)
  if (ratio > 0.95) {
     display.print("100%"); // Easier to read than 1.00
  } else {
     display.print(ratio * 100, 0); // Show as percentage
     display.print("%");
  }

  // Status Bar (Visual Indicator)
  display.setTextSize(1);
  display.setCursor(0+SCREEN_OFFSET_W, 30+SCREEN_OFFSET_H);
  
  if (ratio > 0.8) {
    display.print("Status: Good");
  } else if (ratio > 0.4) {
    display.print("Status: Warning");
  } else {
    display.print("Status: DANGER");
  }

  // Debug Data at bottom
  display.setCursor(0+SCREEN_OFFSET_W, 55+SCREEN_OFFSET_H);
  display.print("V:"); 
  display.print(sensorVoltage, 2);
  display.print("v");

  display.display();
  
  // Debug to Serial
  Serial.print("Volts: "); Serial.print(sensorVoltage);
  Serial.print(" | Rs: "); Serial.print(Rs);
  Serial.print(" | R0: "); Serial.print(R0);
  Serial.print(" | Ratio: "); Serial.println(ratio);

  delay(200);
}