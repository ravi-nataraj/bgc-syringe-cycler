/*
 * ============================================================
 *  SYRINGE CYCLER  -  configurable over WiFi
 *  Seeed XIAO ESP32-C3
 * ============================================================
 *
 *  HOW TO USE
 *    1. Power the board.
 *    2. On a phone or laptop, join the WiFi network:
 *          SSID     : walrus
 *          password : walrus123
 *    3. Open a browser at   http://192.168.4.1
 *    4. Set extend time, retract time and cycle count. Save.
 *    5. Press Start on the page, or the button on the board.
 *
 *  Settings are held in RAM only. Pulling power or pressing reset
 *  returns the fixture to its defaults of 60 s / 10 s / 20 cycles,
 *  so the next person always starts from a known configuration.
 *
 *  Settings lock while a run is in progress. Long-press the board
 *  button, or press Stop on the page, to unlock.
 *
 *  SERIAL COMMANDS (115200)
 *    E60   extend seconds       S   start
 *    R10   retract seconds      X   stop / reset
 *    C20   cycle count          D   restore defaults
 *
 *  ARDUINO IDE
 *    Board           : XIAO_ESP32C3
 *    USB CDC On Boot : Enabled
 * ============================================================
 */

#include <WiFi.h>
#include <WebServer.h>

// ---------------- pins ----------------
const uint8_t PIN_VALVE  = D0;
const uint8_t PIN_BUTTON = D1;
const uint8_t PIN_LED    = D2;

// ---------------- wifi ----------------
const char *AP_SSID = "walrus";        
const char *AP_PASS = "walrus123";      // WPA2 needs 8+ chars, or "" for open

WebServer server(80);

// ---------------- settings ----------------
// These are the power-on defaults. Nothing is written to flash, so
// cutting power or pressing reset restores these values.
const uint16_t DEF_EXTEND  = 60;    // seconds extended
const uint16_t DEF_RETRACT = 10;    // seconds retracted
const uint16_t DEF_CYCLES  = 20;    // cycles per run

uint16_t extendSec   = DEF_EXTEND;
uint16_t retractSec  = DEF_RETRACT;
uint16_t totalCycles = DEF_CYCLES;

const uint16_t MIN_SEC = 1, MAX_SEC = 600;
const uint16_t MIN_CYC = 1, MAX_CYC = 999;

// ---------------- timing ----------------
const unsigned long DEBOUNCE_MS   = 30;
const unsigned long LONG_PRESS_MS = 1000;
const unsigned long BLINK_MS      = 500;

// ---------------- state ----------------
enum State { IDLE, EXTEND_HOLD, RETRACT_HOLD };
State state = IDLE;

unsigned long stateEnteredAt = 0;
unsigned long runStartedAt   = 0;
uint16_t      cycleCount     = 0;

bool          rawPrev      = HIGH;
bool          stableLevel  = HIGH;
unsigned long lastBounceAt = 0;
unsigned long pressStartAt = 0;
bool          longFired    = false;
bool          shortPress   = false;
bool          longPress    = false;

char    serialBuf[16];
uint8_t serialLen = 0;

bool isRunning() { return state != IDLE; }

// ============================================================
//  core machine
// ============================================================
void setValve(bool on) { digitalWrite(PIN_VALVE, on ? HIGH : LOW); }

void enterState(State s) {
  state = s;
  stateEnteredAt = millis();
}

unsigned long extendMs()  { return (unsigned long)extendSec  * 1000UL; }
unsigned long retractMs() { return (unsigned long)retractSec * 1000UL; }

void startRun() {
  cycleCount   = 0;
  runStartedAt = millis();
  Serial.println(F(">>> RUN STARTED"));
  setValve(true);
  enterState(EXTEND_HOLD);
}

void stopRun(const char *why) {
  setValve(false);
  Serial.print(F("!!! STOP: "));
  Serial.println(why);
  cycleCount = 0;
  enterState(IDLE);
}

uint16_t clampU(long v, uint16_t lo, uint16_t hi) {
  if (v < (long)lo) return lo;
  if (v > (long)hi) return hi;
  return (uint16_t)v;
}

unsigned long remainingSec() {
  if (!isRunning()) return 0;
  unsigned long inState  = (millis() - stateEnteredAt) / 1000UL;
  unsigned long thisLeft = (state == EXTEND_HOLD ? extendSec : retractSec);
  thisLeft = (inState >= thisLeft) ? 0 : thisLeft - inState;
  unsigned long full = (unsigned long)(totalCycles - cycleCount - 1) *
                       (extendSec + retractSec);
  if (state == EXTEND_HOLD) full += retractSec;
  return thisLeft + full;
}

const char *stateName() {
  switch (state) {
    case EXTEND_HOLD:  return "EXTENDED";
    case RETRACT_HOLD: return "RETRACTED";
    default:           return "IDLE";
  }
}

// ============================================================
//  web page
// ============================================================
const char PAGE_HTML[] PROGMEM = R"HTML(<!DOCTYPE html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Syringe Cycler</title><style>
body{font-family:system-ui,-apple-system,Segoe UI,Helvetica,Arial,sans-serif;
margin:0 auto;padding:20px;background:#f6f7f9;color:#1d2939;max-width:460px}
h1{font-size:20px;margin:8px 0 4px}
.sub{color:#667085;font-size:13px;margin-bottom:20px}
.card{background:#fff;border:1px solid #e4e7ec;border-radius:12px;padding:18px;margin-bottom:16px}
label{display:block;font-size:13px;font-weight:600;margin:14px 0 6px}
input{width:100%;padding:12px;font-size:17px;border:1px solid #d0d5dd;
border-radius:8px;box-sizing:border-box}
input:disabled{background:#f2f4f7;color:#98a2b3}
button{width:100%;padding:14px;font-size:16px;font-weight:600;border:0;
border-radius:8px;margin-top:14px;cursor:pointer}
.save{background:#344054;color:#fff}
.start{background:#067647;color:#fff}
.stop{background:#b42318;color:#fff}
button:disabled{background:#d0d5dd;color:#98a2b3}
#st{font-size:15px;line-height:1.7}
.big{font-size:26px;font-weight:700}
.dot{display:inline-block;width:10px;height:10px;border-radius:50%;
margin-right:8px;vertical-align:middle}
.hint{font-size:12px;color:#667085;margin-top:10px}
</style></head><body>
<h1>Syringe Cycler</h1>
<div class="sub">fixture control</div>

<div class="card"><div id="st">connecting...</div></div>

<div class="card">
<label>Extend time (seconds)</label>
<input id="e" type="number" min="1" max="600">
<label>Retract time (seconds)</label>
<input id="r" type="number" min="1" max="600">
<label>Number of cycles</label>
<input id="c" type="number" min="1" max="999">
<button class="save" id="bs" onclick="save()">Save settings</button>
<button class="save" id="bd" onclick="go('/defaults')">Restore defaults</button>
<div class="hint">Settings lock while a run is in progress.<br>
Power-off resets to 60 s / 10 s / 20 cycles.</div>
</div>

<div class="card">
<button class="start" id="bg" onclick="go('/start')">Start run</button>
<button class="stop" onclick="go('/stop')">Stop and reset</button>
</div>

<script>
let editing=false;
['e','r','c'].forEach(i=>{
  let el=document.getElementById(i);
  el.addEventListener('focus',()=>editing=true);
  el.addEventListener('blur',()=>editing=false);
});
function fmt(s){let m=Math.floor(s/60),x=s%60;return m+"m "+String(x).padStart(2,'0')+"s";}
function poll(){
  fetch('/status').then(r=>r.json()).then(d=>{
    let col=d.running?(d.state=="EXTENDED"?"#067647":"#b45309"):"#98a2b3";
    let h='<span class="dot" style="background:'+col+'"></span><b>'+d.state+'</b>';
    if(d.running){
      h+='<div class="big">cycle '+d.cycle+' / '+d.total+'</div>';
      h+='<div>'+fmt(d.remain)+' remaining</div>';
    }else{
      h+='<div class="big">Ready</div>';
      h+='<div>'+d.total+' cycles &middot; '+d.ext+'s / '+d.ret+'s</div>';
    }
    document.getElementById('st').innerHTML=h;
    ['e','r','c'].forEach(i=>document.getElementById(i).disabled=d.running);
    document.getElementById('bs').disabled=d.running;
    document.getElementById('bd').disabled=d.running;
    document.getElementById('bg').disabled=d.running;
    if(!editing){
      document.getElementById('e').value=d.ext;
      document.getElementById('r').value=d.ret;
      document.getElementById('c').value=d.total;
    }
  }).catch(()=>{document.getElementById('st').innerHTML='connection lost';});
}
function save(){
  fetch('/set?e='+document.getElementById('e').value
       +'&r='+document.getElementById('r').value
       +'&c='+document.getElementById('c').value).then(poll);
}
function go(u){fetch(u).then(poll);}
setInterval(poll,1000); poll();
</script></body></html>)HTML";

// ============================================================
//  handlers
// ============================================================
void handleRoot() { server.send_P(200, "text/html", PAGE_HTML); }

void handleStatus() {
  char buf[220];
  snprintf(buf, sizeof(buf),
    "{\"state\":\"%s\",\"running\":%s,\"cycle\":%u,\"total\":%u,"
    "\"remain\":%lu,\"ext\":%u,\"ret\":%u}",
    stateName(), isRunning() ? "true" : "false",
    (unsigned)(cycleCount + (isRunning() ? 1 : 0)),
    totalCycles, remainingSec(), extendSec, retractSec);
  server.send(200, "application/json", buf);
}

void handleSet() {
  if (isRunning()) { server.send(409, "text/plain", "run in progress"); return; }
  if (server.hasArg("e")) extendSec   = clampU(server.arg("e").toInt(), MIN_SEC, MAX_SEC);
  if (server.hasArg("r")) retractSec  = clampU(server.arg("r").toInt(), MIN_SEC, MAX_SEC);
  if (server.hasArg("c")) totalCycles = clampU(server.arg("c").toInt(), MIN_CYC, MAX_CYC);
 
  Serial.printf("settings: %us / %us x %u\n", extendSec, retractSec, totalCycles);
  server.send(200, "text/plain", "ok");
}

void handleDefaults() {
  if (isRunning()) { server.send(409, "text/plain", "run in progress"); return; }
  extendSec   = DEF_EXTEND;
  retractSec  = DEF_RETRACT;
  totalCycles = DEF_CYCLES;
  Serial.println(F("settings restored to defaults"));
  server.send(200, "text/plain", "ok");
}

void handleStart() {
  if (!isRunning()) startRun();
  server.send(200, "text/plain", "ok");
}

void handleStop() {
  stopRun("web");
  server.send(200, "text/plain", "ok");
}

// ============================================================
//  button + led
// ============================================================
void pollButton(unsigned long now) {
  shortPress = false;
  longPress  = false;

  bool raw = digitalRead(PIN_BUTTON);
  if (raw != rawPrev) { rawPrev = raw; lastBounceAt = now; }

  if (now - lastBounceAt >= DEBOUNCE_MS && raw != stableLevel) {
    stableLevel = raw;
    if (stableLevel == LOW) { pressStartAt = now; longFired = false; }
    else if (!longFired)    { shortPress = true; }
  }

  if (stableLevel == LOW && !longFired &&
      now - pressStartAt >= LONG_PRESS_MS) {
    longFired = true;
    longPress = true;
  }
}

void updateLed(unsigned long now) {
  switch (state) {
    case IDLE:         digitalWrite(PIN_LED, LOW);  break;
    case EXTEND_HOLD:  digitalWrite(PIN_LED, HIGH); break;
    case RETRACT_HOLD: digitalWrite(PIN_LED, (now / BLINK_MS) % 2); break;
  }
}

// ============================================================
//  serial
// ============================================================
void showSettings() {
  Serial.printf("settings: extend %us  retract %us  cycles %u  [%s]\n",
                extendSec, retractSec, totalCycles, stateName());
}

void handleSerialLine(char *line) {
  char c = toupper(line[0]);
  long v = atol(line + 1);
  switch (c) {
    case 'E': if (!isRunning()) { extendSec   = clampU(v, MIN_SEC, MAX_SEC); } break;
    case 'R': if (!isRunning()) { retractSec  = clampU(v, MIN_SEC, MAX_SEC); } break;
    case 'C': if (!isRunning()) { totalCycles = clampU(v, MIN_CYC, MAX_CYC); } break;
    case 'S': if (!isRunning()) startRun(); break;
    case 'X': stopRun("serial"); break;
    case 'D': if (!isRunning()) { extendSec = DEF_EXTEND;
                                  retractSec = DEF_RETRACT;
                                  totalCycles = DEF_CYCLES; } break;
    default:  break;
  }
  showSettings();
}

void pollSerial() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (serialLen) { serialBuf[serialLen] = 0; handleSerialLine(serialBuf); serialLen = 0; }
    } else if (serialLen < sizeof(serialBuf) - 1) {
      serialBuf[serialLen++] = ch;
    }
  }
}

// ============================================================
void setup() {
  pinMode(PIN_VALVE, OUTPUT);
  digitalWrite(PIN_VALVE, LOW);          // safe state before anything else
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  Serial.begin(115200);
  delay(400);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  server.on("/",       handleRoot);
  server.on("/status", handleStatus);
  server.on("/set",    handleSet);
  server.on("/defaults", handleDefaults);
  server.on("/start",  handleStart);
  server.on("/stop",   handleStop);
  server.begin();

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("  SYRINGE CYCLER"));
  Serial.print  (F("  join wifi : ")); Serial.println(AP_SSID);
  Serial.print  (F("  password  : ")); Serial.println(AP_PASS);
  Serial.print  (F("  browse to : http://"));
  Serial.println(WiFi.softAPIP());
  showSettings();
  Serial.println(F("========================================"));

  enterState(IDLE);
}

void loop() {
  unsigned long now = millis();

  server.handleClient();
  pollSerial();
  pollButton(now);

  if (longPress) {
    stopRun("button held");
    updateLed(now);
    return;
  }

  switch (state) {

    case IDLE:
      if (shortPress) startRun();
      break;

    case EXTEND_HOLD:
      if (now - stateEnteredAt >= extendMs()) {
        setValve(false);
        Serial.printf("cycle %u/%u : retract\n", cycleCount + 1, totalCycles);
        enterState(RETRACT_HOLD);
      }
      break;

    case RETRACT_HOLD:
      if (now - stateEnteredAt >= retractMs()) {
        cycleCount++;
        if (cycleCount >= totalCycles) {
          setValve(false);
          Serial.printf("<<< COMPLETE - %u cycles in %lu s\n",
                        totalCycles, (now - runStartedAt) / 1000UL);
          cycleCount = 0;
          enterState(IDLE);
        } else {
          setValve(true);
          Serial.printf("cycle %u/%u : extend\n", cycleCount + 1, totalCycles);
          enterState(EXTEND_HOLD);
        }
      }
      break;
  }

  updateLed(now);
}
