/********************************************************************
  ESP32 Six-Button Hand-Controller – ESP-NOW Edition
    • Each press sends one byte: 1-6
    • Only peers with MAC 00:4B:12:8E:C2:EC are accepted
    • Same GPIO assignment as your 6-key breakout
    • Debounce 50 ms
*********************************************************************/

#include <WiFi.h>
#include <esp_now.h>

// ------------------------------------------------------------
// Button → GPIO mapping
// ------------------------------------------------------------
constexpr gpio_num_t BUTTON_PIN[6] = {
  GPIO_NUM_15,  // #1  (white flash)
  GPIO_NUM_26,  // #2  (flash pattern)
  GPIO_NUM_25,  // #3  (pink glow)
  GPIO_NUM_27,  // #4  (tri-color fade)
  GPIO_NUM_14,  // #5  (cancel / off)
  GPIO_NUM_33   // #6  (warm orange)
};

// ------------------------------------------------------------
// Target peer MAC (receiver)
// ------------------------------------------------------------
uint8_t PEER_MAC[6] = {0x00, 0x4B, 0x12, 0x8E, 0xC2, 0xEC};

// ------------------------------------------------------------
// Debounce bookkeeping
// ------------------------------------------------------------
static bool           rawState[6];
static bool           stableState[6];
static bool           lastRawState[6];
static unsigned long  lastDebounce[6];
const unsigned long   DEBOUNCE_MS = 50;

// ------------------------------------------------------------
// Forward declarations
// ------------------------------------------------------------
void sendCommand(uint8_t btnIndex);
void addPeer();

// ============================================================
// SETUP
// ============================================================
void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("=== Six-Button Controller – ESP-NOW ==="));

  // 1. Configure buttons
  for (uint8_t i = 0; i < 6; ++i) {
    pinMode(BUTTON_PIN[i], INPUT_PULLUP);
    rawState[i]       = digitalRead(BUTTON_PIN[i]);
    stableState[i]    = rawState[i];
    lastRawState[i]   = rawState[i];
    lastDebounce[i]   = millis();
  }

  // 2. Wi-Fi must be in station mode for ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();            // no AP

  // 3. Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init FAILED – controller can’t run");
    while (true) { delay(1000); }
  }

  // 4. Register peer (receiver)
  addPeer();

  // 5. Optional – show our own MAC for troubleshooting
  Serial.print  ("Controller MAC: ");
  Serial.println(WiFi.macAddress());
}

// ============================================================
// MAIN LOOP
// ============================================================
void loop()
{
  unsigned long now = millis();

  // Handle each button with simple debounce
  for (uint8_t i = 0; i < 6; ++i) {
    rawState[i] = digitalRead(BUTTON_PIN[i]);

    if (rawState[i] != lastRawState[i]) {
      lastDebounce[i] = now;
    }

    if ((now - lastDebounce[i]) > DEBOUNCE_MS) {
      if (rawState[i] != stableState[i]) {
        stableState[i] = rawState[i];
        if (stableState[i] == LOW) {           // active LOW
          Serial.printf("Button %u pressed\n", i + 1);
          sendCommand(i);                      // 0…5 → 1…6
        }
      }
    }
    lastRawState[i] = rawState[i];
  }

  delay(3);                                   // idle
}

// ============================================================
// ESP-NOW helpers
// ============================================================

// Add a single peer (receiver)
void addPeer()
{
  esp_now_peer_info_t peerInfo{};
  memcpy(peerInfo.peer_addr, PEER_MAC, 6);
  peerInfo.channel = 0;           // same channel as controller (auto)
  peerInfo.encrypt = false;       // encryption not required here

  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.print  ("Peer added: ");
    for (int i = 0; i < 6; ++i) {
      Serial.printf("%02X", PEER_MAC[i]);
      if (i < 5) Serial.print(':');
    }
    Serial.println();
  } else {
    Serial.println("!!! Failed to add peer");
    while (true) { delay(1000); }
  }
}

// Send one byte: 1…6
void sendCommand(uint8_t btnIndex)
{
  uint8_t cmd = btnIndex + 1;  // 1-6
  esp_err_t result = esp_now_send(PEER_MAC, &cmd, sizeof(cmd));

  if (result == ESP_OK) {
    Serial.printf("  Sent %u OK\n", cmd);
  } else {
    Serial.printf("  Send ERROR (%d)\n", result);
  }
}
