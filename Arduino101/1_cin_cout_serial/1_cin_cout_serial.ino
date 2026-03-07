
void setup() {
  Serial.begin(115200);
  pinMode(12, OUTPUT);
}

void loop() {
 
  // for (int i=1; i<=100;i ++)
  //   {
  //     if (i%2==0)
  //     analogWrite(10, i);
  //   else
  //     analogWrite(10, 0);
  //   delay(10*i);
  //   Serial.println(i);
  //   tone
  //   }



    int x = Serial.parseInt();     // citește un număr întreg

    Serial.println("Incepe IoT-ul in Romania, cu numarul: " + String(x));
    Serial.println(x);
    if (x==1)
    digitalWrite(12, 0);
    else
     digitalWrite(12, 1);
    
}
