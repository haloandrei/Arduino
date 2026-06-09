// Define the ESP32 pins connected to the DRV8833
const int pinIN1 = 18; 
const int pinIN2 = 19; 

void setup() {
  // Configure the motor control pins as outputs
  pinMode(pinIN1, OUTPUT);
  pinMode(pinIN2, OUTPUT);

  // Ensure the motor is turned OFF when the ESP32 first boots up
  digitalWrite(pinIN1, LOW);
  digitalWrite(pinIN2, LOW);
}

void loop() {
  // 1. PUSH WATER (Motor ON)
  // Setting IN1 HIGH and IN2 LOW spins the motor forward at full speed
  digitalWrite(pinIN1, HIGH);
  digitalWrite(pinIN2, LOW);
  
  // Wait for 3 seconds (3000 milliseconds)
  delay(3000); 

  // 2. STOP (Motor OFF)
  // Setting both pins LOW stops the motor
  digitalWrite(pinIN1, LOW);
  digitalWrite(pinIN2, LOW);
  
  // Wait for 3 seconds before repeating the cycle
  // (You can change this delay if you want a shorter or longer pause)
  delay(3000); 
}