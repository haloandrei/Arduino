#include <Adafruit_NeoPixel.h>

// --- CONFIGURARE DIMENSIUNI MASA (Problema) ---
// Modifica aici valorile pentru a testa (Exemplu: 8 si 3)
const int LUNGIME_M = 30;  // a
const int LATIME_M  = 7;  // b

// --- CONFIGURARE HARDWARE ---
#define PIN         12
#define PANELS      3
#define PANEL_W     16
#define PANEL_H     16
#define BRIGHTNESS  10  // Luminozitate scazuta pentru a nu orbi

// Dimensiuni fizice matrice
const uint8_t WIDTH  = 48; 
const uint8_t HEIGHT = 16;
const uint16_t NUM_LEDS = WIDTH * HEIGHT;

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// --- CULORI ---
uint32_t cBorder = strip.Color(0, 0, 50);    // Albastru inchis
uint32_t cBall   = strip.Color(255, 0, 0);   // Rosu
uint32_t cTrail  = strip.Color(0, 20, 0);    // Verde pal (urma)
uint32_t cCorner = strip.Color(255, 255, 0); // Galben (colt final)

// --- FUNCTIE MAPARE FIZICA (Vertical Topology) ---
uint16_t xyToIndex(uint8_t x, uint8_t y) {
  if (x >= WIDTH || y >= HEIGHT) return 0;
  uint8_t panel = x / PANEL_W;
  uint8_t lx = x % PANEL_W;
  uint8_t ly = y;

  uint16_t colBase = lx * PANEL_H; 
  uint16_t localIndex;
  // Vertical Serpentine Logic
  if (lx % 2 == 0) localIndex = colBase + ly;         
  else             localIndex = colBase + (PANEL_H - 1 - ly);

  return (panel * 256) + localIndex;
}

// Functie pentru desenarea bordurii mesei
// O desenam cu un offset de 1 pixel ca sa nu fie lipita de marginea matricei fizice
void deseneazaMasa(int L, int l) {
  strip.clear();
  
  // Desenam dreptunghiul (Bordura)
  // Coordonate fizice: de la (0,0) pana la (L, l)
  // Vom folosi coordonate reale pe matrice, inversand Y-ul ca (0,0) sa fie jos-stanga vizual
  
  for (int x = 0; x <= L; x++) {
    for (int y = 0; y <= l; y++) {
      // Daca este pe margine
      if (x == 0 || x == L || y == 0 || y == l) {
        // Convertim Y logic (0 jos) in Y fizic (matricea are 0 sus de obicei, dar depinde de montaj)
        // Aici presupunem Y=0 fizic este sus. Ca sa fie "jos-stanga", inversam: HEIGHT - 1 - y
        // Dar pentru simplitate vizuala, desenam direct.
        strip.setPixelColor(xyToIndex(x + 1, y + 1), cBorder); 
      }
    }
  }
  strip.show();
}

void animatieBiliard(int a, int b) {
  Serial.println("--- INCEPUT SIMULARE ---");
  Serial.print("Dimensiuni: "); Serial.print(a); Serial.print(" x "); Serial.println(b);
  
  // Verificam sa incapa pe matrice (fizic avem 48x16)
  if (a > 46 || b > 14) {
    Serial.println("Eroare: Dimensiunile mesei sunt prea mari pentru matricea fizica!");
    return;
  }

  deseneazaMasa(a, b);
  delay(1000);

  // Coordonate logice bila (Porneste din stanga jos 0,0)
  // Dar in problema "porneste DIN colt", deci prima miscare e spre (1,1)
  int x = 0;
  int y = 0;
  
  // Directie (45 grade inseamna ca x si y cresc cu 1)
  int dx = 1;
  int dy = 1;
  
  long nrSchimb = 0;
  bool sStop = false;

  // Afisam pozitia de start
  // Offset +1 pentru a fi in interiorul bordurii desenate anterior
  strip.setPixelColor(xyToIndex(x + 1, y + 1), cBall);
  strip.show();
  delay(500);

  while (!sStop) {
    // 1. Stergem bila veche (lasam urma)
    strip.setPixelColor(xyToIndex(x + 1, y + 1), cTrail);

    // 2. Calculam urmatoarea pozitie
    x += dx;
    y += dy;

    // 3. Verificam loviturile (Ricoșeu)
    bool hitX = false;
    bool hitY = false;

    // Lovire perete vertical (Dreapta sau Stanga)
    if (x == a || x == 0) {
      dx = -dx; // Schimba directia orizontala
      hitX = true;
    }

    // Lovire perete orizontal (Sus sau Jos)
    if (y == b || y == 0) {
      dy = -dy; // Schimba directia verticala
      hitY = true;
    }

    // 4. Logica numarare si Stop
    if (hitX && hitY) {
      // A lovit AMBELE laturi simultan => COLT!
      // Problema spune: "Se opreste intr-un colt".
      sStop = true;
      Serial.println("-> BILA A INTRAT IN COLT!");
    } 
    else if (hitX || hitY) {
      // A lovit doar una => Ricoșeu
      nrSchimb++;
      // Flash scurt alb la impact
      strip.setPixelColor(xyToIndex(x + 1, y + 1), strip.Color(100, 100, 100));
      strip.show();
      delay(50);
    }

    // 5. Desenam bila noua (daca nu s-a oprit, sau bila finala galbena)
    if (sStop) {
       strip.setPixelColor(xyToIndex(x + 1, y + 1), cCorner);
    } else {
       strip.setPixelColor(xyToIndex(x + 1, y + 1), cBall);
    }
    
    strip.show();
    delay(200); // Viteza animatiei
  }

  Serial.print("Numar Total Schimbari Directie: ");
  Serial.println(nrSchimb);
  
  // Afisare matematica pentru verificare (LCM formula)
  // Formula: (LCM(a,b)/a) + (LCM(a,b)/b) - 2
  // Nu implementam LCM aici, dar pentru 8 si 3: (24/8 + 24/3 - 2) = 3 + 8 - 2 = 9. Corect.
}

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();

  // Rulam animatia o data
  animatieBiliard(LUNGIME_M, LATIME_M);
}

void loop() {
  // Nu facem nimic in loop, dam reset la placa daca vrem sa revedem
}