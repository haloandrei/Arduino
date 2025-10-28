#include <Adafruit_NeoPixel.h>
int LED = 38;

// Define the number of LEDs in the strip (usually 1 for built-in LED)
#define NUM_LEDS 1

// Create an instance of the Adafruit_NeoPixel class
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED, NEO_GRB + NEO_KHZ800);

void setup() {
// Initialize the NeoPixel library
strip.begin();
strip.show(); // Initialize all pixels to 'off'
strip.setBrightness(10); 
strip.setPixelColor(0, strip.Color(0, 155, 0)); // Red
strip.show();
  // Initialize the serial communication at 115200 baud rate
  Serial.begin(115200);
  // Wait for the serial communication to be established
  while (!Serial) {
    ; // Wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("ESP32 Serial Communication Initialized");

  digitalWrite(LED, HIGH);
}

void loop() {
  // Check if data is available to read
  if (Serial.available() > 0) {
    // Read the incoming data
    String incomingData = Serial.readString();
    // Print the incoming data
    Serial.print("Received: ");
    Serial.println(incomingData);
    // Echo the received data back to the serial monitor
    Serial.print("Echo: hello ");
    Serial.println(LED);
  }
      
strip.setPixelColor(0, strip.Color(155, 0, 0)); // Red
strip.show();
delay(1000);

// Green
strip.setPixelColor(0, strip.Color(0, 155, 0)); // Green
strip.show();
delay(1000);

// Blue
strip.setPixelColor(0, strip.Color(0, 0, 155)); // Blue
strip.show();
delay(1000);

// Yellow
strip.setPixelColor(0, strip.Color(155, 155, 0)); // Yellow
strip.show();
delay(1000);
}