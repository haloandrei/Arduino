#include <ESP32Servo.h>

Servo servoX; // 180 Degree Standard
Servo servoY; // 360 Degree Continuous

// --- PINS ---
const int pinServoX = 13;
const int pinServoY = 14;
const int pinJoyX   = 32; // Analog Pin
const int pinJoyY   = 33; // Analog Pin
const int pinJoySW  = 27; // Switch Pin (Digital)
const int pinLaser = 2;

// --- SETTINGS X (180 Deg) ---
int posX = 90;           // Start at center
const int stepAngle = 2; // Speed of movement

// --- SETTINGS Y (360 Deg) ---
const int Y_STOP = 90;       // Stop Value (Calibrate if drifting)
const int Y_SPEED_FWD = 95;  // Speed Up
const int Y_SPEED_REV = 85;  // Speed Down

// --- TIMING ---
unsigned long lastUpdate = 0;
const int updateInterval = 20; // 50Hz update rate

void setup() {
  Serial.begin(115200);
  
  // Configure the button with internal pull-up resistor
  // This means the pin reads HIGH when open, and LOW when pressed.
  pinMode(pinJoySW, INPUT_PULLUP);
  pinMode(pinLaser, OUTPUT);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servoX.setPeriodHertz(50);
  servoY.setPeriodHertz(50);
  
  servoX.attach(pinServoX, 500, 2500);
  servoY.attach(pinServoY, 500, 2500);

  // Initial Home Position
  servoX.write(posX);
  servoY.write(Y_STOP);
  
  Serial.println("--- System Ready ---");
}

void loop() {
  // 1. CHECK RESET BUTTON FIRST
  // The switch connects to Ground, so LOW means pressed.
  if (digitalRead(pinJoySW) == LOW) {
    Serial.println(">>> FIRE ACTIVATED <<<");
    digitalWrite(2, HIGH);
    
    delay(500); 
  }
  else digitalWrite(2, LOW);

  // 2. MOVEMENT LOGIC
  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    int valX = analogRead(pinJoyX);
    int valY = analogRead(pinJoyY);

    // --- X-AXIS (Position) ---
    // Move Left
    if (valX < 1000) {
      posX -= stepAngle;
      if (posX < 0) posX = 0;
      servoX.write(posX);
    }
    // Move Right
    else if (valX > 3000) {
      posX += stepAngle;
      if (posX > 180) posX = 180;
      servoX.write(posX);
    }

    // --- Y-AXIS (Continuous Speed) ---
    // Move Forward
    if (valY < 1000) {
      servoY.write(Y_SPEED_FWD);
    }
    // Move Backward
    else if (valY > 3000) {
      servoY.write(Y_SPEED_REV);
    }
    // Stop (Deadzone)
    else {
      servoY.write(Y_STOP);
    }
  }
}