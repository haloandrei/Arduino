/** receiver.ino – ESP-NOW → MG995 servo, IDF-v5 */
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

/* --------------- user settings ---------------- */
constexpr uint8_t   WIFI_CHANNEL = 1;          // must match transmitter
constexpr gpio_num_t SERVO_PIN   = GPIO_NUM_18;
/* ---------------------------------------------- */

Servo servo;

void onReceive(const esp_now_recv_info_t*,
               const uint8_t* data, int len)
{
  if (len == 1) {
    uint8_t angle = data[0];                 // 0–180 °
    angle = constrain(angle, 0, 180);
    servo.write(angle);

    /* ← NEW: print the received value */
    Serial.print("Received angle: ");
    Serial.println(angle);
  }
}

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  /* lock to the same channel before ESP-NOW init */
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(100);
  }
  esp_now_register_recv_cb(onReceive);

  /* Servo PWM (50 Hz, 500–2500 µs) */
  servo.setPeriodHertz(50);
  servo.attach(SERVO_PIN, 500, 2500);        // min/max pulse µs

  Serial.print("Remote ready – my MAC: ");
  Serial.println(WiFi.macAddress());
}

void loop()
{
  /* nothing – all work happens in onReceive() */
}
