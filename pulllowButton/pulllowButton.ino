// Define the pins
#define LED_PIN 5
#define BUTTON_PIN 15

// The integer that changes based on the button
int ledState = 1; 

void setup() {
  pinMode(LED_PIN, OUTPUT);
  
  // Configure the button pin as a standard input
  pinMode(BUTTON_PIN, INPUT); 
  
  Serial.begin(115200);
}

void loop() {

  int buttonReading = digitalRead(BUTTON_PIN);
  if (buttonReading == 0)
  {
  digitalWrite(LED_PIN, 1);
  }
  else {
    
  digitalWrite(LED_PIN, 0);
  sleep(500);
  }
  Serial.printf("LED Integer State: %d\n", ledState);
  delay(50);
}