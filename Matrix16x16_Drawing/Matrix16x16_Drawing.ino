#include <WiFiS3.h>

#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
ArduinoLEDMatrix ipMatrix;

// NeoPixel matrix
#include <Adafruit_NeoPixel.h>

#define LED_PIN 12
#define NUM_LEDS 256
#define WIDTH 16
#define HEIGHT 16

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ===== WiFi credentials =====
const char WIFI_SSID[] = "BradOEL";
const char WIFI_PASS[] = "1234halomine2";
// ===========================

WiFiServer server(80);

// If your physical matrix wiring is serpentine (common), keep true.
// If it’s straight rows left->right every row, set false.
bool SERPENTINE = true;

// In-memory framebuffer (optional but handy)
uint8_t fbR[NUM_LEDS], fbG[NUM_LEDS], fbB[NUM_LEDS];

static inline uint8_t cap128(int v) {
  if (v < 0) return 0;
  if (v > 128) return 128;
  return (uint8_t)v;
}

int xyToIndex(int x, int y) {
  // Rotate 90° clockwise
  int xr = y;
  int yr = x;

  if (xr < 0 || xr >= WIDTH || yr < 0 || yr >= HEIGHT) return -1;

  if (!SERPENTINE) {
    return yr * WIDTH + xr;
  } else {
    // serpentine rows
    if ((yr & 1) == 0) {
      return yr * WIDTH + xr;
    } else {
      return yr * WIDTH + (WIDTH - 1 - xr);
    }
  }
}


void clearMatrixPixels() {
  for (int i = 0; i < NUM_LEDS; i++) {
    fbR[i] = fbG[i] = fbB[i] = 0;
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

void setPixelXY(int x, int y, int r, int g, int b) {
  int idx = xyToIndex(x, y);
  if (idx < 0) return;

  uint8_t R = cap128(r);
  uint8_t G = cap128(g);
  uint8_t B = cap128(b);

  fbR[idx] = R;
  fbG[idx] = G;
  fbB[idx] = B;
  strip.setPixelColor(idx, strip.Color(R, G, B));
}

void connectToWiFiAndGetIP() {
  Serial.print("Connecting to WiFi SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.disconnect();
  delay(300);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(800);
  }
  Serial.println("\nWiFi associated, waiting for DHCP IP...");

  IPAddress ip = WiFi.localIP();
  unsigned long start = millis();
  const unsigned long ipTimeout = 20000;

  while (ip == IPAddress(0, 0, 0, 0) && (millis() - start) < ipTimeout) {
    delay(200);
    ip = WiFi.localIP();
  }

  Serial.print("WiFi connected! IP address: ");
  Serial.println(ip);
}

void scrollIPOnBuiltinMatrix() {
  IPAddress ip = WiFi.localIP();
  String s = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);

  ipMatrix.beginDraw();
  ipMatrix.stroke(0xFFFFFFFF);
  ipMatrix.textFont(Font_5x7);
  ipMatrix.textScrollSpeed(55);
  ipMatrix.beginText(0, 1, 0xFFFFFF);
  ipMatrix.println(s);
  ipMatrix.endText(SCROLL_LEFT);
  ipMatrix.endDraw();
}

void sendOK(WiFiClient &client, const char *ctype = "text/html") {
  client.print(F("HTTP/1.1 200 OK\r\n"));
  client.print(F("Connection: close\r\n"));
  client.print(F("Cache-Control: no-store\r\n"));
  client.print(F("Content-Type: "));
  client.print(ctype);
  client.print(F("\r\n\r\n"));
}

void send404(WiFiClient &client) {
  client.print(F("HTTP/1.1 404 Not Found\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\n404\n"));
}

String urlDecode(const String &s) {
  // Minimal decode: handles %xx and +
  String out;
  out.reserve(s.length());
  for (int i = 0; i < (int)s.length(); i++) {
    char c = s[i];
    if (c == '+') out += ' ';
    else if (c == '%' && i + 2 < (int)s.length()) {
      char h1 = s[i + 1], h2 = s[i + 2];
      auto hex = [](char h) -> int {
        if (h >= '0' && h <= '9') return h - '0';
        if (h >= 'A' && h <= 'F') return 10 + (h - 'A');
        if (h >= 'a' && h <= 'f') return 10 + (h - 'a');
        return 0;
      };
      out += (char)((hex(h1) << 4) | hex(h2));
      i += 2;
    } else out += c;
  }
  return out;
}

bool getQueryParam(const String &path, const char *key, int &valueOut) {
  int q = path.indexOf('?');
  if (q < 0) return false;
  String qs = path.substring(q + 1);

  String k = String(key) + "=";
  int p = qs.indexOf(k);
  if (p < 0) return false;

  int start = p + k.length();
  int end = qs.indexOf('&', start);
  if (end < 0) end = qs.length();

  String val = qs.substring(start, end);
  val = urlDecode(val);
  valueOut = val.toInt();
  return true;
}

void handleRoot(WiFiClient &client) {
  sendOK(client, "text/html; charset=utf-8");

  // A tiny HTML app: 16x16 canvas, pointer drawing, RGB with cap.
  client.print(F(R"HTML(
<!doctype html><html><head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1" />
<title>UNO R4 16x16 Draw</title>
<style>
  body{font-family:system-ui,Arial;margin:16px}
  .row{display:flex;gap:12px;flex-wrap:wrap;align-items:center}
  canvas{touch-action:none;border:1px solid #aaa;border-radius:8px}
  label{display:flex;gap:8px;align-items:center}
  input[type=range]{width:180px}
  button{padding:10px 14px;border-radius:10px;border:1px solid #aaa;background:#f6f6f6}
  .tiny{opacity:.7;font-size:.9em}
</style>
</head><body>
<h2>16×16 LED Matrix Painter</h2>

<div class="row">
  <canvas id="c" width="320" height="320"></canvas>
  <div>
    <div class="row">
      <label>R <input id="r" type="range" min="0" max="255" value="64"></label>
      <span id="rv">64</span>
    </div>
    <div class="row">
      <label>G <input id="g" type="range" min="0" max="255" value="0"></label>
      <span id="gv">0</span>
    </div>
    <div class="row">
      <label>B <input id="b" type="range" min="0" max="255" value="0"></label>
      <span id="bv">0</span>
    </div>
    <p class="tiny">
      Note: Arduino caps each channel to max 128 to limit current.
    </p>
    <div class="row">
      <button id="clear">Clear</button>
      <button id="reip">Scroll IP</button>
    </div>
  </div>
</div>

<script>
const canvas = document.getElementById('c');
const ctx = canvas.getContext('2d');
const N = 16;
const cell = canvas.width / N;

const r = document.getElementById('r');
const g = document.getElementById('g');
const b = document.getElementById('b');
const rv=document.getElementById('rv'), gv=document.getElementById('gv'), bv=document.getElementById('bv');

function updLabels(){ rv.textContent=r.value; gv.textContent=g.value; bv.textContent=b.value; }
[r,g,b].forEach(el=>el.addEventListener('input', updLabels));
updLabels();

function drawGrid(){
  ctx.clearRect(0,0,canvas.width,canvas.height);
  ctx.fillStyle = '#fff';
  ctx.fillRect(0,0,canvas.width,canvas.height);
  ctx.strokeStyle = 'rgba(0,0,0,0.12)';
  for(let i=0;i<=N;i++){
    ctx.beginPath(); ctx.moveTo(i*cell,0); ctx.lineTo(i*cell,canvas.height); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(0,i*cell); ctx.lineTo(canvas.width,i*cell); ctx.stroke();
  }
}
drawGrid();

let isDown=false;
let lastSend=0;

function paintAt(px, py){
  const x = Math.floor(px / cell);
  const y = Math.floor(py / cell);
  if(x<0||x>=N||y<0||y>=N) return;

  // paint preview locally (so it feels instant)
  ctx.fillStyle = `rgb(${r.value},${g.value},${b.value})`;
  ctx.fillRect(x*cell+1, y*cell+1, cell-2, cell-2);

  // throttle network a bit (avoid spamming)
  const now = performance.now();
  if(now - lastSend < 20) return; // ~50 req/s max
  lastSend = now;

  fetch(`/px?x=${x}&y=${y}&r=${r.value}&g=${g.value}&b=${b.value}`).catch(()=>{});
}

function getPos(ev){
  const rect = canvas.getBoundingClientRect();
  const t = (ev.touches && ev.touches[0]) ? ev.touches[0] : ev;
  return { x: (t.clientX - rect.left) * (canvas.width/rect.width),
           y: (t.clientY - rect.top)  * (canvas.height/rect.height) };
}

canvas.addEventListener('pointerdown', (e)=>{ isDown=true; const p=getPos(e); paintAt(p.x,p.y); });
canvas.addEventListener('pointermove', (e)=>{ if(!isDown) return; const p=getPos(e); paintAt(p.x,p.y); });
window.addEventListener('pointerup', ()=>{ isDown=false; });

document.getElementById('clear').addEventListener('click', ()=>{
  drawGrid();
  fetch('/clear').catch(()=>{});
});

document.getElementById('reip').addEventListener('click', ()=>{
  fetch('/ip').catch(()=>{});
});
</script>
</body></html>
)HTML"));
}

void handlePixel(WiFiClient &client, const String &path) {
  int x = 0, y = 0, rr = 0, gg = 0, bb = 0;
  if (!getQueryParam(path, "x", x) || !getQueryParam(path, "y", y) || !getQueryParam(path, "r", rr) || !getQueryParam(path, "g", gg) || !getQueryParam(path, "b", bb)) {
    sendOK(client, "text/plain");
    client.println("missing params");
    return;
  }

  setPixelXY(x, y, rr, gg, bb);
  strip.show();

  sendOK(client, "text/plain");
  client.println("ok");
}

void handleClear(WiFiClient &client) {
  clearMatrixPixels();
  sendOK(client, "text/plain");
  client.println("cleared");
}

void handleIP(WiFiClient &client) {
  // trigger scrolling again on demand
  scrollIPOnBuiltinMatrix();
  sendOK(client, "text/plain");
  client.println("ip scrolled");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  // Built-in matrix for IP
  ipMatrix.begin();

  // NeoPixel
  strip.begin();
  strip.setBrightness(128);  // global cap (0..255). Keeps peak current down.
  clearMatrixPixels();

  connectToWiFiAndGetIP();
  scrollIPOnBuiltinMatrix();

  server.begin();

  Serial.println("HTTP server started.");
  Serial.print("Open: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  // Read request line
  String requestLine = client.readStringUntil('\r');
  client.read();  // '\n'
  // Example: GET /px?x=1&y=2&r=10&g=0&b=0 HTTP/1.1

  int sp1 = requestLine.indexOf(' ');
  int sp2 = requestLine.indexOf(' ', sp1 + 1);
  String path = "/";
  if (sp1 >= 0 && sp2 >= 0) path = requestLine.substring(sp1 + 1, sp2);

  // Drain headers
  while (client.available()) client.read();

  if (path == "/" || path.startsWith("/index")) {
    handleRoot(client);
  } else if (path.startsWith("/px")) {
    handlePixel(client, path);
  } else if (path.startsWith("/clear")) {
    handleClear(client);
  } else if (path.startsWith("/ip")) {
    handleIP(client);
  } else {
    send404(client);
  }

  client.stop();
}
