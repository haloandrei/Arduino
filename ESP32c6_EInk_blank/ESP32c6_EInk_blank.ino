#include <GxEPD2_3C.h> // For three-color displays
#include <Adafruit_GFX.h> // For graphics support
#include <Fonts/FreeMonoBold9pt7b.h>

//SDA 18, SCL 23
// Select the appropriate display class and model
#define MAX_HEIGHT 122
#define MAX_WIDTH 250
GxEPD2_3C<GxEPD2_213_Z98c, GxEPD2_213_Z98c::HEIGHT> display(GxEPD2_213_Z98c(/*CS=5*/ 5, /*DC=*/ 1, /*RES=*/ 16, /*BUSY=*/ 3)); // GDEY0213Z98 122x250, SSD1680

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  while (!Serial);

  // Initialize the display
  display.init(115200); // SPI frequency

  // Clear the display
  display.setRotation(1); // Rotate for landscape/portrait
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);

  // Display a test message
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(10, 30);
    display.print("View your stats on:");
    display.setCursor(10, 60);
    display.setTextColor(GxEPD_RED);
    display.print("haloandrei.com");
    // display.fillRect(10, 90, 120,30, GxEPD_BLACK);
    // display.fillRect(10, 100, 120, 10, GxEPD_RED);
    display.fillCircle(180, 94, 22, GxEPD_BLACK);

    display.fillRect(155, 100, 50, 4, GxEPD_BLACK);
    display.fillRect(155, 84, 50, 4, GxEPD_BLACK);

    display.fillCircle(180, 94, 18, GxEPD_WHITE);
    display.fillRect(120, 88, 100, 4, GxEPD_WHITE);
    display.fillRect(120, 96, 100, 4, GxEPD_WHITE);

    display.fillCircle(180, 94, 14, GxEPD_RED);
    display.fillCircle(180, 94, 10, GxEPD_BLACK);
    
    display.fillRect(120, 92, 120, 4, GxEPD_BLACK);
    
  } while (display.nextPage());

  delay(5000); // Pause for 5 seconds
}

void loop() {
 
}
