#include "network.h"
#include <ArduinoJson.h>
#include <time.h>
#include "display.h"
#include "eyes.h"
#include "telemetry.h"

DeskNetwork deskNetwork;

// Embedded HTML/CSS/JS web dashboard
static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>DeskOS Dashboard</title>
<style>
:root{--bg:#0a0c10;--card:#141923;--border:#222a38;--accent:#ff3344;--text:#e4ebf5;--muted:#7c8c9e;}
*{box-sizing:border-box;margin:0;padding:0;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;}
body{background:var(--bg);color:var(--text);padding:1.5rem;max-width:700px;margin:auto;}
h1{font-size:1.4rem;display:flex;align-items:center;gap:.6rem;margin-bottom:1.2rem;text-transform:uppercase;letter-spacing:.05em;}
.highlight{color:var(--accent);}
.card{background:var(--card);border:1px solid var(--border);border-radius:8px;padding:1.2rem;margin-bottom:1rem;}
.card h2{font-size:1rem;margin-bottom:.8rem;color:var(--accent);}
.row{margin-bottom:.8rem;display:flex;flex-direction:column;gap:.3rem;}
label{font-size:.85rem;color:var(--muted);}
select,input{background:#0e1117;border:1px solid var(--border);color:var(--text);padding:.6rem;border-radius:4px;outline:none;}
select:focus,input:focus{border-color:var(--accent);}
.btn{background:var(--accent);color:#fff;border:none;padding:.65rem 1rem;border-radius:4px;font-weight:600;cursor:pointer;margin-top:.4rem;}
.btn:hover{background:#ff4d5e;}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:.8rem;}
.stat-box{background:#0e1117;border:1px solid var(--border);padding:.8rem;border-radius:4px;text-align:center;}
.stat-val{font-size:1.8rem;font-weight:700;color:var(--accent);font-family:monospace;}
.stat-lbl{font-size:.75rem;color:var(--muted);}
.status-pill{background:#1a2332;color:#00ff88;padding:.2rem .5rem;border-radius:4px;font-size:.75rem;font-family:monospace;}
</style>
</head>
<body>
<h1>ESP32 <span class="highlight">DeskOS</span> <span class="status-pill" id="conn-pill">ONLINE</span></h1>

<div class="card">
  <h2>Live Display Mode</h2>
  <div class="row">
    <label>Active Screen</label>
    <select id="mode-select" onchange="setMode(this.value)">
      <option value="eyes">Animated Robot Eyes</option>
      <option value="stats">PC System Monitor</option>
      <option value="music">Music Player (Now Playing)</option>
      <option value="qr">QR Code</option>
      <option value="clock">Digital Clock & Date</option>
    </select>
  </div>
  <div class="row">
    <label>Display Brightness (Contrast)</label>
    <input type="range" min="0" max="255" value="255" oninput="setBrightness(this.value)">
  </div>
</div>

<div class="card">
  <h2>Robot Eyes Expression</h2>
  <div class="row">
    <label>Mood</label>
    <select id="mood-select" onchange="setMood(this.value)">
      <option value="neutral">Neutral Focus</option>
      <option value="happy">Happy</option>
      <option value="skeptical">Skeptical</option>
      <option value="angry">Warning / Angry</option>
      <option value="sleepy">Sleepy / Idle</option>
      <option value="surprised">Surprised / Curious</option>
      <option value="love">Heart Eyes (Love)</option>
      <option value="wink">Playful Wink</option>
      <option value="confused">Confused</option>
      <option value="sad">Sad / Puppy Eyes</option>
      <option value="scan">Tactical Scanner</option>
    </select>
  </div>
</div>

<div class="card">
  <h2>PC Telemetry</h2>
  <div class="grid2">
    <div class="stat-box"><div class="stat-val" id="cpu-val">0%</div><div class="stat-lbl">CPU USAGE</div></div>
    <div class="stat-box"><div class="stat-val" id="ram-val">0%</div><div class="stat-lbl">RAM USAGE</div></div>
  </div>
  <div style="margin-top:.8rem;font-size:.8rem;color:var(--muted);text-align:center;">
    Streamed live via <code>pc-agent/agent.py</code>
  </div>
</div>

<div class="card">
  <h2>Generate QR Code</h2>
  <div class="row"><label>URL or Text to encode</label><input type="text" id="qr-input" placeholder="https://github.com/SYN606/ESP8266-DeskOS"></div>
  <button class="btn" onclick="sendQR()">Display QR on OLED</button>
</div>

<div class="card">
  <h2>Wi-Fi Settings</h2>
  <div class="row"><label>Wi-Fi SSID</label><input type="text" id="wifi-ssid"></div>
  <div class="row"><label>Password</label><input type="password" id="wifi-pass"></div>
  <button class="btn" onclick="saveWiFi()">Connect to Wi-Fi</button>
</div>

<script>
function setMode(m){fetch('/api/mode',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode:m})});}
function setMood(m){fetch('/api/mood',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mood:m})});}
function setBrightness(b){fetch('/api/brightness',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({val:parseInt(b)})});}
function sendQR(){
  fetch('/api/qr',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({
    text:document.getElementById('qr-input').value
  })});
}
function saveWiFi(){
  fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({
    ssid:document.getElementById('wifi-ssid').value,
    pass:document.getElementById('wifi-pass').value
  })}).then(()=>{alert('Reconnecting to Wi-Fi...');});
}
setInterval(()=>{
  fetch('/api/status').then(r=>r.json()).then(d=>{
    document.getElementById('cpu-val').textContent=Math.round(d.cpu)+'%';
    document.getElementById('ram-val').textContent=Math.round(d.ram)+'%';
    if(d.mode)document.getElementById('mode-select').value=d.mode.toLowerCase();
    if(d.mood)document.getElementById('mood-select').value=d.mood.toLowerCase();
  }).catch(()=>{});
},2000);
</script>
</body>
</html>
)rawliteral";

DeskNetwork::DeskNetwork()
    : m_server(80),
      m_connected(false),
      m_isAP(false),
      m_lastWiFiCheckMs(0) {
    strncpy(m_ipString, "192.168.4.1", sizeof(m_ipString));
    strncpy(m_ssid, "DeskOS-Setup", sizeof(m_ssid));
}

void DeskNetwork::setupNTP() {
    // 19800 seconds = UTC+5:30 offset
    configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println(F("[NTP] Synchronizing network time..."));
}

void DeskNetwork::begin() {
    WiFi.mode(WIFI_STA);
    if (WiFi.SSID().length() > 0) {
        Serial.printf("[WIFI] Connecting to saved network '%s'...\n", WiFi.SSID().c_str());
        WiFi.begin();
        unsigned long startWait = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startWait < 4000) {
            delay(200);
            Serial.print('.');
        }
        Serial.println();
    } else {
        Serial.println(F("[WIFI] No saved Wi-Fi network found in flash."));
    }

    if (WiFi.status() == WL_CONNECTED) {
        m_connected = true;
        m_isAP = false;
        strncpy(m_ipString, WiFi.localIP().toString().c_str(), sizeof(m_ipString));
        strncpy(m_ssid, WiFi.SSID().c_str(), sizeof(m_ssid));
        Serial.printf("[WIFI] Connected to '%s'! IP: %s\n", m_ssid, m_ipString);
        setupNTP();
    } else {
        // Fall back to SoftAP configuration mode
        m_connected = false;
        m_isAP = true;
        WiFi.mode(WIFI_AP);
        WiFi.softAP("DeskOS-Setup");
        IPAddress apIP = WiFi.softAPIP();
        strncpy(m_ipString, apIP.toString().c_str(), sizeof(m_ipString));
        strncpy(m_ssid, "DeskOS-Setup", sizeof(m_ssid));
        Serial.printf("[WIFI] Started Access Point 'DeskOS-Setup' (IP: %s)\n", m_ipString);
    }

    setupServerRoutes();
    m_server.begin();
    Serial.println(F("[HTTP] Web server listening on port 80"));
}

void DeskNetwork::connectWiFi(const char* ssid, const char* password) {
    if (!ssid || strlen(ssid) == 0) return;

    Serial.printf("[WIFI] Connecting to new network '%s'...\n", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
        delay(200);
    }

    if (WiFi.status() == WL_CONNECTED) {
        m_connected = true;
        m_isAP = false;
        strncpy(m_ipString, WiFi.localIP().toString().c_str(), sizeof(m_ipString));
        strncpy(m_ssid, ssid, sizeof(m_ssid));
        Serial.printf("[WIFI] Successfully connected! IP: %s\n", m_ipString);
        setupNTP();
    } else {
        Serial.println(F("[WIFI] Failed to connect. Returning to AP mode."));
        WiFi.mode(WIFI_AP);
        WiFi.softAP("DeskOS-Setup");
        m_connected = false;
        m_isAP = true;
        strncpy(m_ipString, WiFi.softAPIP().toString().c_str(), sizeof(m_ipString));
    }
}

int DeskNetwork::getRSSI() const {
    return m_connected ? WiFi.RSSI() : 0;
}

void DeskNetwork::getTimeString(char* buf, size_t len) {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    if (t && t->tm_year > 70) {
        snprintf(buf, len, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    } else {
        // Fallback to millis elapsed time if NTP is not yet synced
        unsigned long sec = millis() / 1000;
        unsigned int h = (sec / 3600) % 24;
        unsigned int m = (sec / 60) % 60;
        unsigned int s = sec % 60;
        snprintf(buf, len, "%02u:%02u:%02u", h, m, s);
    }
}

void DeskNetwork::getDateString(char* buf, size_t len) {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    if (t && t->tm_year > 70) {
        static const char* const DAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        static const char* const MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        snprintf(buf, len, "%s, %s %02d %d",
                 DAYS[t->tm_wday], MONTHS[t->tm_mon], t->tm_mday, t->tm_year + 1900);
    } else {
        snprintf(buf, len, "DeskOS Active");
    }
}

void DeskNetwork::setupServerRoutes() {
    // Serve dashboard homepage
    m_server.on("/", HTTP_GET, [this]() {
        m_server.send_P(200, "text/html", DASHBOARD_HTML);
    });

    // API: Status snapshot
    m_server.on("/api/status", HTTP_GET, [this]() {
        JsonDocument doc;
        doc["mode"] = deskDisplay.getModeString();
        doc["mood"] = robotEyes.getMoodString();
        doc["brightness"] = deskDisplay.getBrightness();
        doc["wifi_ssid"] = m_ssid;
        doc["wifi_connected"] = m_connected;
        doc["wifi_rssi"] = getRSSI();
        doc["ip"] = m_ipString;
        doc["free_heap"] = ESP.getFreeHeap();
        doc["uptime_sec"] = millis() / 1000;
        doc["cpu"] = globalTelemetry.cpuPercent;
        doc["ram"] = globalTelemetry.ramPercent;

        String out;
        serializeJson(doc, out);
        m_server.send(200, "application/json", out);
    });

    // API: Change display mode
    m_server.on("/api/mode", HTTP_POST, [this]() {
        if (!m_server.hasArg("plain")) { m_server.send(400, "text/plain", "Missing body"); return; }
        JsonDocument doc;
        deserializeJson(doc, m_server.arg("plain"));
        const char* mode = doc["mode"];
        if (mode) deskDisplay.setMode(mode);
        m_server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // API: Change eye mood
    m_server.on("/api/mood", HTTP_POST, [this]() {
        if (!m_server.hasArg("plain")) { m_server.send(400, "text/plain", "Missing body"); return; }
        JsonDocument doc;
        deserializeJson(doc, m_server.arg("plain"));
        const char* mood = doc["mood"];
        if (mood) robotEyes.setMood(mood);
        m_server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // API: Set brightness
    m_server.on("/api/brightness", HTTP_POST, [this]() {
        if (!m_server.hasArg("plain")) { m_server.send(400, "text/plain", "Missing body"); return; }
        JsonDocument doc;
        deserializeJson(doc, m_server.arg("plain"));
        uint8_t val = doc["val"] | 255;
        deskDisplay.setBrightness(val);
        m_server.send(200, "application/json", "{\"status\":\"ok\"}");
    });



    // API: Connect to Wi-Fi
    m_server.on("/api/wifi", HTTP_POST, [this]() {
        if (!m_server.hasArg("plain")) { m_server.send(400, "text/plain", "Missing body"); return; }
        JsonDocument doc;
        deserializeJson(doc, m_server.arg("plain"));
        const char* ssid = doc["ssid"];
        const char* pass = doc["pass"];
        m_server.send(200, "application/json", "{\"status\":\"connecting\"}");
        connectWiFi(ssid, pass);
    });

    // API: Post telemetry
    m_server.on("/api/stats", HTTP_POST, [this]() {
        if (!m_server.hasArg("plain")) { m_server.send(400, "text/plain", "Missing body"); return; }
        JsonDocument doc;
        deserializeJson(doc, m_server.arg("plain"));
        globalTelemetry.cpuPercent = doc["cpu"] | 0.0f;
        globalTelemetry.ramPercent = doc["ram"] | 0.0f;
        const char* up = doc["uptime"];
        if (up) strncpy(globalTelemetry.uptimeStr, up, sizeof(globalTelemetry.uptimeStr) - 1);
        globalTelemetry.netKbIn = doc["net_in"] | 0.0f;
        globalTelemetry.netKbOut = doc["net_out"] | 0.0f;
        globalTelemetry.hasData = true;
        globalTelemetry.lastReceivedMs = millis();
        bool autoShow = doc["auto_show"].is<bool>() ? doc["auto_show"].as<bool>() : true;
        if (autoShow) {
            if (deskDisplay.getMode() != DisplayMode::MUSIC_PLAYER || !globalMedia.isPlaying) {
                deskDisplay.setMode(DisplayMode::PC_STATS);
            }
        }
        m_server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // API: Push QR Code
    m_server.on("/api/qr", HTTP_POST, [this]() {
        if (!m_server.hasArg("plain")) { m_server.send(400, "text/plain", "Missing body"); return; }
        JsonDocument doc;
        deserializeJson(doc, m_server.arg("plain"));
        const char* text = doc["text"];
        if (text) deskDisplay.setQRCode(text);
        deskDisplay.setMode(DisplayMode::QR_CODE);
        m_server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    // API: Push Music / Now Playing info
    m_server.on("/api/media", HTTP_POST, [this]() {
        if (!m_server.hasArg("plain")) { m_server.send(400, "text/plain", "Missing body"); return; }
        JsonDocument doc;
        deserializeJson(doc, m_server.arg("plain"));
        const char* title = doc["title"];
        const char* artist = doc["artist"];
        bool isPlaying = doc["playing"] | false;
        deskDisplay.setMedia(title, artist, isPlaying);
        if (doc["auto_show"] | false) {
            deskDisplay.setMode(DisplayMode::MUSIC_PLAYER);
        }
        m_server.send(200, "application/json", "{\"status\":\"ok\"}");
    });
}

void DeskNetwork::update() {
    m_server.handleClient();

    // Check Wi-Fi state every 5 seconds
    unsigned long now = millis();
    if (now - m_lastWiFiCheckMs >= 5000) {
        m_lastWiFiCheckMs = now;
        if (!m_isAP) {
            m_connected = (WiFi.status() == WL_CONNECTED);
            if (m_connected) {
                strncpy(m_ipString, WiFi.localIP().toString().c_str(), sizeof(m_ipString));
            }
        }
    }
}
