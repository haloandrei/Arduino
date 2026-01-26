/*
 * Precision Coil Winder (Bidirectional)
 * Hardware: ESP32 + MKS SERVO42C Closed Loop Stepper
 * * INSTRUCTIONS:
 * 1. Open Serial Monitor (115200 baud).
 * 2. Type positive number (e.g., "50") to WIND.
 * 3. Type negative number (e.g., "-10") to UNWIND.
 */

// --- Pin Definitions ---
const int enPin = 25;   // Enable Pin
const int stepPin = 26; // Step Pin
const int dirPin = 27;  // Direction Pin

// --- Motor Settings ---
const int stepsPerRev = 200;  // Standard Nema 17
const int microsteps = 16;    // MUST match "MStep" on your MKS OLED!
const int speedDelay = 300;   // Lower = Faster. 300 is a safe speed.

void setup() {
  Serial.begin(115200);
  
  pinMode(enPin, OUTPUT);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);

  // Enable Motor (Lock shaft immediately to hold tension)
  digitalWrite(enPin, LOW); 
  
  Serial.println("----------------------------------------");
  Serial.println("  SMART WINDER READY");
  Serial.println("  > Type positive (e.g. 100) to WIND");
  Serial.println("  > Type negative (e.g. -10) to UNWIND");
  Serial.println("----------------------------------------");
}

void loop() {
  if (Serial.available() > 0) {
    int turns = Serial.parseInt(); // Reads "+" or "-" automatically
    
    // Ignore "0" or empty newlines
    if (turns != 0) {
      // Clear buffer
      while(Serial.available()) Serial.read(); 
      
      processMotor(turns);
    }
  }
}

void processMotor(int turns) {
  // 1. Determine Direction
  if (turns > 0) {
    digitalWrite(dirPin, HIGH); // WIND (Check if this is CW for you)
    Serial.print("WINDING >> ");
  } else {
    digitalWrite(dirPin, LOW);  // UNWIND (Reverse)
    Serial.print("UNWINDING << ");
  }

  // 2. Calculate Total Steps (Always positive for the loop)
  long pulsesPerTurn = (long)stepsPerRev * microsteps; 
  long totalPulses = (long)abs(turns) * pulsesPerTurn;

  Serial.print(abs(turns));
  Serial.println(" turns...");

  // 3. Execute Move
  for(long x = 0; x < totalPulses; x++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(speedDelay); 
    digitalWrite(stepPin, LOW);
    delayMicroseconds(speedDelay);
  }

  Serial.println("Done. Motor Locked.");
  Serial.println("Waiting for command...");
}