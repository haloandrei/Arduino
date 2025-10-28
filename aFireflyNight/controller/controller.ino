/**
 * touchpad_controller.ino
 * 6-key touch-pad → 6 independent ESP-NOW peers
 * Each key press toggles that peer’s LED.
 *
 * Works with Arduino-ESP32 core ≥ 2.0.14 (IDF v5).
 */
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

/* -------------- USER SETTINGS ------------------------------ */
constexpr uint8_t peerMAC[6][6] = {          // replace with real MACs
  {0x78,0x1C,0x3c,0xa9,0xda,0x60},           // key #1
  {0x00,0x4B,0x12,0x8e,0xc2,0xec},           // key #4
  {0x00,0x4B,0x12,0x8e,0x9f,0x60},           // key #2
  {0xEC,0x64,0xC9,0x5D,0x7E,0xC8},           // key #5
  {0x68,0x25,0xDD,0x37,0x13,0x24},           // key #7
  {0x24,0x6F,0x28,0xAA,0xBB,0x06}            // key #6
};

constexpr uint8_t  WIFI_CHANNEL = 1;         // must match every receiver

/* GPIOs that the 6-key module presents to the ESP32
   (the demo board in your previous sketch used these pins already) */
constexpr gpio_num_t BUTTON_PIN[6] = {GPIO_NUM_15, 
                                      GPIO_NUM_26, GPIO_NUM_25, GPIO_NUM_27, GPIO_NUM_14, GPIO_NUM_33};

constexpr uint16_t  DEBOUNCE_MS = 120;       // touch-module already filters,
                                             // but we add a small debounce
/* ------------------------------------------------------------ */

/* internal state */
bool        toggleState[6]   = {false,false,false,false,false,false};
uint32_t    lastPressMS[6]   = {0};          // per-key debounce timers

/* ------------------------------------------------------------ */
void onSend(const uint8_t *mac, esp_now_send_status_t s)
{
  Serial.printf("Send to %02X:%02X:%02X:%02X:%02X:%02X : %s\n",
                mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],
                s==ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

/* ------------------------------------------------------------ */
void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  /* lock to channel before ESP-NOW starts */
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(100);
  }
  esp_now_register_send_cb(onSend);

  /* add six peers */
  for (int i = 0; i < 6; ++i) {
    esp_now_peer_info_t p{};
    memcpy(p.peer_addr, peerMAC[i], 6);
    p.channel = WIFI_CHANNEL;
    p.encrypt = false;
#if ESP_IDF_VERSION_MAJOR >= 5
    p.ifidx = WIFI_IF_STA;
#endif
    if (esp_now_add_peer(&p) != ESP_OK)
      Serial.printf("Failed to add peer %d\n", i+1);
  }

  /* configure the six key outputs from your touch-pad board */
  for (auto pin : BUTTON_PIN) pinMode(pin, INPUT_PULLUP);  // active-LOW

  Serial.println("Six-key touch controller ready!");
}

/* ------------------------------------------------------------ */
void loop()
{
  uint32_t now = millis();

  for (int k = 0; k < 6; ++k) {

    /* the touch-pad pulls its output high while being touched */
    bool pressed = (digitalRead(BUTTON_PIN[k]) == HIGH);

    if (pressed && (now - lastPressMS[k] > DEBOUNCE_MS)) {

      lastPressMS[k] = now;
      toggleState[k] = !toggleState[k];          // flip local state

      /* send single byte payload to that peer only */
      uint8_t payload = toggleState[k];
      esp_err_t err = esp_now_send(peerMAC[k], &payload, 1);

      Serial.printf("Key %d pressed → newState=%u, esp_now_send=%s\n",
                    k+1, payload, err==ESP_OK ? "queued" : "err");
    }
  }

  delay(10);   // ~100 Hz scan rate is plenty
}
