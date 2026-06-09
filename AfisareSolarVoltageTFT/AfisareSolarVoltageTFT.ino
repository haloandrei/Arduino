#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI(); 

// --- Pin Definitions ---
const int voltPin = 34;    // Analog pin for voltage divider
const int motorIN1 = 25;   // H-Bridge control pin 1
const int motorIN2 = 26;   // H-Bridge control pin 2

// --- Voltage Divider Settings ---
// Adjust these depending on the exact resistors you use
const float R1 = 10000.0; // 10k Ohm
const float R2 = 4700.0;  // 4.7k Ohm
// ESP32 ADC calibration factor (adjust if reading is slightly off)
const float calibrationOffset = 1.0; 

// --- Chart Variables ---
const int chartWidth = 240; // Width of 1.14" TFT in landscape
const int chartHeight = 100; 
int xPos = 0;
int lastY = 0;

void setup() {
  Serial.begin(115200);
  
  // Motor Pins
  pinMode(motorIN1, OUTPUT);
  pinMode(motorIN2, OUTPUT);
  stopMotor(); // Ensure motor is off at start

  // Display Init
  tft.init();
  tft.setRotation(1); // Landscape
  tft.fillScreen(TFT_BLACK);
  
  // Draw Graph Borders
  tft.drawRect(0, 30, chartWidth, chartHeight + 2, TFT_WHITE);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
}

void loop() {
  // 1. Read and Calculate Voltage
  int rawADC = analogRead(voltPin);
  
  // Convert ADC value (0-4095) to ESP32 Pin Voltage (0-3.3V)
  float pinVoltage = (rawADC / 4095.0) * 3.3;
  
  // Reverse the voltage divider math to find actual Solar Voltage
  float solarVoltage = (pinVoltage * (R1 + R2) / R2) * calibrationOffset;

  // 2. Update Text on Screen
  tft.setTextSize(2);
  tft.setCursor(10, 5);
  tft.print("Solar: ");
  tft.print(solarVoltage, 2); 
  tft.print(" V   ");

  // 3. Draw Chart Line
  // Map voltage (0V to 8V peak) to the Y-axis of the screen
  int yPos = map(solarVoltage * 10, 0, 80, 30 + chartHeight, 30); 
  
  if (xPos == 0) {
    lastY = yPos;
  }
  
  tft.drawLine(xPos - 1, lastY, xPos, yPos, TFT_GREEN);
  lastY = yPos;

  // Scroll logic: if we hit the edge, clear graph and start over
  if (xPos >= chartWidth) {
    xPos = 0;
    tft.fillRect(1, 31, chartWidth - 2, chartHeight, TFT_BLACK);
  } else {
    xPos++;
  }

  // 4. Example Motor Control Logic
  // Replace this with your actual logic (e.g., based on time or voltage)
  if (solarVoltage > 5.5) {
    driveMotorForward(); 
  } else {
    stopMotor();
  }

  delay(100); // Chart update speed
}

// --- Motor Control Helper Functions ---

void driveMotorForward() {
  digitalWrite(motorIN1, HIGH);
  digitalWrite(motorIN2, LOW);
}

void driveMotorBackward() {
  digitalWrite(motorIN1, LOW);
  digitalWrite(motorIN2, HIGH);
}

void stopMotor() {
  digitalWrite(motorIN1, LOW);
  digitalWrite(motorIN2, LOW);
}