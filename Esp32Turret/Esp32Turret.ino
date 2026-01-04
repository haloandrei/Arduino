#include <ESP32Servo.h>

Servo servoX; // 180 Degree Standard Servo
Servo servoY; // 360 Degree Continuous Servo

// GPIO Pins
const int pinX = 13;
const int pinY = 14;

// --- Settings for X (180 deg) ---
int posX = 90;         // Current position tracker
const int stepAngle = 10; // How many degrees to move per press

// --- Settings for Y (360 deg) ---
const int Y_STOP = 90;       // Center "Stop" signal (Tweak if it drifts)
const int Y_SPEED_UP = 100;  // Speed for 'W' (Values > 90)
const int Y_SPEED_DOWN = 80; // Speed for 'S' (Values < 90)
const int Y_BURST = 100;     // How long to spin in milliseconds per press

void setup() {
  Serial.begin(115200);
  Serial.println("--- Hybrid Servo Control Started ---");
  Serial.println("Controls:");
  Serial.println("  A/D -> X-Axis (Position 0-180)");
  Serial.println("  W/S -> Y-Axis (Continuous Burst)");

  // Allocate timers for ESP32
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servoX.setPeriodHertz(50);
  servoY.setPeriodHertz(50);

  servoX.attach(pinX, 500, 2500);
  servoY.attach(pinY, 500, 2500);

  // Initialize
  servoX.write(posX);   // Go to center
  servoY.write(Y_STOP); // Ensure stopped
}

// Helper to move the 360 motor for a short time
void burstMoveY(int speed) {
  servoY.write(speed);   // Start moving
  delay(Y_BURST);        // Wait
  servoY.write(Y_STOP);  // Stop
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();

    // --- X AXIS LOGIC (Position Tracking) ---
    if (command == 'a') {
      posX -= stepAngle;
      if (posX < 0) posX = 0; // Constrain
      servoX.write(posX);
      Serial.print("X Position: "); Serial.println(posX);
    }
    
    else if (command == 'd') {
      posX += stepAngle;
      if (posX > 180) posX = 180; // Constrain
      servoX.write(posX);
      Serial.print("X Position: "); Serial.println(posX);
    }

    // --- Y AXIS LOGIC (Time-based Burst) ---
    else if (command == 'w') {
      Serial.println("Y: Moving Up...");
      burstMoveY(Y_SPEED_UP);
    }
    
    else if (command == 's') {
      Serial.println("Y: Moving Down...");
      burstMoveY(Y_SPEED_DOWN);
    }
  }
}