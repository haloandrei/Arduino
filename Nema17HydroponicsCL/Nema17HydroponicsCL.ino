/*
 * Hydroponic Water Pump Controller
 * Hardware: ESP32 + MKS SERVO42C Closed Loop Stepper
 * Behavior: Runs for 10 minutes at 3x speed, pauses for 20 minutes.
 */

// --- Pin Definitions ---
const int enPin = 25;   // Enable Pin
const int stepPin = 26; // Step Pin
const int dirPin = 27;  // Direction Pin

// --- Timing & Motor Settings ---
// Original was 300. 100 is three times faster (less delay between pulses).
const int speedDelay = 100; 

// Calculate milliseconds (UL = Unsigned Long to prevent math overflow)
const unsigned long runTime = 10UL * 60UL * 1000UL;   // 10 minutes 
const unsigned long pauseTime = 20UL * 60UL * 1000UL; // 20 minutes 

void setup() {
  pinMode(enPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);

  // Set default pumping direction. 
  // Change to LOW if the water flows backwards.
  digitalWrite(dirPin, HIGH); 
}

void loop() {
  // --- 1. PUMPING PHASE (10 Minutes) ---
  digitalWrite(enPin, LOW); // Enable motor coils
  
  unsigned long startTime = millis();
  int stepsBeforeYield = 0;

  // Pulse the motor until 10 minutes have passed
  while (millis() - startTime < runTime) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(speedDelay); 
    digitalWrite(stepPin, LOW);
    delayMicroseconds(speedDelay);
    
    // Periodically feed the ESP32 Watchdog Timer so the board doesn't crash
    stepsBeforeYield++;
    if (stepsBeforeYield >= 1000) {
      yield();
      stepsBeforeYield = 0;
    }
  }

  // --- 2. RESTING PHASE (20 Minutes) ---
  // Disable motor to save power and prevent the stepper from overheating!
  digitalWrite(enPin, HIGH); 
  
  // Pause execution for 20 minutes 
  // (The ESP32's standard delay() safely handles background tasks automatically)
  delay(pauseTime);          
}