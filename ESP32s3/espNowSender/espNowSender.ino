#include <WiFi.h>
#include <esp_now.h>

// Define a structure for the RGB command
typedef struct rgb_message {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} rgb_message;

rgb_message myData;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial port to connect (if needed)
  
  // Set Wi-Fi to station mode for ESP‑NOW operation.
  WiFi.mode(WIFI_STA);
  
  // Initialize ESP‑NOW.
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP‑NOW");
    return;
  }

  // Add a broadcast peer – send to all devices.
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  // use current channel
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add broadcast peer");
    return;
  }
  
  Serial.println("ESP‑NOW Initialized. Type a command (red, green, blue, off) and press ENTER:");
}

void loop() {
  // Check if a serial command is available.
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();
    
    // Set the command based on the user input.
    if (input == "red") {
      myData.r = 255; myData.g = 0; myData.b = 0;
    } else if (input == "green") {
      myData.r = 0; myData.g = 255; myData.b = 0;
    } else if (input == "blue") {
      myData.r = 0; myData.g = 0; myData.b = 255;
    } else if (input == "off") {
      myData.r = 0; myData.g = 0; myData.b = 0;
    } else {
      Serial.println("Unknown command. Please type red, green, blue, or off.");
      return;
    }

    // Send the RGB command via ESP‑NOW.
    esp_err_t result = esp_now_send(NULL, (uint8_t *)&myData, sizeof(myData));
    if (result == ESP_OK) {
      Serial.println("Command sent successfully.");
    } else {
      Serial.print("Error sending command: ");
      Serial.println(result);
    }
  }
}
