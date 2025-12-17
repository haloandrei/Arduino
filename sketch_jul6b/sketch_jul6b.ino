// Define the GPIO pin where the LED is connected
#define LED_PIN 2

void setup() {
  // Initialize the LED_PIN as an output
  pinMode(LED_PIN, OUTPUT);
  
  // Turn off the LED initially
  for (int i = 0; i<=49; i++)
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  // // Turn the LED on (HIGH is the voltage level)
  for (int i=0; i<=2; i++)

  {
  digitalWrite(LED_PIN, HIGH);
  delay(200); // Wait for a second

  // Turn the LED off by making the voltage LOW
  digitalWrite(LED_PIN, LOW);
  delay(200); // Wait for a second

  }

for (int i=0; i<=2; i++)

  {
  digitalWrite(LED_PIN, HIGH);
  delay(200); // Wait for a second

  // Turn the LED off by making the voltage LOW
  digitalWrite(LED_PIN, LOW);
  delay(400); // Wait for a second

  }
}
