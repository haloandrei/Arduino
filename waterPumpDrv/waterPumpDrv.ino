// --- Pin Definitions ---
const int enablePin = 18; // ENA - PWM speed control
const int motorIN1 = 19;  // Direction pin 1
const int motorIN2 = 21;  // Direction pin 2

// --- Ramp Parameters ---
const int startPWM = 170;      // Roughly 8V on a 12V supply
const int endPWM = 255;        // Full 12V
const int rampTimeMs = 15000;  // 15 seconds

// Calculate how long to wait between each PWM increment
const int stepDelay = rampTimeMs / (endPWM - startPWM); 

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(enablePin, OUTPUT);
  pinMode(motorIN1, OUTPUT);
  pinMode(motorIN2, OUTPUT);
  
  // Ensure the pump is initially off
  digitalWrite(motorIN1, LOW);
  digitalWrite(motorIN2, LOW);
  analogWrite(enablePin, 0);
  
  // Wait a moment before starting the sequence
  delay(2000); 
  
  Serial.println("Starting 15-second pump ramp up...");
  rampPump();
  Serial.println("Pump is now running at full 12V!");
}

void loop() {
  // Your main code goes here. 
  // The pump will remain at max voltage (255) unless changed.
}

void rampPump() {
  // Set direction forward based on your wiring:
  // OUT1 (+) needs IN1 HIGH
  // OUT2 (-) needs IN2 LOW
  digitalWrite(motorIN1, HIGH);
  digitalWrite(motorIN2, LOW);
  
  // Gradually increase the PWM signal on ENA
  for (int dutyCycle = startPWM; dutyCycle <= endPWM; dutyCycle++) {
    analogWrite(enablePin, dutyCycle);
    delay(stepDelay); 
  }
}