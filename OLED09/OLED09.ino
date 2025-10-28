#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//GND to GND (Ground)
//VDD to 5V or 3.3V (depending on your screen’s voltage requirements)
//SCK to A5 (SCL on Arduino Uno, Mega, etc.)
//SDA to A4 (SDA on Arduino Uno, Mega, etc.)

// Declaration for an SSD1306 display connected to I2C (SCL, SDA pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(9600);

  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Check the address with an I2C scanner if unsure
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0,0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
}


void loop() {
  display.clearDisplay();
  display.setTextSize(1);  
  display.setCursor(20,0); // Sets starting point for text
  display.println(F("Hello, world!"));
  display.display(); // Show initial text
  delay(2000);

  // You can add graphics or more text here
  display.setTextSize(2); // Make text bigger
  display.setCursor(0,10);
  display.println(F("Testing!"));
  display.display(); // Update display with new text
  delay(2000);
}

