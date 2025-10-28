#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;
int a = 7;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello to you");
  matrix.begin();

}

void loop() {
  // matrix.loadFrame(LEDMATRIX_EMOJI_HAPPY);
  static unsigned long lastUpdateTime = 0;  // Stores the last update timestamp

  // Check if a second has passed
  if (millis() - lastUpdateTime >= 1000) {
    lastUpdateTime = millis();  // Update the last update time

    // Clear previous display and prepare for new text
    matrix.beginDraw();
    matrix.stroke(0xFFFFFF);    // Set color to white
    matrix.textScrollSpeed(65); // Set a moderate scroll speed

    // Prepare the string message
    String message = "a = " + String(a);

    // Prepare text display
    matrix.textFont(Font_5x7);  // Set font size
    matrix.beginText(0, 1, 0xFFFFFF);  // Begin text at position (0,1)
    matrix.println(message);  // Print the message with variable 'a'
    matrix.endText(SCROLL_LEFT);  // Scroll the text to the left

    matrix.endDraw();
  }
}
