#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft); // Create a buffer (Sprite)

void setup() {
  tft.init();
  tft.setRotation(1); // 1 = Landscape
  
  // Important: Turn on the backlight manually just in case
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  tft.fillScreen(TFT_BLACK);
  
  // Create a sprite that matches the screen size (135x240)
  img.createSprite(240, 135);
}

int xPos = 0;

void loop() {
  img.fillSprite(TFT_BLACK); // Clear the buffer
  
  // Draw something dynamic
  img.fillRoundRect(xPos, 40, 60, 40, 8, TFT_MAGENTA);
  img.setTextColor(TFT_WHITE);
  img.drawString("System Rendering...", 10, 10, 4);
  
  // Push the buffer to the physical screen
  img.pushSprite(0, 0);

  xPos++;
  if (xPos > 240) xPos = -60;
  
  delay(5);
}