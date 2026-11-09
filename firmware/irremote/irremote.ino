// IR Light Remote - custom firmware for "ESP32_IR_TR_WIFI_V1.1" board
// Features: IR pin auto-detect, receive/decode log, NEC code scanner,
// saved buttons, web UI for phone, OTA firmware update (/update).
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Preferences.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <vector>

#define FW_VERSION "1.0"

WebServer server(80);
Preferences prefs;

int rxPin = -1, txPin = -1;
IRrecv *irrecv = nullptr;
IRsend *irsend = nullptr;
decode_results results;

// Candidate GPIOs (excludes 1/3 = serial header, 6-11 = flash)
const int CAND[] = {0, 2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23,
                    25, 26, 27, 32, 33, 34, 35, 36, 39};
const int NC = sizeof(CAND) / sizeof(CAND[0]);
volatile uint32_t edgeCount[NC];
uint32_t baseline[NC];
void IRAM_ATTR edgeISR(void *arg) { edgeCount[(int)(intptr_t)arg]++; }

// ---------- pin detection ----------
int detState = 0;  // 0 idle, 1 baseline, 2 press, 3 done
uint32_t detT0 = 0;
String detResult = "";

void attachAll() {
  for (int i = 0; i < NC; i++) {
    pinMode(CAND[i], CAND[i] >= 34 ? INPUT : INPUT_PULLUP);
    edgeCount[i] = 0;
    attachInterruptArg(CAND[i], edgeISR, (void *)(intptr_t)i, CHANGE);
  }
}
void detachAll() {
  for (int i = 0; i < NC; i++) detachInterrupt(CAND[i]);
}

void startRxDetect() {
  attachAll();
  detState = 1;
  detT0 = millis();
  detResult = "";
}

void loopRxDetect() {
  if (detState == 1 && millis() - detT0 > 3000) {
    for (int i = 0; i < NC; i++) { baseline[i] = edgeCount[i]; edgeCount[i] = 0; }
    detState = 2;
    detT0 = millis();
  } else if (detState == 2 && millis() - detT0 > 6000) {
    detachAll();
    int best = -1;
    long bestScore = 0;
    String table = "";
    for (int i = 0; i < NC; i++) {
      long score = (long)edgeCount[i] - (long)baseline[i] * 2;  // 6s vs 3s
      if (edgeCount[i] > 0 || baseline[i] > 0)
        table += "GPIO" + String(CAND[i]) + ": quiet=" + String(baseline[i]) +
                 " pressing=" + String(edgeCount[i]) + "; ";
      if (score > bestScore) { bestScore = score; best = CAND[i]; }
    }
    if (best >= 0 && bestScore >= 30) {
      rxPin = best;
      detResult = "Receiver found on GPIO" + String(best) + ". " + table;
    } else {
      detResult = "No receiver activity found. Press remote buttons during the 6-second window, close to the board. " + table;
    }
    detState = 3;
  }
}

String txDetect() {
  if (rxPin < 0) return "Detect the receiver first.";
  int ri = -1;
  for (int i = 0; i < NC; i++) if (CAND[i] == rxPin) ri = i;
  if (ri < 0) return "Receiver pin not in list.";
  pinMode(rxPin, rxPin >= 34 ? INPUT : INPUT_PULLUP);
  attachInterruptArg(rxPin, edgeISR, (void *)(intptr_t)ri, CHANGE);
  String out = "";
  int best = -1;
  uint32_t bestN = 0;
  for (int i = 0; i < NC; i++) {
    int p = CAND[i];
    if (p == rxPin || p >= 34) continue;
    delay(30);
    edgeCount[ri] = 0;
    IRsend s(p);
    s.begin();
    s.sendNEC(s.encodeNEC(0x00, 0x00));
    delay(30);
    uint32_t n = edgeCount[ri];
    pinMode(p, INPUT);
    out += "GPIO" + String(p) + "=" + String(n) + " ";
    if (n > bestN) { bestN = n; best = p; }
  }
  detachInterrupt(rxPin);
  if (best >= 0 && bestN >= 20) {
    txPin = best;
    return "Transmitter found on GPIO" + String(best) + " (its own receiver heard it). " + out;
  }
  return "Auto-detect could not hear the LED. Use the manual blink test with your phone's front camera. " + out;
}

void blinkPin(int p) {
  IRsend s(p);
  s.begin();
  for (int k = 0; k < 15; k++) { s.sendNEC(s.encodeNEC(0x00, 0x00)); delay(60); }
  pinMode(p, INPUT);
}

// ---------- receive log ----------
struct LogItem {
  uint32_t id;
  decode_type_t type;
  uint64_t value;
  uint16_t bits;
  uint32_t address, command;
};
std::vector<LogItem> logItems;
uint32_t logId = 0;

// ---------- saved codes ----------
struct Saved {
  String name;
  decode_type_t type;
  uint64_t value;
  uint16_t bits;
};
std::vector<Saved> saved;

String u64hex(uint64_t v) {
  char b[20];
  snprintf(b, sizeof(b), "%llX", (unsigned long long)v);
  return String(b);
}
uint64_t parseNum(const String &s) {
  String t = s;
  t.trim();
  if (t.startsWith("0x") || t.startsWith("0X")) return strtoull(t.c_str() + 2, nullptr, 16);
  return strtoull(t.c_str(), nullptr, 10);
}
String cleanName(String n) {
  String o = "";
  for (unsigned i = 0; i < n.length() && o.length() < 24; i++) {
    char c = n[i];
    if (isalnum(c) || c == ' ' || c == '-' || c == '_' || c == '%' || c == '+') o += c;
  }
  if (o.length() == 0) o = "Button";
  return o;
}
void storeSaved() {
  String s = "";
  for (auto &x : saved)
    s += x.name + "|" + String((int)x.type) + "|" + u64hex(x.value) + "|" + String(x.bits) + "\n";
  prefs.putString("codes", s);
}
void loadSaved() {
  saved.clear();
  String s = prefs.getString("codes", "");
  int start = 0;
  while (start < (int)s.length()) {
    int nl = s.indexOf('\n', start);
    if (nl < 0) nl = s.length();
    String line = s.substring(start, nl);
    start = nl + 1;
    int a = line.indexOf('|'), b = line.indexOf('|', a + 1), c = line.indexOf('|', b + 1);
    if (a < 0 || b < 0 || c < 0) continue;
    Saved x;
    x.name = line.substring(0, a);
    x.type = (decode_type_t)line.substring(a + 1, b).toInt();
    x.value = strtoull(line.substring(b + 1, c).c_str(), nullptr, 16);
    x.bits = line.substring(c + 1).toInt();
    saved.push_back(x);
  }
}

// ---------- sending ----------
bool sendCode(decode_type_t type, uint64_t value, uint16_t bits, uint16_t rep) {
  if (!irsend) return false;
  if (irrecv) irrecv->disableIRIn();
  bool ok = irsend->send(type, value, bits, rep);
  if (irrecv) irrecv->enableIRIn();
  return ok;
}
bool sendNecAC(uint32_t addr, uint32_t cmd, uint16_t rep) {
  if (!irsend) return false;
  return sendCode(NEC, irsend->encodeNEC(addr, cmd), 32, rep);
}

// ---------- scanner ----------
bool scanning = false, scanPaused = false;
uint32_t scanAddr = 0, scanCur = 0, scanTo = 255, scanMs = 1500, scanRep = 1;
int32_t scanLast = -1;
uint32_t scanLastT = 0;

void loopScan() {
  if (!scanning || scanPaused) return;
  if (millis() - scanLastT < scanMs) return;
  if (scanCur > scanTo) { scanning = false; return; }
  sendNecAC(scanAddr, scanCur, scanRep);
  scanLast = scanCur;
  scanCur++;
  scanLastT = millis();
}

// ---------- web ----------
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>IR Light Remote</title><style>
:root{--bg:#f4f5f7;--card:#fff;--t:#1d2433;--m:#667085;--a:#e4572e;--b:#d0d5dd}
@media(prefers-color-scheme:dark){:root{--bg:#111318;--card:#1b1e26;--t:#e7e9ee;--m:#9aa3b2;--b:#333845}}
body{margin:0;font-family:-apple-system,system-ui,sans-serif;background:var(--bg);color:var(--t)}
main{max-width:640px;margin:auto;padding:16px}h1{font-size:22px;margin:4px 0 2px}
.s{color:var(--m);font-size:13px;margin-bottom:12px}
section{background:var(--card);border:1px solid var(--b);border-radius:14px;padding:14px;margin:12px 0}
h2{font-size:16px;margin:0 0 8px}button{background:var(--a);color:#fff;border:0;border-radius:10px;padding:11px 14px;font-size:15px;margin:3px 2px}
button.g{background:#5b6475}input{background:var(--bg);color:var(--t);border:1px solid var(--b);border-radius:8px;padding:9px;font-size:15px;width:84px}
.row{display:flex;flex-wrap:wrap;gap:6px;align-items:center;margin:6px 0}.big{font-size:40px;font-weight:700;text-align:center}
.log div{border-bottom:1px solid var(--b);padding:7px 0;font-size:14px}.mono{font-family:ui-monospace,monospace}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(130px,1fr));gap:8px}.grid button{width:100%;margin:0;padding:16px 8px}
.hint{font-size:13px;color:var(--m)}label{font-size:13px;color:var(--m)}
</style></head><body><main>
<h1>IR Light Remote</h1><div class="s" id="st">Loading…</div>

<section id="setup" hidden><h2>Step 1 · Find the IR pins</h2>
<p class="hint">One-time setup. Have one of your old remotes ready.</p>
<div class="row"><button onclick="api('/api/detectrx')">Detect receiver</button></div>
<div class="big" id="detmsg"></div>
<div class="hint mono" id="detres"></div>
<div class="row"><button onclick="txd()">Detect transmitter</button></div>
<div class="hint mono" id="txres"></div>
<h2 style="margin-top:12px">Manual</h2>
<div class="row"><label>RX</label><input id="mrx"><label>TX</label><input id="mtx"></div>
<div class="row"><label>Blink test pin</label><input id="bp"><button class="g" onclick="api('/api/blink?pin='+v('bp'))">Blink</button></div>
<p class="hint">Blink test: watch the clear LED through your phone's <b>front</b> camera. A purple flicker means that pin is the transmitter.</p>
<button onclick="savePins()">Save pins &amp; restart</button></section>

<section id="main" hidden>
<h2>Saved buttons</h2><div class="grid" id="saved"></div>
<p class="hint" id="noSaved">No buttons yet. Save codes from the scanner or the received list.</p></section>

<section id="scan" hidden><h2>Code scanner</h2>
<p class="hint">Sends every command on one address. Watch the light; tap <b>Save last</b> when it reacts.</p>
<div class="row"><label>Address</label><input id="sa" value="0x00"><label>From</label><input id="sf" value="0"><label>To</label><input id="stt" value="255"></div>
<div class="row"><label>Delay ms</label><input id="sm" value="1500"><label>Repeats</label><input id="sr" value="1"></div>
<div class="big mono" id="scur">–</div><div class="hint" id="sstate"></div>
<div class="row"><button onclick="scan('start')">Start</button><button class="g" onclick="scan('pause')">Pause</button><button class="g" onclick="scan('resume')">Resume</button><button class="g" onclick="scan('stop')">Stop</button></div>
<div class="row"><button class="g" onclick="step(-1)">◀ Prev</button><button class="g" onclick="step(0)">Resend</button><button class="g" onclick="step(1)">Next ▶</button><button onclick="saveLast()">Save last</button></div></section>

<section id="manual" hidden><h2>Send one NEC code</h2>
<div class="row"><label>Address</label><input id="na" value="0x00"><label>Command</label><input id="nc" value="0x00"><button onclick="api('/api/sendnec?addr='+e(v('na'))+'&cmd='+e(v('nc'))+'&rep=1')">Send</button>
<button class="g" onclick="saveNec(v('na'),v('nc'))">Save</button></div></section>

<section id="recv" hidden><h2>Received from your remotes</h2>
<p class="hint">Point a remote at the board and press buttons. Address tells you which light family it belongs to.</p>
<div class="log mono" id="log"></div></section>

<section><h2>Settings</h2>
<div class="row"><button class="g" onclick="if(confirm('Redo pin setup?'))api('/api/resetpins')">Redo pin setup</button>
<a href="/update"><button class="g">Firmware update</button></a>
<button class="g" onclick="if(confirm('Forget Wi-Fi? Board will restart into IR_WIFI setup.'))api('/api/wifireset')">Forget Wi-Fi</button></div></section>
</main><script>
const $=i=>document.getElementById(i),v=i=>$(i).value,e=encodeURIComponent;let S={},lastLog=0;
async function api(u){const r=await fetch(u);const t=await r.text();if(t&&t!='ok')alert(t);poll();return t}
function txd(){$('txres').textContent='Testing pins…';fetch('/api/detecttx').then(r=>r.text()).then(t=>{$('txres').textContent=t;poll()})}
function savePins(){api('/api/pins?rx='+e(v('mrx'))+'&tx='+e(v('mtx')))}
function scan(a){let u='/api/scan?action='+a;if(a=='start')u+='&addr='+e(v('sa'))+'&from='+e(v('sf'))+'&to='+e(v('stt'))+'&ms='+e(v('sm'))+'&rep='+e(v('sr'));api(u)}
function step(d){api('/api/scan?action=step&d='+d)}
function saveLast(){if(S.scan.last<0)return alert('Nothing sent yet');saveNec(S.scan.addr,S.scan.last)}
function saveNec(a,c){const n=prompt('Name for this button (e.g. 50%, Timer 6H):');if(n)api('/api/savenec?name='+e(n)+'&addr='+e(a)+'&cmd='+e(c))}
function saveLog(i){const x=S.log.find(l=>l.id==i);const n=prompt('Name for this button:');if(n)api('/api/save?name='+e(n)+'&type='+x.type+'&value=0x'+x.value+'&bits='+x.bits)}
function del(i){if(confirm('Delete '+S.saved[i].name+'?'))api('/api/del?i='+i)}
async function poll(){try{S=await (await fetch('/api/status')).json()}catch(x){return}
$('st').textContent='v'+S.ver+' · '+S.ip+' · RX GPIO'+S.rx+' · TX GPIO'+S.tx;
const setup=S.mode=='setup';$('setup').hidden=!setup;['main','scan','manual','recv'].forEach(k=>$(k).hidden=setup);
if(setup){const m={0:'',1:'Don’t press anything…',2:'NOW press remote buttons repeatedly!',3:''};$('detmsg').textContent=m[S.det];$('detres').textContent=S.detres;
if(S.rx>=0&&!$('mrx').value)$('mrx').value=S.rx;if(S.tx>=0&&document.activeElement!=$('mtx'))$('mtx').value=S.tx}
$('scur').textContent=S.scan.last<0?'–':'cmd '+S.scan.last+' (0x'+S.scan.last.toString(16).toUpperCase().padStart(2,'0')+')';
$('sstate').textContent=S.scan.on?(S.scan.paused?'Paused':'Scanning address '+S.scan.addr+'…'):'Idle';
$('noSaved').hidden=S.saved.length>0;
$('saved').innerHTML=S.saved.map((s,i)=>'<button onclick="api(\'/api/sendsaved?i='+i+'\')" oncontextmenu="del('+i+');return false">'+s.name+'</button>').join('')+(S.saved.length?'<button class="g" onclick="let i=prompt(\'Delete which number? 1-'+S.saved.length+'\');if(i)del(i-1)">Delete…</button>':'');
$('log').innerHTML=S.log.slice().reverse().map(l=>'<div>'+l.proto+(l.proto=='NEC'?' addr <b>0x'+l.address.toString(16).toUpperCase()+'</b> cmd <b>0x'+l.command.toString(16).toUpperCase()+'</b> ('+l.command+')':' 0x'+l.value)+' <button class="g" onclick="api(\'/api/sendlog?id='+l.id+'\')">Send</button><button class="g" onclick="saveLog('+l.id+')">Save</button></div>').join('')||'<div class="hint">Nothing received yet.</div>'}
poll();setInterval(poll,800);
</script></body></html>)HTML";

const char UPDATE_PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Firmware update</title>
<style>body{font-family:-apple-system,system-ui,sans-serif;padding:20px;max-width:500px;margin:auto}button,input{font-size:16px;margin:8px 0}</style></head><body>
<h2>Firmware update</h2><form method="POST" action="/update" enctype="multipart/form-data"><input type="file" name="fw" accept=".bin"><br><button>Upload</button></form>
<p>Keep the board powered until it says OK. It restarts by itself.</p><a href="/">Back</a></body></html>)HTML";

String jsonStatus() {
  String j = "{\"ver\":\"" FW_VERSION "\",\"mode\":\"";
  j += (irsend && irrecv) ? "normal" : "setup";
  j += "\",\"ip\":\"" + (WiFi.getMode() == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "\"";
  j += ",\"rx\":" + String(rxPin) + ",\"tx\":" + String(txPin);
  j += ",\"det\":" + String(detState) + ",\"detres\":\"" + detResult + "\"";
  j += ",\"scan\":{\"on\":" + String(scanning ? "true" : "false") + ",\"paused\":" + String(scanPaused ? "true" : "false") +
       ",\"addr\":" + String(scanAddr) + ",\"last\":" + String(scanLast) + "}";
  j += ",\"log\":[";
  for (size_t i = 0; i < logItems.size(); i++) {
    auto &l = logItems[i];
    if (i) j += ",";
    j += "{\"id\":" + String(l.id) + ",\"proto\":\"" + typeToString(l.type) + "\",\"type\":" + String((int)l.type) +
         ",\"value\":\"" + u64hex(l.value) + "\",\"bits\":" + String(l.bits) + ",\"address\":" + String(l.address) +
         ",\"command\":" + String(l.command) + "}";
  }
  j += "],\"saved\":[";
  for (size_t i = 0; i < saved.size(); i++) {
    if (i) j += ",";
    j += "{\"name\":\"" + saved[i].name + "\"}";
  }
  j += "]}";
  return j;
}

void ok() { server.send(200, "text/plain", "ok"); }
void restartSoon() { server.send(200, "text/plain", "Restarting…"); delay(600); ESP.restart(); }

void setupWeb() {
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/api/status", []() { server.send(200, "application/json", jsonStatus()); });

  server.on("/api/detectrx", []() {
    if (irrecv) return server.send(200, "text/plain", "Use 'Redo pin setup' first.");
    startRxDetect(); ok();
  });
  server.on("/api/detecttx", []() {
    if (irrecv) return server.send(200, "text/plain", "Use 'Redo pin setup' first.");
    server.send(200, "text/plain", txDetect());
  });
  server.on("/api/blink", []() {
    int p = server.arg("pin").toInt();
    if (irrecv || p < 0 || p >= 34 || (p >= 6 && p <= 11) || p == 1 || p == 3)
      return server.send(200, "text/plain", "Invalid pin (or redo pin setup first).");
    blinkPin(p); ok();
  });
  server.on("/api/pins", []() {
    int r = server.arg("rx").toInt(), t = server.arg("tx").toInt();
    if (server.arg("rx") == "" || server.arg("tx") == "" || r == t)
      return server.send(200, "text/plain", "Enter both RX and TX pins.");
    prefs.putInt("rx", r); prefs.putInt("tx", t);
    restartSoon();
  });
  server.on("/api/resetpins", []() { prefs.remove("rx"); prefs.remove("tx"); restartSoon(); });
  server.on("/api/wifireset", []() {
    server.send(200, "text/plain", "Wi-Fi forgotten. Connect to IR_WIFI to set it up again.");
    delay(600);
    WiFiManager wm; wm.resetSettings(); ESP.restart();
  });

  server.on("/api/sendnec", []() {
    if (!sendNecAC(parseNum(server.arg("addr")), parseNum(server.arg("cmd")), server.arg("rep").toInt()))
      return server.send(200, "text/plain", "Send failed (pins not set?)");
    ok();
  });
  server.on("/api/sendlog", []() {
    uint32_t id = server.arg("id").toInt();
    for (auto &l : logItems)
      if (l.id == id) { sendCode(l.type, l.value, l.bits, 0); return ok(); }
    server.send(200, "text/plain", "Not found");
  });
  server.on("/api/sendsaved", []() {
    int i = server.arg("i").toInt();
    if (i < 0 || i >= (int)saved.size()) return server.send(200, "text/plain", "Not found");
    sendCode(saved[i].type, saved[i].value, saved[i].bits, saved[i].type == NEC ? 1 : 0);
    ok();
  });
  server.on("/api/save", []() {
    Saved x{cleanName(server.arg("name")), (decode_type_t)server.arg("type").toInt(), parseNum(server.arg("value")),
            (uint16_t)server.arg("bits").toInt()};
    saved.push_back(x); storeSaved(); ok();
  });
  server.on("/api/savenec", []() {
    if (!irsend) return server.send(200, "text/plain", "Pins not set");
    Saved x{cleanName(server.arg("name")), NEC,
            irsend->encodeNEC(parseNum(server.arg("addr")), parseNum(server.arg("cmd"))), 32};
    saved.push_back(x); storeSaved(); ok();
  });
  server.on("/api/del", []() {
    int i = server.arg("i").toInt();
    if (i >= 0 && i < (int)saved.size()) { saved.erase(saved.begin() + i); storeSaved(); }
    ok();
  });

  server.on("/api/scan", []() {
    String a = server.arg("action");
    if (!irsend) return server.send(200, "text/plain", "Pins not set");
    if (a == "start") {
      scanAddr = parseNum(server.arg("addr"));
      scanCur = parseNum(server.arg("from"));
      scanTo = parseNum(server.arg("to"));
      scanMs = max((uint32_t)200, (uint32_t)parseNum(server.arg("ms")));
      scanRep = server.arg("rep").toInt();
      scanLast = -1; scanning = true; scanPaused = false; scanLastT = 0;
    } else if (a == "pause") scanPaused = true;
    else if (a == "resume") { scanPaused = false; scanLastT = millis(); }
    else if (a == "stop") scanning = false;
    else if (a == "step") {
      scanPaused = true;
      int d = server.arg("d").toInt();
      int32_t c = (scanLast < 0 ? (int32_t)scanCur : scanLast) + d;
      if (c < 0) c = 0;
      sendNecAC(scanAddr, c, scanRep);
      scanLast = c; scanCur = c + 1;
    }
    ok();
  });

  server.on("/update", HTTP_GET, []() { server.send_P(200, "text/html", UPDATE_PAGE); });
  server.on("/update", HTTP_POST,
    []() {
      server.send(200, "text/plain", Update.hasError() ? "Update FAILED. Try again." : "OK. Restarting…");
      delay(800); ESP.restart();
    },
    []() {
      HTTPUpload &u = server.upload();
      if (u.status == UPLOAD_FILE_START) {
        if (irrecv) irrecv->disableIRIn();
        scanning = false;
        Update.begin(UPDATE_SIZE_UNKNOWN);
      } else if (u.status == UPLOAD_FILE_WRITE) {
        Update.write(u.buf, u.currentSize);
      } else if (u.status == UPLOAD_FILE_END) {
        Update.end(true);
      }
    });
  server.begin();
}

void setup() {
  Serial.begin(115200);
  prefs.begin("irr", false);
  rxPin = prefs.getInt("rx", -1);
  txPin = prefs.getInt("tx", -1);
  loadSaved();

  WiFi.setHostname("irremote");
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  if (!wm.autoConnect("IR_WIFI")) {
    // No home Wi-Fi: run our own hotspot so the page still works at 192.168.4.1
    WiFi.mode(WIFI_AP);
    WiFi.softAP("IR_REMOTE");
  }
  MDNS.begin("irremote");
  MDNS.addService("http", "tcp", 80);

  if (rxPin >= 0 && txPin >= 0) {
    irsend = new IRsend(txPin);
    irsend->begin();
    irrecv = new IRrecv(rxPin, 1024, 50, true);
    irrecv->enableIRIn();
  }
  setupWeb();
}

void loop() {
  server.handleClient();
  loopRxDetect();
  loopScan();
  if (irrecv && irrecv->decode(&results)) {
    if (!results.repeat && results.decode_type != UNKNOWN) {
      LogItem l{++logId, results.decode_type, results.value, results.bits, results.address, results.command};
      logItems.push_back(l);
      if (logItems.size() > 20) logItems.erase(logItems.begin());
      Serial.println(resultToHumanReadableBasic(&results));
    }
    irrecv->resume();
  }
  delay(2);
}
