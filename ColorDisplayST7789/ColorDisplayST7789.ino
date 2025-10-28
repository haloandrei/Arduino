#include <Adafruit_GFX.h>      // Core graphics library
#include <Adafruit_ST7789.h>   // Specific library for ST7735/7789

// Define pin connections
#define TFT_CS     10 // Chip select pin
#define TFT_RST    9  // Reset pin
#define TFT_DC     8  // Data/Command pin
#define TFT_BL     6 //Backlight

float p = 3.1415926;

// Create display object
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  // Initialize the display
  tft.init(240,280, SPI_MODE0); // For 1.44" TFTs; check your display's initialization type
  tft.fillScreen(ST77XX_BLACK); // Clear the screen with black color
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 250); // Set backlight: 0 (off) to 255 (max brightness)

  Serial.begin(9600);
  Serial.print(F("Hello! ST77xx TFT Test"));

  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_RED);
  tft.setTextWrap(true);
  tft.print("Hello!!");
  Serial.print(F("Done"));
}

void loop() {
  tft.invertDisplay(true);
  delay(500);
  tft.invertDisplay(false);
  delay(500);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_RED);
  tft.setTextWrap(true);
  tft.print("Hello!!");
  delay(1500);
}
