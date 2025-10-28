#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;
void setup() {
  Serial.begin(115200);
  matrix.begin();
}

void matrix_begin_settings();
void matrix_end_settings();

int a = 0;
int b = 12;


void loop() {
  // Check if serial data is available
  if (Serial.available() > 0) {
    // Read the input from the serial monitor as an integer
    a = Serial.parseInt();

    // Prepare the message with the updated value of 'a'
    String message = "  a = " + String(a+b);

    matrix_begin_settings();
    matrix.println(message);  // Print the message with the current value of 'a'
    matrix_end_settings();
   
    // Prompt again for new input
    Serial.println("Enter another integer value for a:");
  }
}

void matrix_begin_settings(){
  // Clear the previous display and prepare to display the new message
    matrix.beginDraw();
    matrix.stroke(0xFFFFFF);    // Set color to white
    matrix.textScrollSpeed(75); // Set a moderate scroll speed
    matrix.textFont(Font_5x7);  // Set font size
    matrix.beginText(0, 1, 0xFFFFFF);  // Begin text at position (0,1)
}

void matrix_end_settings(){
    matrix.endText(SCROLL_LEFT);  // Scroll the text to the left
    matrix.endDraw();

    // Add a delay to prevent rapid display refresh
    delay(2000); // 2 seconds delay for viewing the result

}
