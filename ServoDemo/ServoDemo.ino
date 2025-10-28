#include <Servo.h>

// Create a Servo object
Servo myServo;

void setup() {
  // Attach the servo object to pin D9
  myServo.attach(9);
  Serial.begin(9600);
}

void loop() {
  // // Sweep the servo from 0 to 180 degrees
  for (int pos = 0; pos <= 180; pos++) {
    myServo.write(pos);       // Set servo position
    delay(15);                // Wait for the servo to mov e
  }

  // Sweep the servo back to 0 degrees
  for (int pos = 180; pos >= 0; pos--) {
    myServo.write(pos);       // Set servo position
    delay(15);                // Wait for the servo to move
  }
  myServo.write(180);       // Set servo position
  delay(500); 
  myServo.write(0);       // Set servo position
  delay(500); 
}
