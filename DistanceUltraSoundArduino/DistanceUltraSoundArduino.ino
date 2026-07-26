// Define the pins for the ultrasonic sensor
#define TRIG_PIN 9
#define ECHO_PIN 10

// The speed of sound in cm/microsecond
#define SOUND_SPEED 0.0343
 float distance1, distance2,distance3;
void faBip(int distanta)
{
 analogWrite(6,200/distanta);
 delay(10*distanta);
 analogWrite(6,0);
 delay(10*distanta);
}
void setup() {
  // Initialize the serial monitor at standard Arduino speed
  Serial.begin(9600);
  pinMode(6, OUTPUT);
  // Set the pin modes
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {

  long duration;
  float distanceCm;

  // 1. Clear the TRIG pin to ensure a clean pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // 2. Trigger the sensor by sending a HIGH pulse for 10 microseconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // 3. Read the ECHO pin. The pulseIn() function returns the time in microseconds
  duration = pulseIn(ECHO_PIN, HIGH);
  
  // 4. Calculate the distance
  // Multiply the travel time by the speed of sound, then divide by 2 (round trip)
  distanceCm = (duration * SOUND_SPEED) / 2;
  int distmedie=(distance1+distance2+distance3+distanceCm)/4;
if(distmedie<20) faBip(distmedie);  
  // 5. Display the result in the Serial Monitor
  Serial.print("Distance: ");
  Serial.print(distmedie);
  Serial.println(" cm");
  Serial.print(distance1);
  Serial.println(" ");
   Serial.print(distance2);
  Serial.println(" ");
   Serial.print(distance3);
  Serial.println(" ");
  distance1=distance2;
  distance2=distance3;
  distance3=distanceCm;
  // Wait half a second before taking the next reading
  delay(500);
}