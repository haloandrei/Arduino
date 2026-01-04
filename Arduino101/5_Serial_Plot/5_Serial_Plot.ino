void setup() {
  Serial.begin(115200);
}

void loop() {
  static unsigned long t = 0; 
  t = millis();

  // generate a nice sine wave between -50 and 50
  float sineWave = 50 * sin(t / 500.0);

  // generate a square wave between -50 and +50
  int squareWave = (t / 500) % 2 == 0 ? 50 : -50;

  // ramp from 0 to 100 then reset
  int ramp = (t / 20) % 100;

  // print values separated by spaces → Serial Plotter draws multiple lines
  Serial.print(sineWave);
  Serial.print(" ");
  Serial.print(squareWave);
  Serial.print(" ");
  Serial.println(ramp);

  delay(20); // smooth plotting
}
