// Define the analog pin connected to the microphone
const int MIC_PIN = 34; 

void setup() {
  // Start the serial communication at a fast baud rate
  Serial.begin(115200);
  
  // Set the microphone pin as an input
  pinMode(MIC_PIN, INPUT);
}

void loop() {
  // Read the analog value from the microphone (0 to 4095 on ESP32)
  int micValue = analogRead(MIC_PIN);

  // Print the value to the serial port
  // Serial Plotter looks for values separated by line breaks
  Serial.println(micValue);

  // A tiny delay to keep the Serial connection stable 
  // while still sampling fast enough to see audio waveforms
  delay(2); 
}