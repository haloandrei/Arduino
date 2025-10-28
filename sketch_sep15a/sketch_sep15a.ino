#define LED_PIN 13   // Relay control pin
#define PRESSURE_PIN 14  // Input pin connected to the pressure plate

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PRESSURE_PIN, INPUT);
  digitalWrite(LED_PIN, HIGH); // Initialize the relay to be OFF
}

void loop() {
  if (digitalRead(PRESSURE_PIN) == LOW) { // Check if the pressure plate is pressed
    Serial.println("turn on led");
    for (int i=1; i<=7; i++) {
      digitalWrite(LED_PIN, LOW); // Turn ON the LED (relay ON)
      delay(500); // Keep the LED on for 0.5 seconds
      digitalWrite(LED_PIN, HIGH); // Turn OFF the LED (relay OFF)
      delay(500); // Close the LED on for 0.5 seconds 
    }
    // Wait until the pressure plate is released before resetting
    while (digitalRead(PRESSURE_PIN) == LOW) {
      delay(10); // Delay to prevent bouncing issues
    }
  }
}
