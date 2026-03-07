#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Minimal required setup:
// 1) Wi-Fi credentials
// 2) API base URL (https://...)
// 3) device token from POST /v1/access-hooks response
// 4) relay pin
const char* WIFI_SSID = "Energie_D";
const char* WIFI_PASSWORD = "Infoel2025!";
// API host exposing /v1/* endpoints.
// Local dev usually: http://localhost:8081 (not reachable from ESP32 unless same LAN + routing).
// Hosted production: https://haloandrei.com
const char* API_BASE_URL = "https://haloandrei.com";
const char* DEVICE_TOKEN = "0TBZenNN8deBAlb3UOsPGHvgbSSGewEI7w2tZaByvAk";
const uint8_t RELAY_PIN = 5;
const uint8_t PIN_IN2 = 18;

// `API_TLS_INSECURE=true` skips TLS certificate validation (quick setup, lower security).
// `API_TLS_INSECURE=false` validates TLS using `API_ROOT_CA` (recommended for production hardening).
// Minimal setup: leave true.
const bool API_TLS_INSECURE = true;
// Root CA certificate for your API cert chain (only used when API_TLS_INSECURE=false).
// For Let's Encrypt endpoints, use ISRG Root X1 PEM.
const char* API_ROOT_CA = R"EOF(
-----BEGIN CERTIFICATE-----
REPLACE_WITH_CA_CERT
-----END CERTIFICATE-----
)EOF";

const int DEFAULT_UNLOCK_SEC = 15;
const int MIN_UNLOCK_SEC = 1;
const int MAX_UNLOCK_SEC = 30;
const int POLL_WAIT_SEC = 20;     // must be <= server max (25)
const int CONNECT_TIMEOUT_MS = 7000;
const int REQUEST_TIMEOUT_MS = 30000;
const uint8_t MAX_ACK_RETRIES = 5;

String activeCommandId;
uint32_t unlockUntilMs = 0;
bool relayActive = false;
uint8_t consecutivePollFailures = 0;

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
        // Configuration or command mismatch; retries will not fix it.
        return false;
      }
    }

    delay(backoffDelayMs(attempt, 300, 4000));
  }

  return false;
}

void startUnlock(uint16_t durationSec, const String& commandId) {
  durationSec = constrain(durationSec, MIN_UNLOCK_SEC, MAX_UNLOCK_SEC);
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(PIN_IN2, LOW);
  relayActive = true;
  unlockUntilMs = millis() + (uint32_t)durationSec * 1000U;
  activeCommandId = commandId;
  Serial.printf("[relay] unlock start for %u sec\n", durationSec);
}

void finishUnlockIfDue() {
  if (!relayActive) return;
  if ((int32_t)(millis() - unlockUntilMs) < 0) return;

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(PIN_IN2, LOW);
  relayActive = false;
  Serial.println("[relay] unlock finished");

  bool acked = sendAck(activeCommandId, true, "");
  if (!acked) {
    Serial.println("[relay] warning: success ack failed");
  }
  activeCommandId = "";
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
