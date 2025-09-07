/**
 * wifi_provisioning.h  —  Minimal SoftAP provisioning for ESP32 (no external libs)
 * 
 * Features:
 *  - Tries stored Wi‑Fi first (NVS via Preferences).
 *  - If it fails, starts SoftAP + captive portal (DNS hijack) at 192.168.4.1.
 *  - Web UI lists nearby SSIDs and lets you save credentials.
 *  - On save, it attempts to connect and shuts the portal when connected.
 * 
 * Usage:
 *   #include "wifi_provisioning.h"
 *   void setup() {
 *     Serial.begin(115200);
 *     wifi_prov::begin("NFC-PAY-Setup"); // AP SSID prefix
 *     if (!wifi_prov::ensureConnected(15000)) {
 *       // Block here until user provisions (optional: make non‑blocking by calling wifi_prov::loop() in your loop())
 *       wifi_prov::blockUntilProvisioned();
 *     }
 *     // ... continue with your normal boot (HTTP calls, NFC, etc.)
 *   }
 *   void loop() { wifi_prov::loop(); /* your code ... */ 

#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>

namespace wifi_prov {

// ===== Config (you can tweak) =====
static const char* NVS_NS         = "wifi";
static const char* NVS_KEY_SSID   = "ssid";
static const char* NVS_KEY_PASS   = "pass";
static const char* AP_PASSWORD    = "nfcsetup";   // 8+ chars for iOS
static const uint16_t DNS_PORT    = 53;
static const uint16_t HTTP_PORT   = 80;
static const uint32_t STA_TIMEOUT = 15000;        // default 15s

// ===== Internals =====
Preferences      prefs;
DNSServer        dnsServer;
WebServer        server(HTTP_PORT);
bool             portalRunning = false;
String           apSsid;
String           pendingSsid, pendingPass;
bool             credsReceived = false;
bool             tryingConnect = false;
bool             connectedOnce = false;
IPAddress        apIP(192,168,4,1);
IPAddress        apGW(192,168,4,1);
IPAddress        apSN(255,255,255,0);

// ---- HTML UI (very small; scans are fetched from /scan.json) ----
String htmlIndex() {
  String s;
  s.reserve(4096);
  s += F("<!doctype html><meta name=viewport content='width=device-width, initial-scale=1'>"
         "<title>Wi‑Fi Setup</title>"
         "<style>body{font-family:system-ui,Segoe UI,Roboto,Arial;margin:24px;}"
         "h1{font-size:20px;margin:0 0 16px}fieldset{border:1px solid #ddd;border-radius:8px;padding:16px}"
         "label{display:block;margin:8px 0 4px}input,select{width:100%;padding:10px;border:1px solid #ccc;border-radius:6px}"
         "button{margin-top:12px;padding:10px 14px;border:0;border-radius:6px;background:#111;color:#fff}"
         "small{color:#666}ul{padding-left:18px}#ok{color:#0a0}</style>");
  s += F("<h1>NFC‑Pay — Wi‑Fi Setup</h1>"
         "<fieldset><legend>Choose network</legend>"
         "<label for=ssid>SSID</label>"
         "<select id=ssid></select>"
         "<label for=pass>Password</label>"
         "<input id=pass type=password placeholder='WPA2 password'>"
         "<button onclick='save()'>Save & Connect</button>"
         "<div id=msg></div>"
         "<p><small>Device IP after connect: will be shown below.</small></p>"
         "</fieldset>"
         "<fieldset style='margin-top:16px'><legend>Status</legend>"
         "<div id=status>Loading…</div></fieldset>"
         "<script>"
         "async function fetchScan(){"
         "  try{let r=await fetch('/scan.json'); if(!r.ok) throw 0; let j=await r.json();"
         "  let sel=document.getElementById('ssid'); sel.innerHTML='';"
         "  j.networks.forEach(n=>{let o=document.createElement('option');o.value=n.ssid;o.textContent=n.ssid+' ('+n.rssi+'dBm)';sel.appendChild(o);});"
         "  }catch(e){document.getElementById('ssid').innerHTML='<option>(scan failed)</option>';}}"
         "async function status(){try{let r=await fetch('/status');let t=await r.text();document.getElementById('status').innerHTML=t;}catch(e){}}"
         "async function save(){"
         "  let ssid=document.getElementById('ssid').value; let pass=document.getElementById('pass').value;"
         "  let r=await fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass)});"
         "  if(r.ok){document.getElementById('msg').innerHTML='<p id=ok>Saved. Connecting…</p>';}else{document.getElementById('msg').textContent='Error saving';}"
         "}"
         "fetchScan(); status(); setInterval(status,2000);"
         "</script>");
  return s;
}

// ---- Utils ----
String mac4() {
  uint64_t mac = ESP.getEfuseMac();
  uint32_t low = (uint32_t)mac;
  char buf[5]; snprintf(buf, sizeof(buf), "%04X", (unsigned)(low & 0xFFFF));
  return String(buf);
}

bool loadCreds(String& ssid, String& pass) {
  prefs.begin(NVS_NS, true);
  ssid = prefs.getString(NVS_KEY_SSID, "");
  pass = prefs.getString(NVS_KEY_PASS, "");
  prefs.end();
  return ssid.length() > 0;
}

void saveCreds(const String& ssid, const String& pass) {
  prefs.begin(NVS_NS, false);
  prefs.putString(NVS_KEY_SSID, ssid);
  prefs.putString(NVS_KEY_PASS, pass);
  prefs.end();
}

void forget() {
  prefs.begin(NVS_NS, false);
  prefs.remove(NVS_KEY_SSID);
  prefs.remove(NVS_KEY_PASS);
  prefs.end();
}

bool trySTA(const String& ssid, const String& pass, uint32_t timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

// ---- Captive portal ----
void startPortal(){
  if (portalRunning) return;

  // AP
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apGW, apSN);
  WiFi.softAP(apSsid.c_str(), AP_PASSWORD);

  // DNS hijack -> 192.168.4.1 for all queries
  dnsServer.start(DNS_PORT, "*", apIP);

  // HTTP handlers
  server.onNotFound([](){ server.sendHeader("Cache-Control","no-cache"); server.send(302,"text/plain",""); });
  server.on("/", [](){
    server.sendHeader("Cache-Control","no-cache");
    server.send(200, "text/html; charset=utf-8", htmlIndex());
  });
  server.on("/scan.json", [](){
    int n = WiFi.scanNetworks();
    String json = "{\"networks\":[";
    for (int i=0;i<n;i++){
      if (i) json += ",";
      json += "{\"ssid\":\"" + String(WiFi.SSID(i)) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]}";
    server.sendHeader("Access-Control-Allow-Origin","*");
    server.sendHeader("Cache-Control","no-cache");
    server.send(200, "application/json", json);
  });
  server.on("/status", [](){
    String h;
    if (WiFi.status()==WL_CONNECTED) {
      h = "Connected to <b>" + WiFi.SSID() + "</b><br>IP: <b>" + WiFi.localIP().toString() + "</b>";
    } else if (tryingConnect) {
      h = "Trying to connect…";
    } else {
      h = "Not connected.";
    }
    server.sendHeader("Cache-Control","no-cache");
    server.send(200, "text/html; charset=utf-8", h);
  });
  server.on("/save", HTTP_POST, [](){
    if (!server.hasArg("ssid")) { server.send(400,"text/plain","missing ssid"); return; }
    pendingSsid = server.arg("ssid");
    pendingPass = server.hasArg("pass") ? server.arg("pass") : "";
    credsReceived = true;
    server.send(200, "application/json", "{\"ok\":true}");
  });

  server.begin();
  if (!MDNS.begin("nfc-pay")) {
    // ignore mDNS failure
  }
  portalRunning = true;
}

void stopPortal(){
  if (!portalRunning) return;
  dnsServer.stop();
  server.stop();
  WiFi.softAPdisconnect(true);
  portalRunning = false;
}

// ---- Public API ----
void begin(const char* apPrefix = "NFC-PAY-Setup"){
  apSsid = String(apPrefix) + "-" + mac4();
}

bool ensureConnected(uint32_t timeoutMs = STA_TIMEOUT){
  String ssid, pass;
  if (loadCreds(ssid, pass)) {
    if (trySTA(ssid, pass, timeoutMs)) {
      connectedOnce = true;
      return true;
    }
  }
  // Start portal
  startPortal();
  return false;
}

// Call this in loop(); returns true when connected (and portal closed)
bool loop(){
  if (portalRunning) {
    dnsServer.processNextRequest();
    server.handleClient();

    if (credsReceived && !tryingConnect) {
      tryingConnect = true;
      saveCreds(pendingSsid, pendingPass);
      // Try to connect in STA while keeping AP up
      if (trySTA(pendingSsid, pendingPass, 20000)) {
        connectedOnce = true;
        // Close portal and keep STA
        stopPortal();
        WiFi.mode(WIFI_STA);
        return true;
      } else {
        tryingConnect = false; // let user try again
      }
    }
  }
  return WiFi.status()==WL_CONNECTED;
}

// Blocking helper: stay here until connected via stored creds or portal
bool blockUntilProvisioned(uint32_t maxMs = 0){
  uint32_t t0 = millis();
  while (true) {
    if (ensureConnected()) return true;
    // portal active; pump
    while (portalRunning) {
      if (loop()) return true;
      if (maxMs && (millis()-t0 > maxMs)) return false;
      delay(5);
    }
  }
}

bool isPortalRunning(){ return portalRunning; }

// For UI buttons
void eraseAndReboot(){
  forget();
  delay(100);
  ESP.restart();
}

} // namespace wifi_prov
