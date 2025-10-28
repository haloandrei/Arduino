#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 38   // Pin where the RGB LED is connected.
#define LED_COUNT 1  // We are controlling a single LED.

// Create an instance for the Neopixel.
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Define the structure for the incoming RGB data.
typedef struct rgb_message {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} rgb_message;

// Updated callback function with the new signature.
void onReceiveData(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  if (len != sizeof(rgb_message)) {
    Serial.println("Received message size mismatch");
    return;
  }
  
  rgb_message msg;
  memcpy(&msg, data, sizeof(msg));
  Serial.printf("Received: R=%d, G=%d, B=%d\n", msg.r, msg.g, msg.b);
  
  // Set the RGB LED color.
  strip.setPixelColor(0, strip.Color(msg.r, msg.g, msg.b));
  strip.show();
}

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial connection (if needed)

  // Initialize the Neopixel.
  strip.begin();
  strip.show(); // Turn off LED initially

  // Set Wi-Fi to station mode for ESP‑NOW operation.
  WiFi.mode(WIFI_STA);

  // Initialize ESP‑NOW.
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP‑NOW");
    return;
  }

  // Register the receive callback function with the updated signature.
  esp_now_register_recv_cb(onReceiveData);
  
  Serial.println("Receiver is ready and waiting for commands...");
}

void loop() {
  // The loop can remain empty since actions are handled in the callback.
}
