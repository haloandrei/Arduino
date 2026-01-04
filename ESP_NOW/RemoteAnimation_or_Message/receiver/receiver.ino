// ---------- WiFi + LED Matrix HTTP Demo with IDLE IP scroll ----------
// Board: Arduino UNO R4 WiFi

#include <WiFiS3.h>           // WiFi for UNO R4 WiFi
#include <ArduinoGraphics.h>  // must come BEFORE Arduino_LED_Matrix
#include <Arduino_LED_Matrix.h>

ArduinoLEDMatrix matrix;

// ---- WiFi credentials ----
char ssid[] = "Galaxy S25 B5F6";
char pass[] = "1234halomine";

WiFiServer server(80);

// Idle IP scroll config
const unsigned long IDLE_DELAY_MS = 10000;   // go idle after 10s without HTTP
unsigned long lastClientActivity = 0;
bool idleIpShown = false;
char ipText[32] = "";   // text we scroll when idle (the board's IP)

// ---------- LED MATRIX FUNCTIONS ----------

// Heartbeat animation
void playAnimation() {
  Serial.println("[MATRIX] Playing HEARTBEAT animation");

  matrix.loadSequence(LEDMATRIX_ANIMATION_HEARTBEAT_LINE);
  matrix.play(true);   // continuous playback
  idleIpShown = false; // we're in "active" mode now
}

// Generic scrolling text
void scrollText(const char* txt) {
  Serial.print("[MATRIX] Scrolling text: ");
  Serial.println(txt);

  matrix.beginDraw();

  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(50);

  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(txt);
  matrix.endText(SCROLL_LEFT);

  matrix.endDraw();

  idleIpShown = false; // this is an explicit command, not idle
}

// Scroll IP when idle
void scrollIpIdle() {
  if (ipText[0] == '\0') return; // no IP known yet

  Serial.print("[MATRIX] Idle: scrolling IP ");
  Serial.println(ipText);

  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(50);  // pretty slow scroll
  matrix.textFont(Font_5x7);

  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(ipText);
  matrix.endText(SCROLL_LEFT);

  matrix.endDraw();

  idleIpShown = true;
}

// ---------- HELPER: connect to WiFi ----------

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  // Wait until associated
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi associated, waiting for IP...");

  // Wait for a non-zero IP
  IPAddress ip = WiFi.localIP();
  while (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0) {
    delay(200);
    ip = WiFi.localIP();
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");

  Serial.print("IP address: ");
  Serial.println(ip);

  // Store IP as text for idle scroll
  String ipStr = ip.toString();          // e.g. "192.168.1.42"
  ipStr.toCharArray(ipText, sizeof(ipText));

  // WiFiS3: macAddress() expects a buffer
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  for (int i = 0; i < 6; i++) {
    if (i > 0) Serial.print(":");
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
  }
  Serial.println();
}

// ---------- SETUP ----------

void setup() {
  Serial.begin(115200);
  delay(1000);

  matrix.begin();    // init LED matrix

  connectToWiFi();   // connect to AP

  server.begin();    // start HTTP server
  Serial.println("HTTP server started on port 80");
  Serial.println("Use: http://<IP>/anim or /scroll/YourText");

  lastClientActivity = millis();
}

// ---------- LOOP: handle HTTP + idle IP scroll ----------

void loop() {
  // 1) Handle HTTP client if present
  WiFiClient client = server.available();
  if (client) {
    Serial.println();
    Serial.println("[HTTP] Client connected");
    lastClientActivity = millis();
    idleIpShown = false; // reset idle state

    // Read the first line of the HTTP request
    String reqLine = client.readStringUntil('\n');
    Serial.print("[HTTP] Request line: ");
    Serial.println(reqLine);

    // Example lines:
    //   GET /anim HTTP/1.1
    //   GET /scroll/Hello_IoT HTTP/1.1
    if (reqLine.startsWith("GET /anim")) {
      playAnimation();
    } else if (reqLine.startsWith("GET /scroll/")) {
      int start = String("GET /scroll/").length();
      int end = reqLine.indexOf(' ', start);
      if (end == -1) end = reqLine.length();
      String msg = reqLine.substring(start, end);

      // Replace '_' with spaces so you can send "Hello_World"
      msg.replace('_', ' ');

      scrollText(msg.c_str());
    } else {
      Serial.println("[HTTP] Unknown path");
    }

    // Flush remaining HTTP headers
    while (client.available()) {
      client.read();
    }

    // Minimal HTTP response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("OK");
    client.stop();
    Serial.println("[HTTP] Client disconnected");
  }

  // 2) Idle behaviour: after some time with no clients, scroll IP
  unsigned long now = millis();
  if ((now - lastClientActivity > IDLE_DELAY_MS) && !idleIpShown && WiFi.status() == WL_CONNECTED) {
    scrollIpIdle();
    // The text will scroll once; when loop comes again, 
    // idleIpShown prevents restarting it immediately.
    // If you want it to repeat, set idleIpShown=false after some timeout.
  }
}
