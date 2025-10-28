#include <Servo.h>

// Create Servo objects for each joint
Servo clawServo;   // Controls the claw open/close
Servo yawServo;    // Rotates the claw around the yaw axis
Servo pitchServo;  // Tilts the claw up and down (pitch)
Servo baseServo;   // Rotates the arm on the ground plane (base)

// Helper function to sweep a servo from a start to an end angle
void sweepServo(Servo &servo, int startAngle, int endAngle, int delayTime) {
  if (startAngle < endAngle) {
    for (int pos = startAngle; pos <= endAngle; pos++) {
      servo.write(pos);
      delay(delayTime);
    }
  } else {
    for (int pos = startAngle; pos >= endAngle; pos--) {
      servo.write(pos);
      delay(delayTime);
    }
  }
}

void setup() {
  // Attach each servo to its respective pin
  clawServo.attach(9);    // Change pin numbers as needed
  yawServo.attach(10);
  pitchServo.attach(11);
  baseServo.attach(6);

  Serial.begin(9600);
}

void loop() {
  // // Example demonstration sequence:

  // // 1. Rotate the base servo (arm rotates on the ground)
  // sweepServo(baseServo, 0, 180, 15);
  // sweepServo(baseServo, 180, 0, 15);

  // // 2. Tilt the pitch servo (claw up and down)
  // sweepServo(pitchServo, 20, 100, 10);
  // sweepServo(pitchServo, 100, 20, 10);

  // // 3. Rotate the yaw servo (rotate the claw about its axis)
  // sweepServo(yawServo, 0, 180, 15);
  // sweepServo(yawServo, 180, 0, 15);

  // // 4. Operate the claw servo (open and close)
  // // Here, 0 may represent a closed claw and 90 an open claw (adjust as needed)
  // clawServo.write(0);    
  // delay(2500);
  // clawServo.write(180);
  // delay(2500);
  // baseServo.write(0);    
  // delay(1000);
  // baseServo.write(180);
  pitchServo.write(180);
  sweepServo(yawServo, 50, 100, 15);
  sweepServo(yawServo, 150, 50, 15);
  sweepServo(baseServo, 30, 150, 15);
  sweepServo(baseServo, 150, 30, 15);
  delay(2000);

}
