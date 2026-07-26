#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <ESP32Servo.h>

// --- DISPLAY SETUP ---
#define TFT_CS   22
#define TFT_DC   21
#define TFT_RST  4
#define TFT_SCLK 18
#define TFT_MOSI 23

#define CYBER_CYAN   0x07FF
#define DARK_CYAN    0x03EF
#define ALERT_RED    0xF800
#define MATRIX_GREEN 0x07E0

Adafruit_GC9A01A tft = Adafruit_GC9A01A(TFT_CS, TFT_DC, TFT_RST);
int cx, cy;
float scanAngle = 0;

// --- HARDWARE ---
Servo lockServo;

const uint8_t RELAY_PIN = 26;
const uint8_t BUTTON_PIN = 25;
const uint8_t SERVO_PIN = 32;

// NEW PINS
const uint8_t HALL_SENSOR_PIN = 33;
const uint8_t BUZZER_PIN = 27;

// Servo positions
const int SERVO_UNLOCK_ANGLE = 90;  // ACCESS GRANTED
const int SERVO_LOCK_ANGLE = 9;     // LOCKED

// Hall sensor logic
// Your module: LOW = magnet detected, HIGH = magnet lost
const int HALL_MAGNET_DETECTED = LOW;

// Alarm timing
const uint32_t HALL_LOST_CONFIRM_MS = 200;
const uint32_t ALARM_BEEP_ON_MS = 120;
const uint32_t ALARM_BEEP_OFF_MS = 120;

// --- WIFI / API ---
const char* WIFI_SSID = "Baldean";
const char* WIFI_PASSWORD = "12345678";
const char* API_BASE_URL = "https://haloandrei.com";
const char* DEVICE_TOKEN = "BkkrGl76die6HQ1Vf6qUa-2_CtCJAoWADU8jxxuOk10";

const bool API_TLS_INSECURE = true;

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

volatile bool relayActive = false;
volatile bool buttonPressed = false;

// Alarm state
bool alarmActive = false;
bool buzzerState = false;
uint32_t hallLostSinceMs = 0;
uint32_t buzzerLastToggleMs = 0;

// --- UI STATE ---
enum SystemState { BOOTING, LOCKED_IDLE, DECRYPTING, ACCESS_GRANTED };
volatile SystemState sysState = BOOTING;

TaskHandle_t uiTaskHandle;

// --- URLS ---
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

// --- WIFI ---
void ensureWifiConnected() {
  if (WiFi.status() == WL_CONNECTED) return;

  if (sysState == BOOTING) {
    tft.fillScreen(GC9A01A_BLACK);
    tft.setTextColor(CYBER_CYAN);
    tft.setTextSize(2);
    tft.setCursor(cx - 70, cy - 10);
    tft.print("CONNECTING");
    tft.setCursor(cx - 30, cy + 15);
    tft.print("WIFI");
  }

  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t start = millis();
  int dotCount = 0;

  while (WiFi.status() != WL_CONNECTED) {
    if (sysState == BOOTING) {
      if (dotCount % 2 == 0) {
        tft.fillCircle(cx, cy + 50, 5, CYBER_CYAN);
      } else {
        tft.fillCircle(cx, cy + 50, 5, GC9A01A_BLACK);
      }
      dotCount++;
    }

    delay(300);

    if (millis() - start > 20000) {
      Serial.println("[wifi] reconnect timeout...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      start = millis();
    }
  }

  Serial.print("[wifi] connected, ip=");
  Serial.println(WiFi.localIP());

  if (sysState == BOOTING) {
    tft.fillCircle(cx, cy + 50, 5, MATRIX_GREEN);
    delay(500);
  }
}

// --- DISPLAY ANIMATION ---
void drawIdleScanner() {
  int oldX = cx + 115 * cos(scanAngle);
  int oldY = cy + 115 * sin(scanAngle);
  tft.drawLine(cx, cy, oldX, oldY, GC9A01A_BLACK);

  scanAngle += 0.15;
  if (scanAngle > 6.28) scanAngle = 0;

  int newX = cx + 115 * cos(scanAngle);
  int newY = cy + 115 * sin(scanAngle);

  tft.drawCircle(cx, cy, 118, CYBER_CYAN);
  tft.drawCircle(cx, cy, 100, DARK_CYAN);
  tft.drawCircle(cx, cy, 70, DARK_CYAN);

  tft.setTextColor(CYBER_CYAN, GC9A01A_BLACK);
  tft.setTextSize(2);
  tft.setCursor(cx - 40, cy - 8);
  tft.print("LOCKED");

  tft.drawLine(cx, cy, newX, newY, CYBER_CYAN);
  delay(30);
}

void runDecryptionSequence() {
  tft.fillScreen(GC9A01A_BLACK);

  tft.drawCircle(cx, cy, 118, ALERT_RED);
  tft.drawRect(cx - 10, 0, 20, 10, ALERT_RED);
  tft.drawRect(cx - 10, 230, 20, 10, ALERT_RED);
  tft.drawRect(0, cy - 10, 10, 20, ALERT_RED);
  tft.drawRect(230, cy - 10, 10, 20, ALERT_RED);

  tft.setTextColor(ALERT_RED, GC9A01A_BLACK);
  tft.setTextSize(2);
  tft.setCursor(cx - 60, cy - 40);
  tft.print("DECRYPTING");

  tft.setTextSize(3);

  for (int i = 0; i < 35; i++) {
    tft.setCursor(cx - 35, cy - 10);

    char code[5];
    sprintf(code, "%04X", random(0x1000, 0xFFFF));

    tft.setTextColor(random(2) == 0 ? GC9A01A_WHITE : ALERT_RED, GC9A01A_BLACK);
    tft.print(code);

    int barWidth = map(i, 0, 34, 0, 120);
    tft.fillRect(cx - 60, cy + 30, barWidth, 10, ALERT_RED);

    delay(40);
  }

  tft.fillScreen(MATRIX_GREEN);
  delay(100);
  tft.fillScreen(GC9A01A_BLACK);

  tft.drawCircle(cx, cy, 115, MATRIX_GREEN);
  for (int i = 0; i < 5; i++) {
    tft.drawCircle(cx, cy, 115 - i, MATRIX_GREEN);
  }

  tft.setTextColor(MATRIX_GREEN);
  tft.setTextSize(3);
  tft.setCursor(cx - 55, cy - 30);
  tft.print("ACCESS");
  tft.setCursor(cx - 65, cy + 10);
  tft.print("GRANTED");

  tft.setTextSize(1);
  tft.setTextColor(CYBER_CYAN);
  tft.setCursor(cx - 40, cy + 50);
  tft.print("WELCOME ANDREI");
}

void uiTask(void* parameter) {
  SystemState lastRenderedState = BOOTING;

  for (;;) {
    if (lastRenderedState != sysState) {
      tft.fillScreen(GC9A01A_BLACK);
      lastRenderedState = sysState;
    }

    if (sysState == LOCKED_IDLE) {
      drawIdleScanner();
    } else if (sysState == DECRYPTING) {
      runDecryptionSequence();
      sysState = ACCESS_GRANTED;
      lastRenderedState = ACCESS_GRANTED;
    } else {
      delay(50);
    }
  }
}

// --- TLS ---
void configureTlsClient(WiFiClientSecure& client) {
  client.setTimeout(REQUEST_TIMEOUT_MS);

  if (API_TLS_INSECURE) {
    client.setInsecure();
  }
}

// --- ACK ---
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

    if (http.begin(client, buildAckUrl())) {
      http.addHeader("Content-Type", "application/json");

      int code = http.POST(body);
      http.end();

      if (code == 204 || code == 200) return true;
      if (code == 401 || code == 404) return false;
    }

    delay(backoffDelayMs(attempt, 300, 4000));
  }

  return false;
}

// --- HALL SENSOR / ALARM ---
bool isMagnetDetected() {
  return digitalRead(HALL_SENSOR_PIN) == HALL_MAGNET_DETECTED;
}

bool isDoorLocked() {
  return relayActive == false;
}

void stopAlarm() {
  alarmActive = false;
  buzzerState = false;
  digitalWrite(BUZZER_PIN, LOW);
}

void updateDoorAlarm() {
  uint32_t now = millis();

  bool locked = isDoorLocked();
  bool magnetDetected = isMagnetDetected();

  // Alarm only matters when locked.
  // If unlocked, or magnet is present, buzzer stays off.
  if (!locked || magnetDetected) {
    hallLostSinceMs = 0;
    stopAlarm();
    return;
  }

  // Locked and magnet is lost.
  // Wait a short time to avoid false triggers.
  if (hallLostSinceMs == 0) {
    hallLostSinceMs = now;
    return;
  }

  if (now - hallLostSinceMs < HALL_LOST_CONFIRM_MS) {
    return;
  }

  // Start alarm
  if (!alarmActive) {
    alarmActive = true;
    buzzerState = true;
    buzzerLastToggleMs = now;
    digitalWrite(BUZZER_PIN, HIGH);
    return;
  }

  // Active buzzer alarm tone:
  // active buzzer cannot change frequency, so we pulse it ON/OFF.
  uint32_t interval = buzzerState ? ALARM_BEEP_ON_MS : ALARM_BEEP_OFF_MS;

  if (now - buzzerLastToggleMs >= interval) {
    buzzerLastToggleMs = now;
    buzzerState = !buzzerState;
    digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
  }
}

// --- LOCK / UNLOCK ---
void lockDoor() {
  digitalWrite(RELAY_PIN, HIGH);
  lockServo.write(SERVO_LOCK_ANGLE);

  relayActive = false;
  sysState = LOCKED_IDLE;
}

void unlockDoor(uint16_t durationSec) {
  durationSec = constrain(durationSec, MIN_UNLOCK_SEC, MAX_UNLOCK_SEC);

  stopAlarm();

  digitalWrite(RELAY_PIN, LOW);
  lockServo.write(SERVO_UNLOCK_ANGLE);

  relayActive = true;
  unlockUntilMs = millis() + (uint32_t)durationSec * 1000U;

  sysState = DECRYPTING;
}

// SAFE ISR: only sets a flag
void IRAM_ATTR buttonISR() {
  static uint32_t lastPress = 0;
  uint32_t now = millis();

  if (now - lastPress > 300) {
    lastPress = now;
    buttonPressed = true;
  }
}

void handleButtonPress() {
  if (!buttonPressed) return;

  buttonPressed = false;

  if (!relayActive) {
    activeCommandId = "";
    unlockDoor(7);
  }
}

void startUnlock(uint16_t durationSec, const String& commandId) {
  activeCommandId = commandId;
  unlockDoor(durationSec);
}

void finishUnlockIfDue() {
  if (!relayActive) return;
  if ((int32_t)(millis() - unlockUntilMs) < 0) return;

  lockDoor();

  if (activeCommandId.length() > 0) {
    sendAck(activeCommandId, true, "");
    activeCommandId = "";
  }
}

// --- API POLLING ---
bool pollOnceForCommand() {
  ensureWifiConnected();

  WiFiClientSecure client;
  configureTlsClient(client);

  HTTPClient http;
  http.setConnectTimeout(CONNECT_TIMEOUT_MS);
  http.setTimeout(REQUEST_TIMEOUT_MS);

  if (!http.begin(client, buildPollUrl())) {
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
    http.end();
    consecutivePollFailures++;
    return false;
  }

  String response = http.getString();
  http.end();

  StaticJsonDocument<768> doc;
  DeserializationError err = deserializeJson(doc, response);

  if (err) {
    consecutivePollFailures++;
    return false;
  }

  const char* commandId = doc["id"] | "";
  const char* action = doc["action"] | "";
  uint16_t durationSec = doc["payload"]["durationSec"] | DEFAULT_UNLOCK_SEC;

  if (strlen(commandId) == 0) {
    consecutivePollFailures++;
    return false;
  }

  if (strcmp(action, "unlock") != 0) {
    sendAck(String(commandId), false, "unsupported_action");
    consecutivePollFailures = 0;
    return false;
  }

  startUnlock(durationSec, String(commandId));

  consecutivePollFailures = 0;
  return true;
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(HALL_SENSOR_PIN, INPUT_PULLDOWN);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  ESP32PWM::allocateTimer(0);
  lockServo.setPeriodHertz(50);
  lockServo.attach(SERVO_PIN, 500, 2400);
  lockServo.write(SERVO_LOCK_ANGLE);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(GC9A01A_BLACK);

  cx = tft.width() / 2;
  cy = tft.height() / 2;

  tft.setTextColor(CYBER_CYAN);
  tft.setTextSize(2);
  tft.setCursor(cx - 85, cy - 10);
  tft.print("HALO-OS v9.0");
  delay(1500);

  ensureWifiConnected();

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
  tzset();

  xTaskCreatePinnedToCore(
    uiTask,
    "DisplayTask",
    4096,
    NULL,
    1,
    &uiTaskHandle,
    1
  );

  sysState = LOCKED_IDLE;

  Serial.println("[setup] ready");
}

// --- LOOP ---
void loop() {
  handleButtonPress();
  finishUnlockIfDue();
  updateDoorAlarm();

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