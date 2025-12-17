struct culoare{
  int r,g,b;
};
#include <Adafruit_NeoPixel.h>

#define LED_PIN   2
#define NUM_LEDS  256        // 16 × 16 matrix
int n=16, m=16;
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Matrix size
const uint8_t WIDTH  = 16;
const uint8_t HEIGHT = 16;
enum {
  BG = 0,
  ROS = 1,
  MOV = 2,
  GALBN = 3,
  ALBSTR = 4
};

uint32_t palette[] = {
  strip.Color(0,0,0),      // BG
  strip.Color(3,0,0),  // OUT
  strip.Color(0,8,2),      // EYE
  strip.Color(2,1,0),     // MOUTH
  strip.Color(0,0,3)     // MOUTH
};

void drawMatrix(const uint8_t image[16][16], const uint32_t palette[]) {
  for (uint8_t y = 0; y < 16; y++) {
    for (uint8_t x = 0; x < 16; x++) {
      uint8_t palIndex = image[y][x];
      uint32_t col = palette[palIndex];
      strip.setPixelColor(xyToIndex(x,y), col);
    }
  }
  strip.show();
}


 uint8_t smiley[16][16] = {
  {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
  {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},

  {2,2,0,0,0,0,2,2,2,2,0,0,0,0,2,2},
  {2,2,0,0,0,0,2,2,2,2,0,0,0,0,2,2},

  {2,2,0,0,0,0,2,2,2,2,0,0,0,0,2,2},
  {2,2,0,0,0,0,2,2,2,2,0,0,0,0,2,2},

  {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
  {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},

  {2,2,2,2,0,0,0,0,0,0,0,0,2,2,2,2},
  {2,2,2,2,0,0,0,0,0,0,0,0,2,2,2,2},

  {2,2,2,2,0,0,0,0,0,0,0,0,2,2,2,2},
  {2,2,2,2,0,0,0,0,0,0,0,0,2,2,2,2},

  {2,2,2,2,0,0,2,2,2,2,0,0,2,2,2,2},
  {2,2,2,2,0,0,2,2,2,2,0,0,2,2,2,2},

  {2,2,2,2,0,0,2,2,2,2,0,0,2,2,2,2},
  {2,2,2,2,0,0,2,2,2,2,0,0,2,2,2,2},
};


// Helper: convert (x, y) to LED index for serpentine wiring
// Row 0: left -> right
// Row 1: right -> left
// Row 2: left -> right
// etc.
uint16_t xyToIndex(uint8_t x, uint8_t y) {
   y = HEIGHT - 1 - y; // invert vertically
// then use mappedY instead of y in the function

  if (y % 2 == 0) {
    // even row: wired left → right
    return y * WIDTH + x;
  } else {
    // odd row: wired right → left, so we flip x
    return y * WIDTH + (WIDTH - 1 - x);
  }
}

// Convert a number 0–63 into an RGB color.
// Each channel has only 4 intensities: 0, 85, 170, 255.
uint32_t colorFromIndex(uint8_t n) {
  // Clamp just in case
  if (n > 63) n = 63;

  // Extract base-4 RGB components:
  uint8_t r4 = (n / 16) % 4;   // 0..3
  uint8_t g4 = (n / 4)  % 4;   // 0..3
  uint8_t b4 =  n       % 4;   // 0..3

  // Map 0..3 → 0..255
  uint8_t r = r4 * 85;   // 0,85,170,255
  uint8_t g = g4 * 85;
  uint8_t b = b4 * 85;

  return strip.Color(r4, g4, b4);
}


// ---- Static pattern: walk through all pixels ----
void showStaticPattern(uint32_t red) {
  

      for (uint8_t g = 0; g < WIDTH; g++){
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
        if (x==g || y == g || y == WIDTH-g || x == WIDTH-g){
      uint16_t i = xyToIndex(x, y);
      strip.setPixelColor(i, colorFromIndex(g));
      strip.show();        // keep this here if you want to "draw" pixel by pixel
      delay(10);
      }
      // if(x == 0 || y == 0 || x == HEIGHT-1 || y == HEIGHT-1){
      // uint16_t i = xyToIndex(x, y);
      // strip.setPixelColor(i, red);
      // strip.show();        // keep this here if you want to "draw" pixel by pixel
      // delay(50);
      // }
      // else if(x==y || x+y==WIDTH-1){
      // uint16_t i = xyToIndex(x, y);
      // strip.setPixelColor(i, red);
      // strip.show();        // keep this here if you want to "draw" pixel by pixel
      // delay(50);
      // }
      // else if( x > y && x+y > WIDTH-1) {
      // uint16_t i = xyToIndex(x, y);
      // strip.setPixelColor(i, strip.Color(0,2,0));
      // strip.show();        // keep this here if you want to "draw" pixel by pixel
      // delay(50);
      // }
      //  else if( x < y && x+y < WIDTH-1) {
      // uint16_t i = xyToIndex(x, y);
      // strip.setPixelColor(i, strip.Color(0,0,3));
      // strip.show();        // keep this here if you want to "draw" pixel by pixel
      // delay(50);
      // }
      // else if( x < y && x+y > WIDTH-1) {
      // uint16_t i = xyToIndex(x, y);
      // strip.setPixelColor(i, strip.Color(2,0,1));
      // strip.show();        // keep this here if you want to "draw" pixel by pixel
      // delay(50);
      // }
      // else {
      // uint16_t i = xyToIndex(x, y);
      // strip.setPixelColor(i, strip.Color(2,1,0));
      // strip.show();        // keep this here if you want to "draw" pixel by pixel
      // delay(50);
      // }
      }
    }
  }

  // If you don't care about the drawing effect, you can move strip.show()
  // outside the loops and remove the delay for instant fill:
  // strip.show();
}

void showMatrix(uint32_t red) {
  

  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {

      uint16_t i = xyToIndex(x, y);
      strip.setPixelColor(i, red);
      strip.show();        // keep this here if you want to "draw" pixel by pixel
      delay(10);
  
    }
  }

  // If you don't care about the drawing effect, you can move strip.show()
  // outside the loops and remove the delay for instant fill:
  // strip.show();
}

void setup() {
  strip.begin();
  strip.show(); // clear
  Serial.begin(115200);
  //showStaticPattern(strip.Color(3, 0, 0));
}

void stergc(int n,int &m,uint8_t a[16][16],int c)
{ 
if(m==c)
return;
    for( int j=c;j<m-1;j++){
    for(int i=0;i<n;i++){
    a[i][j]=a[i][j+1];
    }
    }
  m--;
for(int i=0;i<n;i++){
a[i][m]=0;
}
}
void stergl(int &n, int m, uint8_t a[16][16], int l)
{ if(n==l)
return;
for(int i=l;i<n-1;i++)
for(int j=0;j<m;j++)
a[i][j]=a[i+1][j];
n--;
for(int j=0;j<m;j++)
a[n][j]=0;



}

void inserareD(int n,uint8_t a[16][16]){
  for(int i=0;i<n;i++){
    for(int j=0;j<n;j++){
      if(i==j)
      strip.setPixelColor(xyToIndex(i,j), strip.Color(10, 5, 10));
    }
  }
}

void inserareDS(int n,uint8_t a[16][16]){
  for(int i=0;i<n;i++){
    for(int j=0;j<n;j++){
      if(i==n-j-1)
            strip.setPixelColor(xyToIndex(i,j), strip.Color(0, 5, 10));

    }
  }
}

void inserareDSP(int n,uint8_t a[16][16]){
  for(int i=0;i<n;i++){
    for(int j=0;j<n;j++){
      if(i<j&&i+j>n-1)
                  strip.setPixelColor(xyToIndex(i,j), strip.Color(0, 0, 40));
                


    }
  }
  
}
void inserareDSR(int n,uint8_t a[16][16]){
  for(int i=0;i<n;i++){
    for(int j=0;j<n;j++){
      if(i<j&&i+j<n-1)
                  strip.setPixelColor(xyToIndex(i,j), strip.Color(40, 0, 0));
                


    }
  }
}
void inserareDSM(int n,uint8_t a[16][16]){
  for(int i=0;i<n;i++){
    for(int j=0;j<n;j++){
      if(i>j && i+j<n-1)
                        strip.setPixelColor(xyToIndex(i,j), strip.Color(0, 60, 0));

    }
  }
}
void inserareDSN(int n,uint8_t a[16][16]){
  for(int i=0;i<n;i++){
    for(int j=0;j<n;j++){
      if(i>j && i+j>n-1)
                        strip.setPixelColor(xyToIndex(i,j), strip.Color(40, 0, 0));

    }
  }
}

void coloreazamatricea(){
for(int i=0;i<HEIGHT;i++)
for(int j=0;j<WIDTH;j++)
     strip.setPixelColor(i*16+j, strip.Color(00, 00, 6));
    
     strip.show();
}
void coloanas()
{
  for(int i=0;i<16;i++)
  for(int j=0;j<16;j++)
    if(i+j+1==n)
      strip.setPixelColor(xyToIndex(i,j), strip.Color(60, 60, 00));
     strip.show();
     
}
void colp()
{
  for(int i=0;i<16;i++)
  for(int j=0;j<16;j++)
    if(i==j)
      strip.setPixelColor(xyToIndex(i,j), strip.Color(60, 60, 60));
     strip.show();
}
void c12(){
  for(int i=0;i<16;i++)
  for(int j=0;j<16;j++)
    if(i<j)
      strip.setPixelColor(xyToIndex(i,j), strip.Color(60,0, 00));
     strip.show();
}
void c3()
{
    for(int i=0;i<16;i++)
  for(int j=0;j<16;j++)
    if(i>j && i+j+1<16)
      strip.setPixelColor(xyToIndex(i,j), strip.Color(20,40, 00));
     strip.show();
}
bool matrice[16][16]={0};
void rec(int i, int j, culoare c)
{
   Serial.print(String(i) + " " + String(j));
  Serial.println("");
  if(i==-1 || j==-1 || i==16 || j==16 || matrice[i][j]==1) return;
  matrice[i][j]=1;
     strip.setPixelColor(xyToIndex(i,j), strip.Color(c.r,c.g,c.b));
     strip.show(); 
     delay(20);
    culoare noua=c;
    noua.g=noua.g+1;
    noua.b=noua.b+1;
   rec(i-1,j,noua);
   rec(i,j-1,noua);
   rec(i,j+1,noua);
   rec(i+1,j,noua);   
}

void loop() {
  drawMatrix(smiley, palette);
  delay(200);
  for (int i = 0; i < HEIGHT; i++)
  for (int j = 0; j < WIDTH; j++)
     strip.setPixelColor(i*16+j, strip.Color(0, 00, 0));
     strip.show();
  delay(100);



}