#include <ESP32Servo.h>

Servo myServo; 
int servoPin = 14; // Make sure this matches the wire you are using

void setup() {
  // Allocate timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 500, 2500);

  // 180 is "Full Speed Forward" for a 360 servo
  // If it doesn't spin with this, the servo is dead or unpowered.
  myServo.write(180); 
}

void loop() {
  // Do nothing, just keep spinning
}