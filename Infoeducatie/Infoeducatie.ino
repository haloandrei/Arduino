#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Ticker.h> 
#include <time.h> // Included for NTP real-time clock

TFT_eSPI tft = TFT_eSPI(); 

// Minimal required setup:
const char* WIFI_SSID = "BradOEL";
const char* WIFI_PASSWORD = "1234halomine2";
const char* API_BASE_URL = "https://haloandrei.com";
const char* DEVICE_TOKEN = "BkkrGl76die6HQ1Vf6qUa-2_CtCJAoWADU8jxxuOk10";

const uint8_t RELAY_PIN = 26;
const uint8_t BUTTON_PIN = 25;

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
volatile bool triggerUnlockUI = false; // Flag to safely draw unlock UI
volatile bool triggerLockUI = false;   // Flag to safely draw lock UI
Ticker localUnlockTimer;

// UI State Management
enum UIState { UI_BOOT, UI_NORMAL, UI_UNLOCKED };
UIState currentUI = UI_BOOT;

// --- UTILITY FUNCTIONS ---

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

// Get standard formatted time
String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 50)) { // 50ms wait
    return "Syncing...";
  }
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  return String(buffer);
}

// --- DISPLAY FUNCTION ---

// Master function to manage TFT changes without flickering
void updateScreen(bool isUnlocked, String lastChecked = "") {
  static String prevLastChecked = "";
  static wl_status_t prevWifi = WL_IDLE_STATUS;
  int cx = tft.width() / 2; // Center X

  if (isUnlocked) {
    if (currentUI != UI_UNLOCKED) {
      tft.fillScreen(TFT_BLACK);
      
      // Draw a massive thick green checkmark
      int cy = tft.height() / 2 - 20;
      for(int i = -5; i <= 5; i++) {
        tft.drawLine(cx - 40, cy + i, cx - 10, cy + 30 + i, TFT_GREEN);
        tft.drawLine(cx - 10, cy + 30 + i, cx + 50, cy - 40 + i, TFT_GREEN);
      }
      
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.drawString("ACCESS GRANTED", cx, cy + 70, 4);
      currentUI = UI_UNLOCKED;
    }
  } else {
    // If returning from unlock state, wipe the screen
    if (currentUI != UI_NORMAL) {
      tft.fillScreen(TFT_BLACK);
      currentUI = UI_NORMAL;
      prevLastChecked = ""; // Force redraw of time
      prevWifi = WL_IDLE_STATUS; // Force redraw of WiFi
    }
    
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("PRIME ACCESS", cx, 30, 4);
    
    // Draw WiFi Status (only update if it changed)
    if (WiFi.status() != prevWifi) {
        prevWifi = WiFi.status();
        tft.fillRect(0, 70, tft.width(), 30, TFT_BLACK); // Clear row
        if (WiFi.status() == WL_CONNECTED) {
           tft.setTextColor(TFT_CYAN, TFT_BLACK);
           tft.drawString("WiFi: " + WiFi.localIP().toString(), cx, 85, 2);
        } else {
           tft.setTextColor(TFT_RED, TFT_BLACK);
           tft.drawString("WiFi: Disconnected", cx, 85, 2);
        }
    }

    // Draw Last Checked Time (only update if it changed)
    if (lastChecked != "" && lastChecked != prevLastChecked) {
      tft.fillRect(0, 120, tft.width(), 30, TFT_BLACK); // Clear row
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.drawString("Last Check: " + lastChecked, cx, 110, 2);
      prevLastChecked = lastChecked;
    }
  }
}

// --- NETWORK CORE ---

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
  
  // Immediately reflect disconnected state
  updateScreen(false); 

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
  
  // Refresh UI now that we are connected
  updateScreen(false, getFormattedTime());
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

// --- HARDWARE INTERRUPT FUNCTIONS ---

void endLocalUnlock() {
  digitalWrite(RELAY_PIN, LOW);
  relayActive = false;
  localUnlockActive = false;
  triggerLockUI = true; // Safely tell loop() to revert screen
}

void IRAM_ATTR buttonISR() {
  static uint32_t lastPress = 0;
  uint32_t now = millis();
  
  if (now - lastPress > 300) { 
    lastPress = now;
    
    if (!relayActive) {
      digitalWrite(RELAY_PIN, HIGH);
      relayActive = true;
      localUnlockActive = true;
      triggerUnlockUI = true; // Safely tell loop() to draw checkmark
      
      localUnlockTimer.once_ms(7000, endLocalUnlock);
    }
  }
}

// --- RELAY CONTROL LOGIC ---

void startUnlock(uint16_t durationSec, const String& commandId) {
  durationSec = constrain(durationSec, MIN_UNLOCK_SEC, MAX_UNLOCK_SEC);
  
  if (localUnlockActive) {
    localUnlockTimer.detach();
    localUnlockActive = false;
  }

  digitalWrite(RELAY_PIN, HIGH);
  relayActive = true;
  unlockUntilMs = millis() + (uint32_t)durationSec * 1000U;
  activeCommandId = commandId;
  
  // Trigger UI Change
  updateScreen(true);
  
  Serial.printf("[relay] unlock start for %u sec\n", durationSec);
}

void finishUnlockIfDue() {
  if (!relayActive) return;
  if (localUnlockActive) return; 
  if ((int32_t)(millis() - unlockUntilMs) < 0) return;

  digitalWrite(RELAY_PIN, LOW);
  relayActive = false;
  
  // Revert UI to normal mode
  updateScreen(false, getFormattedTime());
  
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
  
  // Irrespective of result, update the "Last Checked" timestamp
  if (!relayActive) { 
    updateScreen(false, getFormattedTime());
  }

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
  
  // Initialize TFT First
  tft.init();
  tft.setRotation(1); // Landscape mode usually works best for menus
  tft.fillScreen(TFT_BLACK);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  updateScreen(false, "Booting...");
  
  ensureWifiConnected();
  
  // Initialize NTP Time Sync (EET/EEST Timezone applied)
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
  tzset();

  Serial.println("[setup] ready");
}

void loop() {
  // Check flags set by the ISR and apply UI changes safely
  if (triggerUnlockUI) {
    triggerUnlockUI = false;
    updateScreen(true);
  }
  
  if (triggerLockUI) {
    triggerLockUI = false;
    updateScreen(false, getFormattedTime());
  }

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