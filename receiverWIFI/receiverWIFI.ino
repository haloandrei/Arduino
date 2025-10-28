#include <WiFi.h>
#include <esp_wifi.h>          // <-- ADD THIS
#include <esp_now.h>

/* --------------- user settings ---------------- */
const int OUTPUT_PIN   = 2;     // built-in LED by default
const uint8_t WIFI_CHANNEL = 1;
/* ---------------------------------------------- */

/* NEW-STYLE callback */
void onReceive(const esp_now_recv_info_t* info,
               const uint8_t* data, int len)
{
  if (len == 1) {
    digitalWrite(OUTPUT_PIN, data[0] ? HIGH : LOW);
  }
}

void setup() {
  pinMode(OUTPUT_PIN, OUTPUT);

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  /* lock to chosen channel before ESP-NOW init */
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(100);
  }
  esp_now_register_recv_cb(onReceive);

  Serial.print("My MAC: "); Serial.println(WiFi.macAddress());
}

void loop() {
  /* nothing -- work is done in onReceive() */
}
