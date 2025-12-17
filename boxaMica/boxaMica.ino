int speakerPin = 3;

void setup() {
  // put your setup code here, to run once:
   pinMode(speakerPin, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  for(int i=0;i<40;i++)
{
  analogWrite(speakerPin, i);
  delay(100);
}
for(int i=40;i>0;i--)
{
  analogWrite(speakerPin, i);
  delay(100);

}
}
