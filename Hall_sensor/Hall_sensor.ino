// Define the analog pin connected to the AO pin of the sensor
const int hallPin = 32; 

void setup() {
  Serial.begin(115200);
  
  // ESP32 analog pins don't strictly require pinMode setup, 
  // but it's good practice.
  pinMode(hallPin, INPUT);
  
  Serial.println("Hall Sensor Polarity Test Started!");
}

void loop() {
  // Read the analog value (0 to 4095)
  int sensorValue = analogRead(hallPin);
  
  // Create a "deadzone" between 1800 and 2300 to ignore minor noise
  if (sensorValue > 2300) {
    Serial.print("Polarity: SOUTH Pole detected | Value: ");
    Serial.println(sensorValue);
  } 
  else if (sensorValue < 1800) {
    Serial.print("Polarity: NORTH Pole detected | Value: ");
    Serial.println(sensorValue);
  } 
  else {
    // We are in the deadzone (roughly 2048)
    Serial.print("Neutral / No strong field   | Value: ");
    Serial.println(sensorValue);
  }
  
  // Wait a bit before reading again so we don't spam the serial monitor
  delay(200);
}