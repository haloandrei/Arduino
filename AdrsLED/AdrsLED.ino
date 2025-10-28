#include <Adafruit_NeoPixel.h>

#define LED_PIN 2            // GPIO pin connected to the LED strip
#define NUM_LEDS 280          // Total number of LEDs in the strip

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
int n, max1, poz, nr;
void setup() {
  strip.begin();             // Initialize the LED strip
  strip.show();              // Turn off all LEDs initially
  strip.setBrightness(50);   // Adjust brightness (0 to 255)
  
  Serial.begin(115200);
  Serial.println("LEDs ready to be controlled by index.");
}

void aprinde_becul(int i, int red, int green, int blue){
  strip.setPixelColor(i, strip.Color(red, green, blue)); // Set LED at index i to red (RGB: 255,0,0)
  strip.show();                                   // Update the LEDs to apply the change
}
void inchide_becul(int i){
  strip.setPixelColor(i, strip.Color(0, 0, 0));
  strip.show();
}

int prim(int n){
    int eprim = 1;
    for (int i = 2; i < n ; i++)
        if (n % i == 0)
            eprim = 0;
    return eprim;
}

void loop() {



  int a = 8, b = 30;
  // for(int i = 1; i <= b; i++)
  //   {
  //       aprinde_becul(a*i, 255, 255, 255);
  //   }  
  //   delay(10000); // Wait a second before turning off all LEDs
  //   for (int i = 0; i < NUM_LEDS; i++) {
  //     inchide_becul(i); // Turn off all LEDs
  //   }


  //aprinde_becul(80, 255,0,0);
  // for (int j = 1; j <= 7; j++)
  // {
  // for (int i = 0; i < NUM_LEDS/2; i+=4)  
  //   aprinde_becul(i, 0, 0, 250);
  // delay(1000);
  // for (int i = 0; i < NUM_LEDS/2; i+=4)  
  //   aprinde_becul(i, 0, 0, 0);
  // delay(100);
  // }
  // for (int i = NUM_LEDS; i > NUM_LEDS/3*2; i--)  
  //    aprinde_becul(i, 255, 0, 0);
  // for (int i = NUM_LEDS/3*2; i > NUM_LEDS/3; i--)  
  //    aprinde_becul(i, 255, 255, 0);
  // for (int i = NUM_LEDS/3; i > 0; i--)  
  //    aprinde_becul(i, 0, 0, 255);
  // delay(1000);
  for (int i = 0; i < NUM_LEDS; i++) {
        inchide_becul(i); // Turn off all LEDs
   }

  // for (int i = 0; i < NUM_LEDS; i++) {
  //      inchide_becul(i); // Turn off all LEDs
  // }

  delay(1000); // Short delay before the loop repeats 
}
