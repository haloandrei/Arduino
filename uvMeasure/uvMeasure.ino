// Define the analog pin connected to the UV sensor OUT pin
// GPIO 34 is a safe choice as it is an input-only ADC1 pin on the ESP32
const int UV_PIN = 34;

void setup() {
  // Initialize serial communication at 115200 baud rate
  Serial.begin(115200);
  delay(1000); // Give the serial monitor a moment to connect
  
  Serial.println("--- ESP32 UV Sensor Reader Started ---");
  
  // Configure the UV pin as an input
  pinMode(UV_PIN, INPUT);
}

void loop() {
  // Read the raw ADC value (0 to 4095 for ESP32's 12-bit ADC)
  int rawValue = analogRead(UV_PIN);
  
  // Convert the raw ADC value to voltage
  // ESP32 ADC default range is 0V to ~3.3V (using 11dB attenuation)
  float voltage = (rawValue * 3.3) / 4095.0;
  
  // Output the results to the Serial Monitor
  Serial.print("Raw ADC Value: ");
  Serial.print(rawValue);
  Serial.print("  |  Calculated Voltage: ");
  Serial.print(voltage, 3); // Print with 3 decimal places
  Serial.println(" V");
  
  // Wait 1 second before the next reading
  delay(1000);
}