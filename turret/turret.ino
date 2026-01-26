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
  
    digitalWrite(pinLaser, HIGH);
    delay(100); 
    digitalWrite(pinLaser, LOW);
   servoX.write(120);
   delay(100); 
   digitalWrite(pinLaser, HIGH);
   delay(100);
   digitalWrite(pinLaser, LOW);
   delay(100);
   servoX.write(130);
   delay(100); 
   digitalWrite(pinLaser, HIGH);
   delay(100);
   digitalWrite(pinLaser, LOW);

 
}