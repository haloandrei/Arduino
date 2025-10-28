#include <Arduino.h>
#include <ESP32Servo.h>
#include <math.h>   // for fmax, fmod, fabs

#define SERIAL_BAUD 115200

// --- Pins ---
static const int POS_PIN  = 14;   // positional servo signal (GPIO14)
static const int CONT_PIN = 2;   // continuous servo signal (GPIO13)

// --- Servo freq ---
static const int SERVO_FREQ_HZ = 50;

// --- Positional pulse range (tunable at runtime via 'posrange') ---
int posMinUs = 1000;
int posMaxUs = 2000;

// --- Continuous neutral/range (tunable via 'cneutral'/'crange') ---
int contNeutralUs = 1500; // stop
int contRangeUs   = 400;  // full speed at 1500 ± range
int contPct       = 33;    // -100..100 current command

// --- Follow/orbit oscillator for positional servo ---
bool   followEnabled   = true;    // 'follow on/off'
double freqHz          = 0.5;     // 'freq <Hz>'     (circle/rps)
double ampDeg          = 45.0;    // 'amp <deg>'     (± around offset)
double offsetDeg       = 90.0;    // 'offset <deg>'  (center angle)
double phaseDeg        = 0.0;     // 'phase <deg>'   (lead/lag vs spinner)
double slewDegPerSec   = 360.0;   // 'slew <deg/s>'  (0 = unlimited)
double posAngleNowDeg  = 90.0;    // tracked output angle

// --- Timing state ---
uint64_t lastUpdateUs  = 0;
double   phiRad        = 0.0;     // running oscillator phase

// --- Safety (optional stop for the 360) ---
static const uint32_t CONT_IDLE_TIMEOUT_MS = 0; // set >0 if you want auto-stop
uint32_t lastCmdMs = 0;

Servo servoPos, servoCont;

// ---------------- Serial line editor (CR, LF, CRLF, backspace) --------------
String inBuf;
bool readCommand(String &out) {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (c == '\r' && Serial.peek() == '\n') Serial.read();
      out = inBuf; inBuf = ""; return true;
    }
    if (c == 0x08 || c == 0x7F) { if (inBuf.length()) { inBuf.remove(inBuf.length()-1); Serial.print("\b \b"); } }
    else if (isPrintable(c)) { inBuf += c; Serial.print(c); }
  }
  return false;
}
void prompt() { Serial.print("\r\n> "); }

// ---------------- Helpers ----------------------------------------------------
int clampi(int v, int lo, int hi){ return v<lo?lo:(v>hi?hi:v); }
double clampd(double v, double lo, double hi){ return v<lo?lo:(v>hi?hi:v); }

int angleToUs(double deg) {
  deg = clampd(deg, 0.0, 180.0);
  return map((int)round(deg), 0, 180, posMinUs, posMaxUs);
}

void writePositionalAngle(double deg) {
  deg = clampd(deg, 0.0, 180.0);
  posAngleNowDeg = deg;
  servoPos.writeMicroseconds(angleToUs(deg));
}

void setContPct(int pct) {
  contPct = clampi(pct, -100, 100);
  int us = contNeutralUs + (contPct * contRangeUs) / 100;
  servoCont.writeMicroseconds(us);
}

void attachPos() {
  servoPos.detach();
  servoPos.setPeriodHertz(SERVO_FREQ_HZ);
  bool ok = servoPos.attach(POS_PIN, posMinUs, posMaxUs);
  Serial.printf("\r\n[attach POS] pin=%d range=%d..%d -> %s", POS_PIN, posMinUs, posMaxUs, ok?"OK":"FAIL");
}

void attachCont() {
  servoCont.detach();
  servoCont.setPeriodHertz(SERVO_FREQ_HZ);
  bool ok = servoCont.attach(CONT_PIN, 400, 2600); // wide for raw pulses/speed
  Serial.printf("\r\n[attach CONT] pin=%d range=400..2600 -> %s", CONT_PIN, ok?"OK":"FAIL");
}

void printHelp() {
  Serial.println(F("\r\nCommands:"));
  Serial.println(F("  help                    - show this help"));
  Serial.println(F("  status                  - print current settings"));
  Serial.println(F("  --- continuous (GPIO13) ---"));
  Serial.println(F("  cspeed <-100..100>      - set speed percent"));
  Serial.println(F("  cneutral <us>           - set stop pulse (e.g., 1500..1520)"));
  Serial.println(F("  crange <us>             - set speed range (e.g., 300..600)"));
  Serial.println(F("  cpulse <us>             - raw pulse to continuous servo"));
  Serial.println(F("  --- positional (GPIO14) ---"));
  Serial.println(F("  posrange <min> <max>    - set pulse range and re-attach"));
  Serial.println(F("  posangle <0..180>       - manual angle (disables follow)"));
  Serial.println(F("  poscenter               - go to 90 deg"));
  Serial.println(F("  pspulse <us>            - raw pulse to positional servo"));
  Serial.println(F("  --- follow/orbit generator ---"));
  Serial.println(F("  follow on|off           - enable/disable sine tracking"));
  Serial.println(F("  freq <Hz>               - cycles per second (0.01..5)"));
  Serial.println(F("  amp <deg>               - amplitude around offset"));
  Serial.println(F("  offset <deg>            - center angle"));
  Serial.println(F("  phase <deg>             - phase lead (+) / lag (-)"));
  Serial.println(F("  slew <deg_per_s>        - limit rate (0 = unlimited)"));
  Serial.println(F("  sync                    - reset phase now (line up visually)"));
  Serial.println(F("  ampcalc <Rmm> <Lmm>     - est. amplitude ≈ asin(R/L) in degrees"));
}

void printStatus() {
  Serial.printf("\r\n[STATUS]");
  Serial.printf("\r\n POS pin=%d range=%d..%d us angle=%.1f°", POS_PIN, posMinUs, posMaxUs, posAngleNowDeg);
  Serial.printf("\r\n CONT pin=%d neutral=%d us range=%d us speed=%d%%", CONT_PIN, contNeutralUs, contRangeUs, contPct);
  Serial.printf("\r\n FOLLOW=%s freq=%.3f Hz amp=%.1f° offset=%.1f° phase=%.1f° slew=%.1f°/s",
                followEnabled?"ON":"OFF", freqHz, ampDeg, offsetDeg, phaseDeg, slewDegPerSec);
  Serial.println();
}

// ---------------- Command handling ------------------------------------------
void handleLine(String line) {
  line.trim(); if (!line.length()) return;
  lastCmdMs = millis();

  // tokenize
  line.toLowerCase();
  int sp = line.indexOf(' ');
  String cmd = (sp==-1)? line : line.substring(0, sp);
  String arg = (sp==-1)? ""   : line.substring(sp+1); arg.trim();

  if (cmd=="help"||cmd=="?") { printHelp(); }
  else if (cmd=="status")    { printStatus(); }

  // continuous
  else if (cmd=="cspeed")    { setContPct(arg.toInt()); Serial.printf("\r\ncspeed=%d%%", contPct); }
  else if (cmd=="cneutral")  { contNeutralUs = clampi(arg.toInt(), 1400, 1600); Serial.printf("\r\ncneutral=%d", contNeutralUs); setContPct(contPct); }
  else if (cmd=="crange")    { contRangeUs   = clampi(arg.toInt(), 100, 700);  Serial.printf("\r\ncrange=%d", contRangeUs); setContPct(contPct); }
  else if (cmd=="cpulse")    { int us = clampi(arg.toInt(), 400, 2600); servoCont.writeMicroseconds(us); Serial.printf("\r\ncpulse=%d", us); }

  // positional
  else if (cmd=="posrange")  {
    int s2 = arg.indexOf(' ');
    if (s2==-1) { Serial.println(F("\r\nUsage: posrange <min_us> <max_us>")); }
    else {
      int mn = clampi(arg.substring(0,s2).toInt(), 400, 2400);
      int mx = clampi(arg.substring(s2+1).toInt(), mn+100, 2600);
      posMinUs = mn; posMaxUs = mx; attachPos();
    }
  }
  else if (cmd=="posangle")  { 
    followEnabled = false; 
    writePositionalAngle(clampd((double)arg.toFloat(), 0.0, 180.0));  // FIX: clampd (double)
    Serial.print(F("\r\nfollow=OFF")); 
  }
  else if (cmd=="poscenter") { followEnabled = false; writePositionalAngle(90.0); Serial.print(F("\r\nfollow=OFF")); }
  else if (cmd=="pspulse")   { int us = clampi(arg.toInt(), 400, 2600); servoPos.writeMicroseconds(us); Serial.printf("\r\npspulse=%d", us); followEnabled=false; }

  // follow/orbit
  else if (cmd=="follow")    { followEnabled = (arg=="on"||arg=="1"||arg=="true"); Serial.printf("\r\nfollow=%s", followEnabled?"ON":"OFF"); }
  else if (cmd=="freq")      { freqHz = clampd((double)arg.toFloat(), 0.01, 5.0); Serial.printf("\r\nfreq=%.3f Hz", freqHz); }
  else if (cmd=="amp")       { ampDeg = clampd((double)arg.toFloat(), 0.0, 90.0);  Serial.printf("\r\namp=%.1f deg", ampDeg); }
  else if (cmd=="offset")    { offsetDeg = clampd((double)arg.toFloat(), 0.0, 180.0); Serial.printf("\r\noffset=%.1f deg", offsetDeg); }
  else if (cmd=="phase")     { phaseDeg = fmod((double)arg.toFloat(), 360.0); Serial.printf("\r\nphase=%.1f deg", phaseDeg); }
  else if (cmd=="slew")      { slewDegPerSec = clampd((double)arg.toFloat(), 0.0, 2000.0); Serial.printf("\r\nslew=%.1f deg/s", slewDegPerSec); }
  else if (cmd=="sync")      { phiRad = 0.0; Serial.println(F("\r\nsynced phase = 0")); }
  else if (cmd=="ampcalc")   {
    int s2 = arg.indexOf(' ');
    if (s2==-1) { Serial.println(F("\r\nUsage: ampcalc <Rmm> <Lmm>")); }
    else {
      // FIX: use fmax with doubles to avoid std::max(float,double) ambiguity
      double R = fmax(1.0, (double)arg.substring(0,s2).toFloat());
      double L = fmax(1.0, (double)arg.substring(s2+1).toFloat());
      double ratio = R / L;
      if (ratio > 1.0) ratio = 1.0;
      double deg = (180.0/M_PI)*asin(ratio);
      Serial.printf("\r\namp ≈ %.1f deg   (for radius=%.1f mm, horn=%.1f mm)", deg, R, L);
    }
  }

  else { Serial.println(F("\r\nUnknown. Type 'help'.")); }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  uint32_t t0 = millis(); while (!Serial && (millis()-t0<3000)) { delay(10); }

  // PWM timers (ESP32)
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  attachPos();
  attachCont();

  Serial.println(F("\r\nDual-Servo Orbit Follower"));
  Serial.printf("POS GPIO=%d | CONT GPIO=%d | Baud=%d\r\n", POS_PIN, CONT_PIN, SERIAL_BAUD);
  Serial.println(F("Type 'help' for commands. Start with 'cspeed', then match 'freq', 'amp', 'phase'."));
  writePositionalAngle(offsetDeg);
  setContPct(30);
  lastUpdateUs = micros();
  prompt();
}

void loop() {
  // --- Commands
  String line;
  if (readCommand(line)) { handleLine(line); prompt(); }

  // --- Auto-stop for continuous (optional)
  if (CONT_IDLE_TIMEOUT_MS>0 && (millis()-lastCmdMs > CONT_IDLE_TIMEOUT_MS) && contPct!=0) {
    setContPct(0);
    Serial.println(F("\r\n[safety] idle -> continuous STOP"));
    prompt();
  }

  // --- Follow/orbit update
  uint64_t nowUs = micros();
  double dt = (nowUs - lastUpdateUs) / 1e6;
  if (dt < 0) dt = 0;
  lastUpdateUs = nowUs;

  if (followEnabled) {
    // advance oscillator
    double phaseOffsetRad = phaseDeg * (M_PI/180.0);
    phiRad += (2.0 * M_PI) * freqHz * dt;
    if (phiRad > 1e6) phiRad = fmod(phiRad, 2.0*M_PI);

    double targetDeg = offsetDeg + ampDeg * sin(phiRad + phaseOffsetRad);

    // optional slew limiting
    double stepMax = (slewDegPerSec <= 0.0) ? 1e9 : (slewDegPerSec * dt);
    double delta = targetDeg - posAngleNowDeg;
    if (fabs(delta) > stepMax) delta = (delta > 0) ? stepMax : -stepMax;

    writePositionalAngle(posAngleNowDeg + delta);
  }

  delay(5);
}
