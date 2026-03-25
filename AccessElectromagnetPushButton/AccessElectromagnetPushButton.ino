#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Ticker.h> // Included for background timers

// Minimal required setup:
const char* WIFI_SSID = "Energie_D";
const char* WIFI_PASSWORD = "Infoel2025!";
const char* API_BASE_URL = "https://haloandrei.com";
const char* DEVICE_TOKEN = "BkkrGl76die6HQ1Vf6qUa-2_CtCJAoWADU8jxxuOk10";

const uint8_t RELAY_PIN = 12;
const uint8_t PIN_IN2 = 18;
const uint8_t BUTTON_PIN = 4; // Add your physical push button here

const bool API_TLS_INSECURE = true;
const char* API_ROOT_CA = R"EOF(
-----BEGIN CERTIFICATE-----
REPLACE_WITH_CA_CERT
-----END CERTIFICATE-----
)EOF";

const int DEFAULT_UNLOCK_SEC = 15;
const int MIN_UNLOCK_SEC = 1;
const int MAX_UNLOCK_SEC = 30;
const int POLL_WAIT_SEC = 20;     
const int CONNECT_TIMEOUT_MS = 7000;
const int REQUEST_TIMEOUT_MS = 30000;
const uint8_t MAX_ACK_RETRIES = 5;

String activeCommandId;
uint32_t unlockUntilMs = 0;
uint8_t consecutivePollFailures = 0;

// Variables shared between interrupts and the main loop must be volatile
volatile bool relayActive = false;
volatile bool localUnlockActive = false;
Ticker localUnlockTimer;

String buildPollUrl() {
  return String(API_BASE_URL) + "/v1/devices/poll/" + DEVICE_TOKEN + "?waitSec=" + String(POLL_WAIT_SEC);
}

String buildAckUrl() {
  return String(API_BASE_URL) + "/v1/devices/ack/" + DEVICE_TOKEN;
}

uint32_t backoffDelayMs(uint8_t attempt, uint32_t baseMs, uint32_t capMs) {
  uint32_t delayMs = baseMs;
  for (uint8_t i = 0; i < attempt; i++) {
    delayMs = delayMs >= capMs / 2 ? capMs : delayMs * 2;
  }
  if (delayMs > capMs) delayMs = capMs;
  return delayMs + (esp_random() % 150);
}

void configureTlsClient(WiFiClientSecure& client) {
  client.setTimeout(REQUEST_TIMEOUT_MS);
  if (API_TLS_INSECURE) {
    client.setInsecure();
  } else {
    client.setCACert(API_ROOT_CA);
  }
}

void ensureWifiConnected() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    if (millis() - start > 20000) {
      Serial.println("[wifi] reconnect timeout, retrying...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      start = millis();
    }
  }
  Serial.print("[wifi] connected, ip=");
  Serial.println(WiFi.localIP());
}

bool sendAck(const String& commandId, bool success, const String& errorMessage) {
  StaticJsonDocument<256> payload;
  payload["commandId"] = commandId;
  payload["success"] = success;
  if (!success && errorMessage.length() > 0) {
    payload["error"] = errorMessage;
  }

  String body;
  serializeJson(payload, body);

  for (uint8_t attempt = 0; attempt < MAX_ACK_RETRIES; attempt++) {
    ensureWifiConnected();
    WiFiClientSecure client;
    configureTlsClient(client);
    HTTPClient http;
    http.setConnectTimeout(CONNECT_TIMEOUT_MS);
    http.setTimeout(REQUEST_TIMEOUT_MS);

    if (!http.begin(client, buildAckUrl())) {
      Serial.println("[ack] begin failed");
    } else {
      http.addHeader("Content-Type", "application/json");
      int code = http.POST(body);
      http.end();
      if (code == 204 || code == 200) {
        Serial.println("[ack] success");
        return true;
      }
      Serial.printf("[ack] http code=%d\n", code);
      if (code == 401 || code == 404) {
        return false;
      }
    }
    delay(backoffDelayMs(attempt, 300, 4000));
  }
  return false;
}

// --- NEW HARDWARE INTERRUPT FUNCTIONS ---

// This function runs when the 7-second background timer finishes
void endLocalUnlock() {
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(PIN_IN2, LOW);
  relayActive = false;
  localUnlockActive = false;
}

// This runs instantly the millisecond the button is pressed
void IRAM_ATTR buttonISR() {
  static uint32_t lastPress = 0;
  uint32_t now = millis();
  
  // 300ms debounce to prevent button bounce from triggering multiple times
  if (now - lastPress > 300) { 
    lastPress = now;
    
    // Only trigger if the door isn't already unlocked
    if (!relayActive) {
      digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(PIN_IN2, LOW);
      relayActive = true;
      localUnlockActive = true;
      
      // Start a background timer for 7 seconds (7000 ms)
      localUnlockTimer.once_ms(7000, endLocalUnlock);
    }
  }
}
// ----------------------------------------

void startUnlock(uint16_t durationSec, const String& commandId) {
  durationSec = constrain(durationSec, MIN_UNLOCK_SEC, MAX_UNLOCK_SEC);
  
  // If someone presses the button, and then the server sends an unlock command
  // a second later, we cancel the local timer and let the server take over.
  if (localUnlockActive) {
    localUnlockTimer.detach();
    localUnlockActive = false;
  }

  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(PIN_IN2, LOW);
  relayActive = true;
  unlockUntilMs = millis() + (uint32_t)durationSec * 1000U;
  activeCommandId = commandId;
  Serial.printf("[relay] unlock start for %u sec\n", durationSec);
}

void finishUnlockIfDue() {
  if (!relayActive) return;
  if (localUnlockActive) return; // If unlocked by button, let the Ticker handle it
  if ((int32_t)(millis() - unlockUntilMs) < 0) return;

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(PIN_IN2, LOW);
  relayActive = false;
  Serial.println("[relay] network unlock finished");

  if (activeCommandId.length() > 0) {
    bool acked = sendAck(activeCommandId, true, "");
    if (!acked) {
      Serial.println("[relay] warning: success ack failed");
    }
    activeCommandId = "";
  }
}

bool pollOnceForCommand() {
  ensureWifiConnected();
  WiFiClientSecure client;
  configureTlsClient(client);
  HTTPClient http;
  http.setConnectTimeout(CONNECT_TIMEOUT_MS);
  http.setTimeout(REQUEST_TIMEOUT_MS);

  if (!http.begin(client, buildPollUrl())) {
    Serial.println("[poll] begin failed");
    consecutivePollFailures++;
    return false;
  }

  int code = http.GET();
  if (code == 204) {
    http.end();
    consecutivePollFailures = 0;
    return false;
  }

  if (code != 200) {
    Serial.printf("[poll] http code=%d\n", code);
    http.end();
    consecutivePollFailures++;
    return false;
  }

  String response = http.getString();
  http.end();

  StaticJsonDocument<768> doc;
  DeserializationError err = deserializeJson(doc, response);
  if (err) {
    Serial.printf("[poll] json parse error: %s\n", err.c_str());
    consecutivePollFailures++;
    return false;
  }

  const char* commandId = doc["id"] | "";
  const char* action = doc["action"] | "";
  uint16_t durationSec = doc["payload"]["durationSec"] | DEFAULT_UNLOCK_SEC;
  durationSec = constrain(durationSec, MIN_UNLOCK_SEC, MAX_UNLOCK_SEC);

  if (strlen(commandId) == 0) {
    Serial.println("[poll] missing command id");
    consecutivePollFailures++;
    return false;
  }

  if (strcmp(action, "unlock") != 0) {
    Serial.printf("[poll] unsupported action: %s\n", action);
    sendAck(String(commandId), false, "unsupported_action");
    consecutivePollFailures = 0;
    return false;
  }

  startUnlock(durationSec, String(commandId));
  consecutivePollFailures = 0;
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(PIN_IN2, LOW);

  // Setup the button pin with the internal pullup resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // Attach the interrupt to watch for the voltage dropping to LOW
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  ensureWifiConnected();
  Serial.println("[setup] ready");
}

void loop() {
  finishUnlockIfDue();

  if (relayActive) {
    delay(20);
    return;
  }

  bool started = pollOnceForCommand();
  if (started) {
    delay(20);
    return;
  }

  if (consecutivePollFailures == 0) {
    delay(100);
  } else {
    uint8_t attempt = consecutivePollFailures > 6 ? 6 : consecutivePollFailures;
    delay(backoffDelayMs(attempt, 250, 5000));
  }
}