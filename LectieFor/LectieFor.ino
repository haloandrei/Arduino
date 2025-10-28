#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

int ledPin = 13;
int button = 2;
ArduinoLEDMatrix matrix;

void setup() {
  matrix.begin();
  Serial.begin(115200);    
}



void loop() {
 matrix.loadSequence(LEDMATRIX_ANIMATION_BOUNCING_BALL);
  matrix.begin();
  matrix.play(true);
}
