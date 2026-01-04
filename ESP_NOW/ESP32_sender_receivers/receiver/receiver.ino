#include <WiFi.h>
#include <esp_now.h>

#define LED_PIN 2   // onboard LED on many ESP32 devkits (often labeled D2)z

// Simple message: just the blink count
typedef struct __attribute__((packed)) {
  uint8_t count;
} Message;

volatile uint8_t pendingBlinkCount = 0;
volatile bool hasNewCommand = false;

// ESP-NOW receive callback
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Serial.print("[RECEIVER] Packet received from: ");
  for (int i = 0; i < 6; i++) {
    if (i) Serial.print(":");
    Serial.print(mac[i], HEX);
  }
  Serial.println();

  if (len != sizeof(Message)) {
    Serial.print("[RECEIVER] Invalid message size: ");
    Serial.println(len);
    return;
  }

  Message msg;
  memcpy(&msg, incomingData, sizeof(Message));
  Serial.print("[RECEIVER] Blink command: ");
  Serial.println(msg.count);

  pendingBlinkCount = msg.count;
  hasNewCommand = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // ESP-NOW needs station mode
  WiFi.mode(WIFI_STA);

  // Print this board's MAC so you can put it in the sender list
  Serial.print("[RECEIVER] My MAC address: ");
  Serial.println(WiFi.macAddress());  // e.g. "24:6F:28:AA:BB:CC"

  if (esp_now_init() != ESP_OK) {
    Serial.println("[RECEIVER] Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("[RECEIVER] Ready. Waiting for ESP-NOW messages...");
}

void loop() {
  // Handle blink command outside of the ESP-NOW callback
  if (hasNewCommand) {
    uint8_t times = pendingBlinkCount;
    hasNewCommand = false;

    Serial.print("[RECEIVER] Blinking LED ");
    Serial.print(times);
    Serial.println(" times");

    for (uint8_t i = 0; i < times; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);
      delay(200);
    }

    Serial.println("[RECEIVER] Done blinking");
  }
}
