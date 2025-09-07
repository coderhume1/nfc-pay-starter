/*
  ESP32 + PN532 NFC Provisioner with SoftAP Wi‑Fi Provisioning, LED & Buzzer Payment Status,
  and Factory Reset (button + serial).

  - Starts with stored Wi‑Fi (NVS). If not found/failed, opens SoftAP portal:
      SSID: NFC-PAY-Setup-XXXX
      PASS: nfcsetup
    Visit http://192.168.4.1 to select SSID and save password.
  - After Wi‑Fi connects, device:
      1) GET  /api/v1/bootstrap (auto-enrolls device, gets terminal/amount/currency/URL)
      2) POST /api/v1/sessions    (creates "pending" session)
      3) Writes NDEF URI (checkoutUrl) to NTAG21x; verifies; turns RF OFF
      4) Polls GET /api/v1/sessions/{id} until status == "paid" (timeout -> error pattern)
  - Indicators:
      Pending: LED slow blink + short chirp every 2s
      Paid:    LED solid ON + 3 quick chirps
      Error:   LED fast blink + chirp every 500ms
  - Factory Reset (clears saved Wi‑Fi and reboots):
      * Hold BUTTON_PIN LOW on boot for ~3 seconds
      * Or Serial command: FACTORY / RESETWIFI / FORGET

  Requirements (Arduino IDE):
    - ESP32 core (tested with 2.0.15)
    - Libraries from ESP32 core: WiFi, WiFiClientSecure, WebServer, DNSServer, Preferences, ESPmDNS
    - Adafruit PN532 library
*/

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "wifi_provisioning.h"  // SoftAP provisioning (no external deps)

// ====== EDIT THESE ======
// Your Netlify deployment base URL and API key (must match server env)
const char* BASE_URL  = "https://spiffy-elf-7b7eb0.netlify.app";  // <-- set
const char* API_KEY   = "esp32_test_api_key_123456";        // <-- set

// Status pins
#define LED_PIN       2     // Built-in LED on many ESP32 boards
#define BUZZER_PIN    15    // Active buzzer (for passive use tone())
#define BUTTON_PIN    34    // Input-only pin, attach to GND via button for factory reset

// PN532 SPI pins
#define PN532_SCK     18
#define PN532_MOSI    23
#define PN532_MISO    19
#define PN532_SS       5
#define PN532_RST     27
Adafruit_PN532 nfc(PN532_SS);

// ---- Runtime config from server ----
String targetURL = "";
String terminalId = "";
int    amount = 0;
String currency = "";
String sessionId = "";

// ---- Device state machines ----
enum class State { ARMING, PROVISION, POLL, VERIFIED_RF_OFF, IDLE_RF_OFF };
State state = State::ARMING;
bool armed  = true;
bool rfOff  = false;

// Payment indicator state
enum class PayStat { NONE, PENDING, PAID, ERROR_ };
PayStat payStatus = PayStat::NONE;

// timings
const uint32_t POLL_MS_SHORT = 1000;   // poll every 1s initially
const uint32_t POLL_MS_LONG  = 3000;   // after 60s, slow down
const uint32_t POLL_SLOW_AFTER_MS = 60000;
const uint32_t POLL_TIMEOUT_MS     = 180000; // give up after 3 minutes

uint32_t pollLast = 0;
uint32_t startPollingAt = 0;

// ---------- Indicators (non-blocking blink/beep) ----------
bool ledOn = false;
bool buzOn = false;
uint32_t lastBlink = 0;
uint32_t lastChirp = 0;
uint32_t chirpEndsAt = 0;
int paidChirpsRemaining = 0;

void setLED(bool on){ ledOn = on; digitalWrite(LED_PIN, on ? HIGH : LOW); }
void setBuzzer(bool on){ buzOn = on; digitalWrite(BUZZER_PIN, on ? HIGH : LOW); }

void setPayStatus(PayStat s){
  payStatus = s;
  lastBlink = lastChirp = chirpEndsAt = 0;
  paidChirpsRemaining = 0;
  setBuzzer(false);

  if (s == PayStat::PENDING) {
    setLED(false); // start blinking
  } else if (s == PayStat::PAID) {
    setLED(true);
    paidChirpsRemaining = 3; // do 3 quick chirps once
  } else if (s == PayStat::ERROR_) {
    setLED(false); // fast blink pattern will start
  } else {
    setLED(false);
  }
}

void tickIndicators(){
  uint32_t now = millis();

  if (payStatus == PayStat::PENDING){
    // LED: 500ms on/off
    if (now - lastBlink >= 500){ setLED(!ledOn); lastBlink = now; }
    // Buzzer: 40ms chirp every 2s
    if (!buzOn && (now - lastChirp >= 2000)){ setBuzzer(true); chirpEndsAt = now + 40; lastChirp = now; }
    if (buzOn && now >= chirpEndsAt){ setBuzzer(false); }
  }
  else if (payStatus == PayStat::PAID){
    // LED solid ON
    if (paidChirpsRemaining > 0){
      if (!buzOn && (now - lastChirp >= 100)){ // gap between chirps
        setBuzzer(true); chirpEndsAt = now + 100; lastChirp = now;
      }
      if (buzOn && now >= chirpEndsAt){ setBuzzer(false); paidChirpsRemaining--; lastChirp = now; }
    }
  }
  else if (payStatus == PayStat::ERROR_){
    // LED fast blink 100ms
    if (now - lastBlink >= 100){ setLED(!ledOn); lastBlink = now; }
    // Buzzer 60ms every 500ms
    if (!buzOn && (now - lastChirp >= 500)){ setBuzzer(true); chirpEndsAt = now + 60; lastChirp = now; }
    if (buzOn && now >= chirpEndsAt){ setBuzzer(false); }
  }
}

// ---------- PN532 helpers (robust routines) ----------
bool rfFieldOn(bool on) {
  uint8_t cmd[] = { PN532_COMMAND_RFCONFIGURATION, 0x01, (uint8_t)(on ? 0x01 : 0x00) };
  return nfc.sendCommandCheckAck(cmd, sizeof(cmd), 100);
}
void dumpPage(uint8_t page){
  uint8_t b[4];
  if(nfc.mifareultralight_ReadPage(page,b)){
    Serial.print("P"); Serial.print(page); Serial.print(": ");
    for(int i=0;i<4;i++){ if(b[i]<16)Serial.print('0'); Serial.print(b[i],HEX); Serial.print(' '); }
    Serial.println();
  } else { Serial.print("P"); Serial.print(page); Serial.println(": <read err>"); }
}
bool readPage(uint8_t page, uint8_t out[4]) { return nfc.mifareultralight_ReadPage(page, out); }
bool writeCC_NTAG213() { uint8_t cc[4] = { 0xE1, 0x10, 0x12, 0x00 }; return nfc.mifareultralight_WritePage(3, cc); }
uint8_t uriPrefixCode(const String& url, String& rem) {
  struct Map{ const char* pre; uint8_t code; } map[] = {
    {"http://www.", 0x01},{"https://www.",0x02},{"http://",0x03},{"https://",0x04}
  };
  for (auto &m: map) { size_t L=strlen(m.pre); if(url.startsWith(m.pre)){ rem=url.substring(L); return m.code; } }
  rem = url; return 0x00;
}
size_t buildURI_TLV(const String& url, uint8_t* out, size_t maxlen) {
  String rem; uint8_t id = uriPrefixCode(url, rem);
  size_t payloadLen = 1 + rem.length();
  size_t ndefLen = 3 + 1 + payloadLen;
  size_t tlvLen = 2 + ndefLen + 1;
  if (tlvLen > 255 || tlvLen > maxlen) return 0;
  size_t i=0;
  out[i++]=0x03; out[i++]=(uint8_t)ndefLen;
  out[i++]=0xD1; out[i++]=0x01; out[i++]=(uint8_t)payloadLen;
  out[i++]=0x55; out[i++]=id;
  for(size_t k=0;k<rem.length();k++) out[i++]=(uint8_t)rem[k];
  out[i++]=0xFE;
  return i;
}
bool writePagesPaced(const uint8_t* data, size_t len, uint8_t startPage=4) {
  uint8_t pageBuf[4]; size_t idx=0; uint8_t page=startPage;
  while(idx<len){
    memset(pageBuf,0x00,4);
    for(uint8_t k=0;k<4 && idx<len;k++) pageBuf[k]=data[idx++];
    if(!nfc.mifareultralight_WritePage(page,pageBuf)) return false;
    delay(12);
    page++;
  }
  memset(pageBuf,0x00,4);
  nfc.mifareultralight_WritePage(page,pageBuf);
  delay(12);
  return true;
}
bool ndefQuickMatchesURL(const String& url) {
  uint8_t p4[4], p5[4];
  if(!readPage(4,p4) || !readPage(5,p5)) return false;
  if(p4[0]!=0x03) return false;
  if(p4[2]!=0xD1 || p4[3]!=0x01) return false;
  String rem; uint8_t expId = uriPrefixCode(url, rem);
  if(p5[0]!=expId) return false;
  for(int i=0;i<3 && i<(int)rem.length();i++){
    uint8_t b = (i==0)?p5[1]:(i==1)?p5[2]:p5[3];
    if((uint8_t)rem[i]!=b) return false;
  }
  return true;
}
bool readRangeToBuf(uint8_t startPage, size_t byteCount, uint8_t* out){
  size_t got=0; uint8_t page=startPage;
  while(got<byteCount){
    if(!readPage(page, &out[got])) return false;
    got += 4; page++;
  }
  return true;
}
bool verifyTLVExactWithRetries(const uint8_t* tlv, size_t tlvLen){
  for(int attempt=1; attempt<=3; attempt++){
    rfFieldOn(false); delay(200);
    rfFieldOn(true);  delay(20);
    uint8_t uid[7], uidLen=0; (void)nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 200);
    uint8_t buf[192]; if(tlvLen > sizeof(buf)) return false;
    if(!readRangeToBuf(4, tlvLen, buf)) continue;
    bool same=true; for(size_t i=0;i<tlvLen;i++){ if(buf[i]!=tlv[i]){ same=false; break; } }
    if(same) return true;
  }
  return false;
}

// ---------- Tiny JSON helpers ----------
String jsonStr(const String& body, const char* key){
  String pat = String("\"") + key + "\":\""; int i=body.indexOf(pat); if(i<0) return "";
  i+=pat.length(); int j=body.indexOf('\"',i); if(j<0) return ""; return body.substring(i,j);
}
int jsonInt(const String& body, const char* key){
  String pat = String("\"") + key + "\":"; int i=body.indexOf(pat); if(i<0) return 0;
  i+=pat.length(); int j=i; while(j<(int)body.length() && strchr("0123456789", body[j])) j++;
  return body.substring(i,j).toInt();
}

// ---------- HTTP ----------
String chipIdHex(){ uint64_t mac = ESP.getEfuseMac(); char buf[17]; snprintf(buf, sizeof(buf), "%012llX", (unsigned long long)mac); return String(buf); }

bool httpGET(const String& url, String& out){
  WiFiClientSecure c; c.setInsecure(); // For production: pin CA
  HTTPClient http; if(!http.begin(c,url)) return false;
  http.addHeader("X-API-Key", API_KEY);
  http.addHeader("X-Device-Id", chipIdHex());
  int code = http.GET(); out = http.getString(); http.end();
  Serial.printf("[HTTP] GET %s -> %d\n", url.c_str(), code);
  return code>=200 && code<300;
}
bool httpPOST(const String& url, const String& json, String& out){
  WiFiClientSecure c; c.setInsecure();
  HTTPClient http; if(!http.begin(c,url)) return false;
  http.addHeader("Content-Type","application/json");
  http.addHeader("X-API-Key", API_KEY);
  http.addHeader("X-Device-Id", chipIdHex());
  int code = http.POST(json); out=http.getString(); http.end();
  Serial.printf("[HTTP] POST %s -> %d\n", url.c_str(), code);
  return code>=200 && code<300;
}

// ---------- Server integration ----------
bool fetchConfig() {
  String body;
  String url = String(BASE_URL) + "/api/v1/bootstrap?deviceId=" + chipIdHex();
  if(!httpGET(url, body)) return false;
  terminalId = jsonStr(body, "terminalId");
  amount     = jsonInt(body, "amount");
  currency   = jsonStr(body, "currency");
  targetURL  = jsonStr(body, "checkoutUrl");
  Serial.printf("[CFG] terminalId=%s amount=%d currency=%s\n", terminalId.c_str(), amount, currency.c_str());
  Serial.printf("[CFG] checkoutUrl=%s\n", targetURL.c_str());
  return targetURL.length() > 0;
}

bool createSession() {
  String url  = String(BASE_URL) + "/api/v1/sessions";
  String json = terminalId.length() ? (String("{\"terminalId\":\"") + terminalId + "\"}") : String("{}");
  String resp;
  if(!httpPOST(url, json, resp)) { Serial.println(resp); return false; }
  sessionId = jsonStr(resp, "id");
  Serial.printf("[SESSION] id=%s\n", sessionId.c_str());
  return sessionId.length() > 0;
}

String getSessionStatus(){
  if(sessionId.length()==0) return "";
  String url = String(BASE_URL) + "/api/v1/sessions/" + sessionId;
  String body;
  if(!httpGET(url, body)) return ""; // network error
  return jsonStr(body, "status");     // "pending" | "paid"
}

// ---------- Serial commands ----------
void handleSerial() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n'); line.trim();
  if (line.equalsIgnoreCase("ARM")) {
    armed = true; state = State::ARMING;
    if (rfOff){ rfFieldOn(true); rfOff=false; }
    Serial.println("[CTRL] Provisioning ARMED. RF ON.");
    return;
  }
  if (line.equalsIgnoreCase("FACTORY") || line.equalsIgnoreCase("RESETWIFI") || line.equalsIgnoreCase("FORGET")) {
    Serial.println("[CTRL] Factory reset Wi‑Fi and reboot...");
    wifi_prov::eraseAndReboot();
    return;
  }
  if (line.length()>0) {
    targetURL = line;
    Serial.print("[CTRL] targetURL set to: "); Serial.println(targetURL);
  }
}

// ---------- Provisioning ----------
bool provisionOnce() {
  // select tag (best-effort)
  uint8_t uid[7]; uint8_t uidLen=0;
  (void)nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 200);

  // Ensure CC
  bool needFormat=true; uint8_t p3[4];
  if(readPage(3,p3)){ if(p3[0]==0xE1 && p3[1]==0x10 && p3[2]==0x12) needFormat=false; }
  if(needFormat){
    Serial.println("[PROV] Writing CC for NTAG213...");
    if(!writeCC_NTAG213()){ Serial.println("[ERR] CC write failed."); return false; }
    delay(12);
  } else { Serial.println("[PROV] CC present."); }

  if(ndefQuickMatchesURL(targetURL)){
    Serial.println("[PROV] Existing NDEF matches. Skipping write.");
  } else {
    uint8_t tlv[160];
    size_t tlvLen = buildURI_TLV(targetURL, tlv, sizeof(tlv));
    if(tlvLen==0){ Serial.println("[ERR] URL too long for short TLV."); return false; }
    Serial.printf("[PROV] Writing NDEF TLV (%u bytes)...\n", (unsigned)tlvLen);
    if(!writePagesPaced(tlv, tlvLen, 4)){ Serial.println("[ERR] Write failed."); return false; }
    Serial.println("[PROV] Write OK.");
    delay(40);
    if(!verifyTLVExactWithRetries(tlv, tlvLen)){
      Serial.println("[ERR] Verify failed.");
      Serial.println("[DBG] Dump pages 3..12:");
      for(uint8_t pg=3; pg<=12; pg++) dumpPage(pg);
      return false;
    }
    Serial.println("[PROV] Verify OK.");
    return true;
  }

  // If skipped write, still exact verify
  uint8_t tlv[160]; size_t tlvLen = buildURI_TLV(targetURL, tlv, sizeof(tlv));
  if(tlvLen==0){ Serial.println("[ERR] URL too long for short TLV."); return false; }
  if(!verifyTLVExactWithRetries(tlv, tlvLen)){
    Serial.println("[ERR] Verify failed (skip path).");
    Serial.println("[DBG] Dump pages 3..12:");
    for(uint8_t pg=3; pg<=12; pg++) dumpPage(pg);
    return false;
  }
  Serial.println("[PROV] Verify OK.");
  return true;
}

// ---------- Factory reset via button (hold low on boot) ----------
void checkFactoryButtonOnBoot(){
  pinMode(BUTTON_PIN, INPUT_PULLUP); // expects button to pull to GND
  delay(10);
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("[BOOT] Factory button held. Hold ~3s to confirm...");
    uint32_t t0 = millis();
    while (digitalRead(BUTTON_PIN) == LOW) {
      if (millis() - t0 > 3000) {
        Serial.println("[BOOT] Factory reset Wi‑Fi now...");
        wifi_prov::eraseAndReboot();
      }
      delay(50);
    }
    Serial.println("[BOOT] Factory reset canceled.");
  }
}

// ---------- Arduino entry ----------
void setup() {
  Serial.begin(115200); delay(50);
  Serial.println("\nESP32 NFC + SoftAP Provisioning + Indicators + Factory Reset");

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  setLED(false); setBuzzer(false);

  checkFactoryButtonOnBoot();

  // PN532 init
  pinMode(PN532_RST, OUTPUT);
  digitalWrite(PN532_RST, LOW); delay(10);
  digitalWrite(PN532_RST, HIGH); delay(10);
  nfc.begin();
  uint32_t ver = nfc.getFirmwareVersion();
  if (!ver) { Serial.println("[ERR] PN532 not found. Check SPI wiring & mode."); }
  else { Serial.print("[INFO] PN532 FW: 0x"); Serial.println(ver, HEX); nfc.SAMConfig(); }

  // Wi‑Fi provisioning
  wifi_prov::begin("NFC-PAY-Setup"); // AP SSID prefix
  if (!wifi_prov::ensureConnected(15000)) {
    Serial.println("[PROV] Starting SoftAP portal. Connect to Wi‑Fi SSID above (pass: nfcsetup)");
    wifi_prov::blockUntilProvisioned(); // blocks until connected
  }
  Serial.printf("[NET] Connected: %s  IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

  // Bootstrap & create session
  if(fetchConfig() && createSession()){
    setPayStatus(PayStat::PENDING);
  } else {
    Serial.println("[CFG] Bootstrap/session failed; fallback to ADMIN_TEST.");
    terminalId = "ADMIN_TEST";
    targetURL = String(BASE_URL) + "/p/" + terminalId;
    setPayStatus(PayStat::ERROR_);
  }

  rfFieldOn(true); rfOff=false;
  Serial.print("[INFO] Write URL: "); Serial.println(targetURL);
  Serial.println("[INFO] Send 'ARM' to re-arm later; send 'FACTORY' to erase Wi‑Fi & reboot.");

  state = State::PROVISION;
}

void loop() {
  wifi_prov::loop(); // handle portal if somehow still running
  handleSerial();
  tickIndicators();

  switch(state){
    case State::ARMING:
      if(armed && rfOff){ rfFieldOn(true); rfOff=false; }
      if(armed) state = State::PROVISION;
      break;

    case State::PROVISION: {
      bool ok = provisionOnce();
      if (ok) {
        if (rfFieldOn(false)) { rfOff=true; Serial.println("[DONE] RF OFF for phone read. Polling payment..."); }
        else { Serial.println("[WARN] RF off cmd failed; retrying..."); rfFieldOn(false); rfOff=true; }
        state = State::POLL;
        startPollingAt = millis();
        pollLast = 0;
      } else {
        Serial.println("[RETRY] Provision failed. Check tag & wiring, then 'ARM' to try again.");
        state = State::IDLE_RF_OFF;
      }
    } break;

    case State::POLL: {
      uint32_t now = millis();
      uint32_t pollEvery = (now - startPollingAt) > POLL_SLOW_AFTER_MS ? POLL_MS_LONG : POLL_MS_SHORT;

      if (now - startPollingAt > POLL_TIMEOUT_MS){
        Serial.println("[POLL] Timeout waiting for payment.");
        setPayStatus(PayStat::ERROR_);
        state = State::VERIFIED_RF_OFF;
        break;
      }

      if (now - pollLast >= pollEvery){
        pollLast = now;
        String st = getSessionStatus();
        if (st == "paid"){
          Serial.println("[POLL] Paid detected!");
          setPayStatus(PayStat::PAID);
          state = State::VERIFIED_RF_OFF; // stay idle; user can 'ARM' to re-provision
        } else if (st == "pending" || st == ""){
          // pending or transient error; keep waiting
        } else {
          Serial.print("[POLL] Unexpected status: "); Serial.println(st);
          setPayStatus(PayStat::ERROR_);
          state = State::VERIFIED_RF_OFF;
        }
      }
    } break;

    case State::VERIFIED_RF_OFF:
    case State::IDLE_RF_OFF:
      delay(50);
      break;
  }
}
