#define MIC_PIN 15
int vol1, vol2, vol3;
void setup() {
  Serial.begin(115200);
}
int convert(int val)
{
  return (val*255/4095);
}
void loop() {
  int micValue = analogRead(MIC_PIN);
  int volm=(micValue+vol1+vol2+vol3)/4;
  Serial.println(volm);
  analogWrite(22, convert(volm)); 
vol1=vol2;
vol2=vol3;
vol3=micValue;
  delay(10);
}