/*****************************************************************
  ESP32 LED Receiver (ESP-NOW only, no local buttons)
  → Added Serial debug logs and interruption check in doPinkGlow()
*****************************************************************/
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

// ---------------- NeoPixel ----------------
#define LED_PIN   2
#define NUM_LEDS  256                          // 16×16 matrix
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ------------- Controller MAC -------------
const uint8_t CONTROLLER_MAC[6] = {0xEC, 0x64, 0xC9, 0x5E, 0x6A, 0x34};

// ------------- Command queue --------------
volatile uint8_t queuedCmd     = 0;            // 1-6, 0 = none
volatile bool    cancelFlag    = false;
portMUX_TYPE     mux           = portMUX_INITIALIZER_UNLOCKED;

// ------------- Short-flash helpers --------
bool          flashing       = false;
unsigned long flashStart     = 0;
const unsigned FLASH_MS      = 100;

// ------------------------------------------------
//  ESP-NOW receive callback – *new signature!*
// ------------------------------------------------
void IRAM_ATTR onDataRecv(const esp_now_recv_info_t *info,
                          const uint8_t             *data,
                          int                        len)
{
  if (len != 1) return;                       // expect single byte
  if (memcmp(info->src_addr, CONTROLLER_MAC, 6) != 0) return; // foreign packet

  uint8_t cmd = data[0];
  if (cmd < 1 || cmd > 6) return;

  portENTER_CRITICAL_ISR(&mux);
    queuedCmd = cmd;
    if (cmd == 5) {    // async cancel
      cancelFlag = true;
      // Log in ISR (will still show at 115200)
      Serial.println("[ISR] cmd=5 → cancelFlag set");
    } else {
      Serial.print("[ISR] queuedCmd set = ");
      Serial.println(cmd);
    }
  portEXIT_CRITICAL_ISR(&mux);
}

// ------------------------------------------------
//  Helper replacing old isCancelPressed()
// ------------------------------------------------
bool isCancelRequested()
{
  bool f = false;
  portENTER_CRITICAL(&mux);
    if (cancelFlag) {
      f = true;
      cancelFlag = false;
    }
  portEXIT_CRITICAL(&mux);
  if (f) {
    Serial.println("[DEBUG] isCancelRequested() == true");
  }
  return f;
}

// ------------------------------------------------
//  Forward-declared animation routines
// ------------------------------------------------
void flashMatrix(uint8_t r, uint8_t g, uint8_t b);
void doLongFlashingSequence();   // action #2
void doPinkGlow();               // action #3
void doFadingRandomBlues();      // action #4

// ============================== SETUP ==========================
void setup()
{
  Serial.begin(115200);
  delay(500);
  strip.begin(); strip.show();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();             // no AP

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    while (true) delay(1000);
  }
  esp_now_register_recv_cb(onDataRecv);

  randomSeed(millis());
  Serial.print("Receiver ready. My MAC: ");
  Serial.println(WiFi.macAddress());
}

// ============================== LOOP ===========================
void loop()
{
  /* ---------- 1. pull queued radio command ---------- */
  uint8_t cmd = 0;
  portENTER_CRITICAL(&mux);
    if (queuedCmd) {
      cmd = queuedCmd;
      queuedCmd = 0;
    }
  portEXIT_CRITICAL(&mux);

  if (cmd) {
    Serial.print("[LOOP] Processing cmd = ");
    Serial.println(cmd);
  }

  /* ---------- 2. act on it -------------------------- */
  switch (cmd) {
    case 1:  // 100-ms white flash
      if (!flashing) {
        flashing = true;
        flashStart = millis();
        Serial.println("[ACTION] Flash once (white)");
        flashMatrix(255,255,255);
      }
      break;
    case 2:
      Serial.println("[ACTION] Long flashing sequence");
      doLongFlashingSequence();
      break;
    case 3:
      Serial.println("[ACTION] Pink glow (indefinite)");
      doPinkGlow();
      break;
    case 4:
      Serial.println("[ACTION] Fading random blues");
      doFadingRandomBlues();
      break;
    case 5:
      Serial.println("[ACTION] Immediate off!");
      flashMatrix(0,0,0);
      break;
    case 6:
      Serial.println("[ACTION] Warm orange flash");
      flashMatrix(128,64,0);
      break;
  }

  /* ---------- 3. finish white flash ---------------- */
  if (flashing && millis() - flashStart >= FLASH_MS) {
    Serial.println("[FLASH END] Turning off after white flash");
    flashMatrix(0,0,0);
    flashing = false;
  }

  delay(2);
}

// ===============================================================
//  Everything below is IDENTICAL to your previous sketch
//  except isCancelPressed() → isCancelRequested()
// ===============================================================

void flashMatrix(uint8_t r, uint8_t g, uint8_t b)
{
  for (uint16_t i = 0; i < NUM_LEDS; i++)
    strip.setPixelColor(i, strip.Color(r,g,b));
  strip.show();
}

// ----------- 2. 10-s flash pattern (3-1-2 loop) ---------------
void doLongFlashingSequence()
{
  const unsigned long RUN_MS = 10000;
  unsigned long start = millis();
  int patt[] = {3,1,2}, idx = 0;
  Serial.println("[SEQ2] Starting 10s flash pattern");

  while (millis() - start < RUN_MS) {
    if (isCancelRequested()) {
      Serial.println("[SEQ2] Cancelled!");
      flashMatrix(0,0,0);
      return;
    }
    for (int i=0; i<patt[idx]; i++) {
      if (isCancelRequested()) { Serial.println("[SEQ2] Cancelled mid-pattern"); flashMatrix(0,0,0); return; }
      flashMatrix(255,255,255); delay(100);
      if (isCancelRequested()) { Serial.println("[SEQ2] Cancelled mid-pattern"); flashMatrix(0,0,0); return; }
      flashMatrix(0,0,0);       delay(100);
    }
    delay(500);
    idx = (idx+1)%3;
  }
  flashMatrix(0,0,0);
  Serial.println("[SEQ2] Finished");
}

// ---------------- 3. indefinite pink glow ----------------------
void doPinkGlow()
{
  Serial.println("[GLOW] Entering pink glow loop");
  const unsigned long DUR_MS = 5000;
  unsigned long start = millis();

  while (true) {
    // -- check for immediate commands --
    if (isCancelRequested()) {
      Serial.println("[GLOW] Cancel requested → exit");
      flashMatrix(0,0,0);
      return;
    }
    // also break on *any* new queuedCmd:
    portENTER_CRITICAL(&mux);
      bool hasCmd = (queuedCmd != 0);
    portEXIT_CRITICAL(&mux);
    if (hasCmd) {
      Serial.println("[GLOW] New CMD queued → exiting glow");
      flashMatrix(0,0,0);
      return;
    }

    // -- do the pulsing glow --
    float t = float(millis() - start) / DUR_MS;  // cycles every 5 s
    float b = sin(t * M_PI);                    // 0→1→0
    flashMatrix(uint8_t(255*b), uint8_t(20*b), uint8_t(147*b));
    delay(20);
  }
}

// --------------- 4. 10-s random blue fade ----------------------
void doFadingRandomBlues()
{
  const unsigned long RUN_MS = 10000;
  unsigned long t0 = millis();
  uint32_t pal[3] = { strip.Color(50,50,128), strip.Color(0,0,64),
                      strip.Color(119,65,119) };
  static uint32_t sC[NUM_LEDS], dC[NUM_LEDS];
  static unsigned long tBeg[NUM_LEDS], tDur[NUM_LEDS];

  Serial.println("[BLUES] Starting 10 s random blue fades");
  for (uint16_t i=0;i<NUM_LEDS;i++){
    sC[i]=dC[i]=strip.Color(0,0,0);
    tBeg[i]=millis(); tDur[i]=0;
    strip.setPixelColor(i,0);
  }
  strip.show();

  while (millis()-t0 < RUN_MS) {
    if (isCancelRequested()) {
      Serial.println("[BLUES] Cancelled!");
      flashMatrix(0,0,0);
      return;
    }
    unsigned long now = millis();
    for (uint16_t i=0;i<NUM_LEDS;i++){
      if (tDur[i]) {
        float r=float(now-tBeg[i])/tDur[i];
        if (r>=1){ r=1; tDur[i]=0; }
        auto mix=[&](uint32_t a,uint32_t b,float f){
          uint8_t ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab=a&0xFF;
          uint8_t br=(b>>16)&0xFF, bg=(b>>8)&0xFF, bb=b&0xFF;
          return strip.Color(uint8_t(ar+(br-ar)*f),
                             uint8_t(ag+(bg-ag)*f),
                             uint8_t(ab+(bb-ab)*f));
        };
        strip.setPixelColor(i, mix(sC[i], dC[i], r));
      } else if (random(120)==0) {
        sC[i] = strip.getPixelColor(i);
        dC[i] = pal[random(3)];
        tBeg[i]=now; tDur[i]=random(500,1500);
      }
    }
    strip.show();
    delay(50);
  }
  flashMatrix(0,0,0);
  Serial.println("[BLUES] Finished");
}
