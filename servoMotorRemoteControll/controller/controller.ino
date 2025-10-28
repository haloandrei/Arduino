/** controller.ino – ESP-NOW transmitter + serial echo */
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

/* ---------- user settings --------------------------------- */
constexpr uint8_t receiverMAC[6] = { 0x00, 0x4B, 0x12, 0x8E, 0x9F, 0x60 };
constexpr uint8_t WIFI_CHANNEL   = 1;
constexpr gpio_num_t POT_PIN     = GPIO_NUM_34;
constexpr uint16_t   SEND_PERIOD = 20;      // ms – 50 Hz
/* ----------------------------------------------------------- */

void onSend(const uint8_t*, esp_now_send_status_t status) {
  Serial.print(F("Send: "));
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? F("OK") : F("FAIL"));
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    Serial.println(F("ESP-NOW init failed")); while (true) delay(100);
  }
  esp_now_register_send_cb(onSend);

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, receiverMAC, 6);
  peer.channel = WIFI_CHANNEL;
  peer.encrypt = false;
#if ESP_IDF_VERSION_MAJOR >= 5
  peer.ifidx   = WIFI_IF_STA;
#endif
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println(F("Peer add failed")); while (true) delay(100);
  }

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  Serial.println(F("Potentiometer transmitter ready …"));
}

void loop() {
  static uint32_t prev = 0;
  if (millis() - prev >= SEND_PERIOD) {
    prev = millis();

    uint16_t adc   = analogRead(POT_PIN);       // 0-4095
    uint8_t  angle = map(adc, 0, 4095, 0, 180); // 0-180 °

    esp_now_send(receiverMAC, &angle, 1);       // transmit

    /* NEW: also print what we just sent */
    Serial.print(F("Angle sent: "));
    Serial.println(angle);                      // 0-180
  }
}
