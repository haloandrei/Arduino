// Pin where the LED is connected
const int ledPin = 8;

void setup() {
  // Set the LED pin as OUTPUT
  pinMode(ledPin, OUTPUT);
}

void loop() {
  delay(36000000);                  // Wait for 1 second
  digitalWrite(ledPin, HIGH);   // Turn the LED on
  digitalWrite(ledPin, LOW);    // Turn the LED off
  delay(100);                  // Wait for 1 second
}
