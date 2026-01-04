#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;

// --- Pin Definitions ---
const int xPin = A0;
const int yPin = A1;
const int swPin = 2;

// --- Player Variables ---
int playerX = 6;  
int playerY = 3;  // Start slightly higher to leave room for the leg

// --- Movement Settings ---
unsigned long lastMoveTime = 0;
const int moveSpeed = 150;     // Speed of movement
const int thresholdLow = 300;  // Joystick Deadzone Low
const int thresholdHigh = 700; // Joystick Deadzone High

// --- Matrix Frame Buffer ---
byte frame[8][12];

void setup() {
  Serial.begin(9600);
  matrix.begin();
  pinMode(swPin, INPUT_PULLUP);
}

void loop() {
  int xVal = analogRead(xPin);
  int yVal = analogRead(yPin);
  bool buttonPressed = (digitalRead(swPin) == LOW);

  // --- 1. Movement Logic ---
  if (millis() - lastMoveTime > moveSpeed) {
    // X Axis
    if (xVal < thresholdLow) playerX--;
    else if (xVal > thresholdHigh) playerX++;

    // Y Axis (Depending on orientation, swap ++ and -- if needed)
    if (yVal < thresholdLow) playerY--;
    else if (yVal > thresholdHigh) playerY++;

    // Boundary Checks (Keep player inside the 12x8 grid)
    if (playerX < 0) playerX = 0;
    if (playerX > 11) playerX = 11;
    if (playerY < 0) playerY = 0;
    if (playerY > 7) playerY = 7;

    lastMoveTime = millis();
  }

  // --- 2. Clear Frame ---
  for(int y = 0; y < 8; y++) {
    for(int x = 0; x < 12; x++) {
      frame[y][x] = 0;
    }
  }

  // --- 3. Draw Player ---
  frame[playerY][playerX] = 1;

  // --- 4. Draw Cross if Button Pressed ---
  if (buttonPressed) {
    // Draw Top (Head)
    if (playerY > 0) frame[playerY - 1][playerX] = 1;

    // Draw Left Arm
    if (playerX > 0) frame[playerY][playerX - 1] = 1;

    // Draw Right Arm
    if (playerX < 11) frame[playerY][playerX + 1] = 1;

    // Draw Bottom (Body) - Short Leg
    if (playerY < 7) frame[playerY + 1][playerX] = 1;
    
    // Draw Bottom (Feet) - Long Leg Extension
    // We check if Y < 6 because we are adding +2 to the current Y
    if (playerY < 6) frame[playerY + 2][playerX] = 1; 
  }

  // --- 5. Render to Screen ---
  matrix.renderBitmap(frame, 8, 12);
}