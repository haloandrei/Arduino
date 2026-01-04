#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;

void setup() {
  Serial.begin(115200);
  matrix.begin();
}

byte frame[8][12] = {
  { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0 },
  { 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
  { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 }
};

void loop(){
  matrix.renderBitmap(frame, 8, 12);
}