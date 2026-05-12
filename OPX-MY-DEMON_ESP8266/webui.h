#ifndef WEBUI_H
#define WEBUI_H

#include <Arduino.h>
#include <LittleFS.h>
#include "config.h"
#include "language.h"

extern unsigned long startTime;

static String pageHeader() {
  String h = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>"
    "<title>OPX-MY-DEMON</title><style>"
    ":root{--accent:#00ff00;--bg:#000;--text:#fff;--dim:#555}"
    "*{margin:0;padding:0;box-sizing:border-box}"
    "body{background:var(--bg);color:var(--text);font-family:'Consolas','Courier New',monospace;font-size:12px;line-height:1.4}"
    ".container{max-width:900px;margin:0 auto;padding:20px 10px}"
    ".hdr-box{border:1px solid var(--accent);padding:20px;position:relative;text-align:center;margin-bottom:30px}"
    ".hdr-box h1{font-size:24px;letter-spacing:4px;text-transform:uppercase;color:var(--text);text-shadow:0 0 10px var(--accent)}"
    ".hdr-box::before,.hdr-box::after{content:'';position:absolute;width:10px;height:10px;border:2px solid var(--accent)}"
    ".hdr-box::before{top:-2px;left:-2px;border-right:0;border-bottom:0}"
    ".hdr-box::after{bottom:-2px;right:-2px;border-left:0;border-top:0}"
    ".sec{font-size:14px;color:var(--accent);margin:25px 0 10px 0;border-bottom:1px solid #222;padding-bottom:5px;font-weight:bold}"
    ".sec::before{content:'root@rt18720:~#';color:var(--dim);font-weight:normal}"
    ".sec::after{content:'_';animation:blink 1s infinite}"
    "@keyframes blink{0%,100%{opacity:1}50%{opacity:0}}"
    ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(120px,1fr));gap:8px;margin-bottom:15px}"
    ".btn{background:0 0;border:1px solid var(--dim);color:var(--text);padding:10px 5px;text-align:center;text-transform:uppercase;font-size:11px;cursor:pointer;display:block;text-decoration:none;font-weight:700;transition:.2s}"
    ".btn:hover{border-color:var(--accent);color:var(--accent)}"
    ".btn-red{border-color:#f00;color:#f00}.btn-green{border-color:#0f0;color:#0f0}"
    "table{width:100%;border-collapse:collapse;margin-top:10px}"
    "th{background:var(--accent);color:#000;text-align:left;padding:8px;font-size:11px;text-transform:uppercase}"
    "td{padding:8px;border-bottom:1px solid #111;font-size:11px}"
    "input,select{background:#000;border:1px solid var(--dim);color:var(--text);padding:8px;font-family:inherit;width:100%}"
    ".log-box{background:#050505;border:1px solid #1a1a1a;padding:10px;height:250px;overflow-y:auto;color:#0f0}"
    ".info-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}"
    ".badge{border:1px solid var(--accent);padding:2px 6px;font-size:10px;color:var(--accent)}"
    ".capture-box{border:1px solid var(--accent);padding:10px;margin-top:8px;background:#050505;font-size:11px}"
    ".pw{color:var(--accent);font-weight:bold}"
    ".info-item{padding:8px;border:1px solid #1a1a1a}"
    ".info-item .lbl{font-size:9px;color:var(--dim)}"
    ".info-item .val{font-size:13px;margin-top:2px}"
    ".green{color:var(--accent)}.red{color:#f44336}"
    ".mb-8{margin-bottom:8px}.mt-8{margin-top:8px}"
    ".empty{text-align:center;padding:40px 20px;color:var(--dim)}"
    ".footer{text-align:center;padding:20px;color:var(--dim);font-size:10px}"
    ".flex{display:flex;gap:10px;flex-wrap:wrap}</style>"
    "<script>"
    "function setTheme(n){"
    "const t={'CYBER':'#00ffff','TERMINAL':'#00ff00','RED':'#ff0000','VAPORWAVE':'#ff00ff'};"
    "document.documentElement.style.setProperty('--accent',t[n]||'#00ff00');"
    "localStorage.setItem('theme',n);"
    "}"
    "window.onload=()=>{setTheme(localStorage.getItem('theme')||'TERMINAL');}"
    "</script></head><body>");
  return h;
}

static String navBar(uint8_t active, uint8_t lang) {
  if (active == PAGE_SCAN) {
    return F("<div class='hdr-box'><div style='font-size:24px;font-weight:bold;color:var(--accent);text-shadow:0 0 10px var(--accent);letter-spacing:2px'>OPX-MY-DEMON</div><div style='font-size:10px;color:var(--dim);margin-top:6px'>v1.0.1 | developer op aminul ff | demo</div></div>");
  }
  static const char titleTable[] PROGMEM = "SCAN\0ATTACK\0LOGS\0SETTINGS\0FILE MANAGER\0CUSTOM HTML\0LANGUAGE\0EXTENDER\0HELP";
  const char* tp = titleTable;
  for (int i = 0; i < active; i++) tp += strlen_P(tp) + 1;
  char buf[16];
  strcpy_P(buf, tp);
  return String(F("<div class='hdr-box'><h1>")) + buf + String(F("</h1></div><div class='grid' style='grid-template-columns:100px 100px'><a class='btn' href='/'>BACK</a></div>"));
}

static String pageFooter() {
  return F("<div class='footer'>OPX-MY-DEMON &bull; developer op aminul ff</div></body></html>");
}

static String buildScanPage(uint8_t lang, bool deauth, bool beacon, bool probe, bool eviltwin,
                            String targetSSID, String capturedList, int capturedCount,
                            String tableRows, String apSSID, String apIP, bool apHidden = false, String multiHtml = "",
                            bool deauthAll = false, bool rogueAP = false, bool preciseDeauth = false, bool trueDeauth = false, bool hijack = false) {
  String h = pageHeader();
  h += "<div class='container'>";
  h += navBar(PAGE_SCAN, lang);

  if (multiHtml.length() > 0) h += multiHtml;

  h += "<div class='sec'>MENU</div>";
  h += "<div class='grid'>";
  h += "<a class='btn' href='/customhtml'>HTML MENU</a>";
  h += "<a class='btn' href='/settings'>SETTINGS</a>";
  h += "<a class='btn' href='/monitor'>MONITOR</a>";
  h += "<a class='btn' href='/files'>FILE MANAGER</a>";
  h += "<a class='btn' href='/?action=hide_ap'>" + String(apHidden ? "SHOW AP" : "HIDE AP") + "</a>";
  h += "<a class='btn' href='/help'>CREDITS</a>";
  h += "<a class='btn' href='/?action=beacon_start'>BEACON SPAM</a>";
  h += "<a class='btn' href='/monitor'>LOGS</a>";
  h += "<a class='btn' href='/?action=reboot'>RESTART</a>";
  h += "</div>";

  h += "<div class='sec'>ACTIONS</div>";
  h += "<div class='grid'>";
  h += "<a class='btn " + String(deauth ? "btn-red" : "") + "' href='/?action=deauth_" + String(deauth ? "stop" : "start") + "'>START DEAUTH</a>";
  h += "<a class='btn " + String(eviltwin ? "btn-red" : "") + "' href='/?action=eviltwin_" + String(eviltwin ? "stop" : "start") + "'>START EVIL-TWIN</a>";
  h += "<a class='btn " + String(rogueAP ? "btn-red" : "") + "' href='/?action=rogue_ap_" + String(rogueAP ? "stop" : "start") + "'>" + String(rogueAP ? "STOP ROGUE AP" : "START ROGUE AP") + "</a>";
  h += "<a class='btn' href='/?action=count_stations'>COUNT STATIONS</a>";
  h += "<a class='btn' href='/?action=capture_handshake'>CAPTURE HANDSHAKE</a>";
  h += "<a class='btn " + String(deauthAll ? "btn-red" : "") + "' href='/?action=deauth_all_" + String(deauthAll ? "stop" : "start") + "'>" + String(deauthAll ? "STOP DDOS" : "DDOS ROUTER") + "</a>";
  h += "<a class='btn " + String(probe ? "btn-red" : "") + "' href='/?action=probe_" + String(probe ? "stop" : "start") + "'>" + String(probe ? "STOP PROBE" : "START PROBE") + "</a>";
  h += "<a class='btn " + String(hijack ? "btn-red" : "") + "' href='/?action=hijack_" + String(hijack ? "stop" : "start") + "'>" + String(hijack ? "STOP HIJACK" : "HIJACK SESSION") + "</a>";
  h += "<a class='btn " + String(preciseDeauth ? "btn-red" : "") + "' href='/?action=precise_deauth_" + String(preciseDeauth ? "stop" : "start") + "'>" + String(preciseDeauth ? "STOP PRECISE" : "PRECISE DEAUTH") + "</a>";
  h += "<a class='btn " + String(trueDeauth ? "btn-red" : "") + "' href='/?action=true_deauth_" + String(trueDeauth ? "stop" : "start") + "'>" + String(trueDeauth ? "STOP TRUE DEAUTH" : "TRUE DEAUTH") + "</a>";
  h += "</div>";

  h += "<div class='sec'>TARGETS</div>";
  h += "<div class='grid' style='grid-template-columns:repeat(3,1fr)'>";
  h += "<a class='btn' href='/'>SCAN</a>";
  h += "<a class='btn' href='/?action=deep_scan'>DEEP SCAN</a>";
  h += "<a class='btn' href='/?action=clear'>CLEAR SELECTION</a>";
  h += "</div>";

  if (targetSSID.length() > 0) {
    h += "<div class='mb-8' style='color:var(--accent)'>SELECTED: " + htmlEntities(targetSSID) + "</div>";
  }

  if (tableRows.length() > 0) {
    h += "<table><thead><tr><th>SSID</th><th>BSSID</th><th>CH</th><th>RSSI</th><th>SEC</th><th>VENDOR</th><th>SELECT</th></tr></thead><tbody>";
    h += tableRows;
    h += "</tbody></table>";
  } else {
    h += "<div class='empty'>SCANNING...</div>";
  }

  if (capturedCount > 0) {
    h += "<div class='sec'>CAPTURED (" + String(capturedCount) + ")</div>";
    h += capturedList;
  }

  h += "</div>";
  h += pageFooter();
  return h;
}

static String buildAttackPage(uint8_t lang, bool deauth, bool beacon, bool probe, bool eviltwin,
                              String targetSSID, uint8_t phishingPage, bool hijack,
                              bool deauthAll, bool preciseDeauth, bool trueDeauth, bool rogueAP,
                              String beaconSSIDHtml, int beaconSSIDCount) {
  String h = pageHeader();
  h += "<div class='container'>";
  h += navBar(PAGE_ATTACK, lang);

  h += "<div class='sec'>BEACON SPAM</div>";
  h += "<div class='grid' style='grid-template-columns:repeat(4,1fr)'>";
  if (beacon) h += "<a class='btn btn-red' href='/?action=beacon_stop'>STOP BEACON SPAM</a>";
  else h += "<a class='btn' href='/?action=beacon_start'>START BEACON SPAM</a>";
  h += "<button class='btn' onclick=\"var s=prompt('Enter SSID:');if(s)window.location='/?action=beacon_add&ssid='+encodeURIComponent(s)\">ADD</button>";
  h += "<a class='btn' href='/?action=beacon_remove'>REMOVE</a>";
  h += "<a class='btn' href='/?action=beacon_randomize'>RANDOMIZE</a>";
  h += "</div>";

  h += "<div style='border:1px solid #222; padding:15px; min-height:100px; margin-bottom:30px'>";
  h += "<div style='color:var(--dim); font-size:10px; margin-bottom:10px'>SSID LIST (" + String(beaconSSIDCount) + ")</div>";
  if (beaconSSIDHtml.length() > 0) {
    h += beaconSSIDHtml;
  } else {
    h += "<div style='color:var(--dim); font-size:11px'>No SSIDs added. Use ADD to add beacon SSIDs.</div>";
  }
  h += "</div>";

  h += "<div class='sec'>ATTACK CONFIG</div>";
  h += "<p style='margin-bottom:10px'>Target: <span style='color:var(--accent)'>" + (targetSSID.length() > 0 ? htmlEntities(targetSSID) : "NONE") + "</span></p>";
  
  h += "<div style='margin-bottom:10px'>Phishing Page:</div>";
  h += "<select onchange=\"window.location='/attack?page='+this.value\" style='margin-bottom:20px'>";
  h += "<option value='0'" + String(phishingPage==0?" selected":"") + ">Facebook</option>";
  h += "<option value='1'" + String(phishingPage==1?" selected":"") + ">Tenda</option>";
  h += "<option value='2'" + String(phishingPage==2?" selected":"") + ">Generic ISP</option>";
  h += "<option value='3'" + String(phishingPage==3?" selected":"") + ">Router Update</option>";
  h += "<option value='4'" + String(phishingPage==4?" selected":"") + ">Maintenance</option>";
  h += "<option value='5'" + String(phishingPage==5?" selected":"") + ">Custom</option>";
  h += "<option value='6'" + String(phishingPage==6?" selected":"") + ">Google</option>";
  h += "<option value='7'" + String(phishingPage==7?" selected":"") + ">Instagram</option>";
  h += "</select>";

  h += "</div>";
  h += pageFooter();
  return h;
}

static String buildMonitorPage(uint8_t lang, unsigned long deauthPkts, unsigned long beaconPkts, unsigned long probePkts, unsigned long totalPkts, String logs) {
  String h = pageHeader();
  h += "<div class='container'>";
  h += navBar(PAGE_MONITOR, lang);

  h += "<div class='sec'>Packet Statistics</div>";
  h += "<table><thead><tr><th>Type</th><th>Count</th></tr></thead><tbody>";
  h += "<tr><td>Deauth</td><td>" + String(deauthPkts) + "</td></tr>";
  h += "<tr><td>Beacon</td><td>" + String(beaconPkts) + "</td></tr>";
  h += "<tr><td>Probe</td><td>" + String(probePkts) + "</td></tr>";
  h += "<tr><td>Total</td><td>" + String(totalPkts) + "</td></tr>";
  h += "</tbody></table>";

  h += "<div class='sec'>Power Config</div>";
  h += "<div class='info-grid'>";
  h += "<div class='info-item'><div class='lbl'>Deauth Burst</div><div class='val'>" + String(DEAUTH_BURST_COUNT) + " pkt</div></div>";
  h += "<div class='info-item'><div class='lbl'>Beacon Burst</div><div class='val'>" + String(BEACON_BURST_COUNT) + " pkt</div></div>";
  h += "<div class='info-item'><div class='lbl'>Probe Burst</div><div class='val'>" + String(PROBE_BURST_COUNT) + " pkt</div></div>";
  h += "<div class='info-item'><div class='lbl'>Channel</div><div class='val'>1-" + String(CHANNEL_MAX) + " (hop)</div></div>";
  h += "</div>";

  h += "<div class='mt-8 mb-8'><a class='btn' href='/monitor'>REFRESH</a></div>";

  h += "<div class='sec'>System Logs</div>";
  if (logs.indexOf("empty") == -1) {
    h += "<div class='log-box'>" + logs + "</div>";
    h += "<div class='mt-8'><a class='btn btn-red' href='/?action=clear_logs'>CLEAR LOGS</a></div>";
  } else {
    h += "<div class='empty'>No Logs</div>";
  }

  h += "</div>";
  h += pageFooter();
  return h;
}

static String buildSettingsPage(uint8_t lang, String version, String board, String mac, int freeHeap, long uptimeSecs,
                                String apSSID, String apPass,
                                bool wifiClientConnected, String wifiClientSSID, bool internetSharing,
                                String wifiScanRows) {
  String h = pageHeader();
  h += "<div class='container'>";
  h += navBar(PAGE_SETTINGS, lang);

  h += "<div class='sec'>THEME</div>";
  h += "<div style='margin-bottom:10px;color:var(--dim)'>Select your theme</div>";
  h += "<div class='grid'>";
  h += "<button class='btn' onclick='setTheme(\"CYBER\")'>CYBER</button>";
  h += "<button class='btn' onclick='setTheme(\"RED\")'>RED</button>";
  h += "<button class='btn' onclick='setTheme(\"TERMINAL\")'>TERMINAL</button>";
  h += "<button class='btn' onclick='setTheme(\"VAPORWAVE\")'>VAPORWAVE</button>";
  h += "</div>";

  h += "<div class='sec'>DISPLAY</div>";
  h += "<div style='margin-bottom:10px;color:var(--dim)'>Display sleep timeout</div>";
  h += "<div class='grid' style='grid-template-columns:repeat(auto-fit,minmax(100px,1fr))'>";
  h += "<a class='btn' href='/?action=display_timeout&val=15'>15 SECONDS</a>";
  h += "<a class='btn' href='/?action=display_timeout&val=30'>30 SECONDS</a>";
  h += "<a class='btn' style='border-color:var(--accent);color:var(--accent)' href='/?action=display_timeout&val=60'>60 SECONDS</a>";
  h += "<a class='btn' href='/?action=display_timeout&val=120'>120 SECONDS</a>";
  h += "<a class='btn' href='/?action=display_timeout&val=0'>NO SLEEP</a>";
  h += "</div>";

  h += "<div class='sec'>SYSTEM HEALTH</div>";
  h += "<div class='info-grid' style='margin-bottom:15px'>";
  h += "<div class='info-item'><div class='lbl'>Free Heap</div><div class='val'>" + String(ESP.getFreeHeap()) + " B</div></div>";
  h += "<div class='info-item'><div class='lbl'>Heap Min</div><div class='val'>" + String(heapLowMark == 0xFFFFFFFF ? ESP.getFreeHeap() : heapLowMark) + " B</div></div>";
  h += "<div class='info-item'><div class='lbl'>Uptime</div><div class='val'>" + String((millis() - startTime) / 1000) + "s</div></div>";
  h += "<div class='info-item'><div class='lbl'>Captured</div><div class='val'>" + String(capturedCount) + "</div></div>";
  h += "</div>";

  h += "<div class='sec'>SECURITY</div>";
  h += "<div style='margin-bottom:10px;color:var(--dim)'>Set PIN to protect sensitive actions</div>";
  h += "<form action='/' method='get' style='margin-bottom:8px'>";
  h += "<input type='hidden' name='action' value='pin_set'>";
  h += "<input type='password' name='val' placeholder='4-8 digit PIN' minlength='4' maxlength='8' style='margin-bottom:8px'>";
  h += "<button class='btn' style='border-color:var(--accent);color:var(--accent)' type='submit'>SET PIN</button>";
  h += "</form>";
  h += "<a class='btn' style='border-color:#f00;color:#f00' href='/?action=pin_clear'>CLEAR PIN</a>";

  h += "<div class='sec'>GENERAL</div>";
  h += "<div style='margin-bottom:10px;color:var(--dim)'>Automatically select multiple SSID from the same Router.</div>";
  h += "<div class='grid' style='grid-template-columns:100px 100px'>";
  h += "<a class='btn' style='border-color:var(--accent);color:var(--accent)' href='/?action=auto_select&val=0'>OFF</a>";
  h += "<a class='btn' href='/?action=auto_select&val=1'>ALL</a>";
  h += "</div>";

  h += "<div class='sec'>ACCESS POINT</div>";
  h += "<form action='/' method='get'>";
  h += "<input type='hidden' name='action' value='save_ap'>";
  h += "<div style='margin-bottom:10px'>SSID:</div><input type='text' name='ssid' value='" + htmlEntities(apSSID) + "' class='mb-8'>";
  h += "<div style='margin-bottom:10px'>PASS:</div><input type='text' name='pass' value='" + htmlEntities(apPass) + "' class='mb-8'>";
  h += "<div class='mt-8 mb-8'><button class='btn' style='border-color:var(--accent);color:var(--accent)' type='submit'>SAVE SETTINGS</button></div>";
  h += "</form>";

  h += "<div class='grid' style='grid-template-columns:1fr 1fr; margin-top:30px'>";
  h += "<a class='btn btn-red' href='/?action=reboot'>RESTART</a>";
  h += "<a class='btn btn-red' href='/?action=reset'>FACTORY RESET</a>";
  h += "</div>";

  h += "<div class='sec' style='margin-top:20px;'>INTERNET FORWARDING (NAT)</div>";
  h += "<p style='font-size:12px;color:var(--dim);margin-bottom:12px;line-height:1.5'>" + String(lang==LANG_INDONESIAN?"Hubungkan ESP8266 ke jaringan WiFi nyata untuk menyediakan internet ke korban Rogue AP.":"Connect ESP8266 to a real WiFi network to provide internet to Rogue AP victims.") + "</p>";

  h += "<div class='info-grid' style='margin-bottom:15px'>";
  if (wifiClientConnected) {
    h += "<div class='info-item'><div class='lbl'>Status</div><div class='val' style='color:#4caf50'>Connected</div></div>";
    h += "<div class='info-item'><div class='lbl'>SSID</div><div class='val'>" + htmlEntities(wifiClientSSID) + "</div></div>";
    h += "<div class='info-item'><div class='lbl'>Sharing</div><div class='val " + String(internetSharing?"green":"red") + "'>" + String(internetSharing?"ACTIVE":"INACTIVE") + "</div></div>";
  } else {
    h += "<div class='info-item'><div class='lbl'>Status</div><div class='val' style='color:#f44336'>Disconnected</div></div>";
  }
  h += "</div>";

  if (wifiScanRows.length() > 0) {
    h += "<div style='margin-bottom:10px;color:var(--dim)'>Scanned Networks:</div>";
    h += "<table><thead><tr><th>SSID</th><th>BSSID</th><th>CH</th><th>RSSI</th><th>SEC</th><th>Vendor</th></tr></thead><tbody>";
    h += wifiScanRows;
    h += "</tbody></table>";
  }

  h += "<div class='flex' style='margin-top:10px'>";
  h += "<a class='btn' href='/?action=wifi_scan'>📶 SCAN NETWORKS</a>";
  if (wifiClientConnected) {
    h += "<a class='btn btn-red' href='/?action=wifi_disconnect'>⏹ DISCONNECT</a>";
    if (internetSharing) {
      h += "<a class='btn btn-red' href='/?action=wifi_sharing_stop'>⏹ DISABLE SHARING</a>";
    } else {
      h += "<a class='btn' style='border-color:var(--accent);color:var(--accent)' href='/?action=wifi_sharing_start'>▶ ENABLE SHARING</a>";
    }
  }
  h += "</div>";

  h += "<form action='/' method='get' class='mt-12' style='display:" + String(wifiClientConnected?"none":"block") + "'>";
  h += "<input type='hidden' name='action' value='wifi_connect'>";
  h += "<div style='margin-bottom:10px'>SSID:</div><input type='text' name='wifi_ssid' placeholder='Target WiFi SSID' required>";
  h += "<div style='margin-bottom:10px;margin-top:10px'>PASS:</div><input type='password' name='wifi_pass' placeholder='WiFi password' required>";
  h += "<button class='btn' style='border-color:var(--accent);color:var(--accent);margin-top:10px' type='submit'>▶ CONNECT</button>";
  h += "</form>";

  h += "</div>";
  h += pageFooter();
  return h;
}

static String buildFilesPage(uint8_t lang, String fileList, bool hasCustom) {
  String h = pageHeader();
  h += "<div class='container'>";
  h += navBar(PAGE_FILES, lang);

  // Storage Bar Calculation
  FSInfo fs_info;
  LittleFS.info(fs_info);
  float totalBytes = fs_info.totalBytes;
  float usedBytes = fs_info.usedBytes;
  float freeBytes = totalBytes - usedBytes;
  
  // Convert to KB
  String freeStr = String(freeBytes / 1024.0, 2);
  String totalStr = String(totalBytes / 1024.0, 2);
  float usagePct = totalBytes > 0 ? (usedBytes / totalBytes) * 100.0 : 0;
  
  h += "<div style='border:1px solid var(--accent); padding:10px 15px; margin-bottom:20px; display:flex; justify-content:space-between; align-items:center;'>";
  h += "<div style='width:" + String(usagePct) + "%; height:20px; background:var(--accent); min-width:20px;'></div>";
  h += "<div style='font-size:11px; font-weight:bold; color:var(--text); padding-left:10px;'>Free: " + freeStr + " KB / Total: " + totalStr + " KB</div>";
  h += "</div>";

  // Upload Box
  h += "<div style='border:1px dashed var(--dim); padding:15px; margin-bottom:20px'>";
  h += "<form action='/upload' method='post' enctype='multipart/form-data'>";
  h += "<div class='grid' style='grid-template-columns:1fr; gap:10px;'>";
  h += "<div style='display:flex; border:1px solid #333; background:transparent;'>";
  h += "<div style='position:relative; width:120px; border-right:1px solid #333;'><input type='file' name='file' required style='opacity:0; position:absolute; left:0; top:0; width:100%; height:100%; cursor:pointer;' onchange=\"document.getElementById('file-label').innerText = this.files[0].name;\"><button type='button' class='btn' style='border:none; width:100%; font-weight:bold;'>CHOOSE FILES</button></div>";
  h += "<div id='file-label' style='padding:10px; color:var(--dim); font-size:11px; flex-grow:1; display:flex; align-items:center;'>No file chosen</div>";
  h += "</div>";
  h += "<button class='btn' type='submit' style='background:var(--accent); color:#000; border:none; font-weight:bold; text-shadow:none;'>UPLOAD SELECTED FILES</button>";
  h += "</div>";
  h += "<div style='font-size:10px; color:var(--text); margin-top:15px;'>* Use web browser to upload, do not use Captive Portal.</div>";
  h += "</form></div>";

  // Actions Section
  h += "<div class='sec'>ACTIONS</div>";
  h += "<div id='action-placeholder' style='border:1px dashed var(--dim); padding:15px; text-align:center; color:var(--dim); font-size:11px; margin-bottom:20px;'>Select files to see actions</div>";

  if (fileList.length() > 0) {
    h += "<table><thead><tr style='border-bottom:2px solid var(--accent);'><th style='color:var(--accent); background:transparent;'>NAME</th><th style='color:var(--accent); background:transparent;'>SIZE</th><th style='color:var(--accent); background:transparent; text-align:center;'>SELECTION</th></tr></thead><tbody>";
    h += fileList;
    h += "</tbody></table>";
  } else {
    h += "<div class='empty'>NO FILES</div>";
  }

  // Dynamic Actions Container
  h += "<div id='file-actions-container'></div>";

  h += "<script>";
  h += "let selectedFile = null;";
  h += "function selectFile(fn) {";
  h += "  selectedFile = fn;";
  h += "  document.getElementById('action-placeholder').style.display = 'none';";
  h += "  let container = document.getElementById('file-actions-container');";
  h += "  container.innerHTML = '<div class=\"grid\" style=\"grid-template-columns:repeat(1,1fr); gap:5px; margin-bottom:20px;\">' +";
  h += "    '<button class=\"btn\" style=\"border-color:var(--accent); color:var(--accent);\" onclick=\"performAction(\\'download\\')\">DOWNLOAD</button>' +";
  h += "    '<button class=\"btn\" onclick=\"var n=prompt(\\'New name:\\');if(n)window.location=\\'/?action=rename&file=\\'+encodeURIComponent(selectedFile)+\'&name=\\'+encodeURIComponent(n)\">RENAME</button>' +";
  h += "    '<button class=\"btn btn-red\" style=\"background:#cc0000; color:#fff; border:none;\" onclick=\"performAction(\\'delete\\')\">DELETE</button>' +";
  h += "  '</div>';";
  h += "}";
  h += "function performAction(act) {";
  h += "  if(!selectedFile) return;";
  h += "  if(act === 'delete') {";
  h += "    if(confirm('Delete ' + selectedFile + '?')) window.location = '/?delete=' + selectedFile;";
  h += "  } else if(act === 'download') {";
  h += "    window.location = '/' + selectedFile;";
  h += "  }";
  h += "}";
  h += "</script>";

  h += "</div>";
  h += pageFooter();
  return h;
}

static String buildCustomHtmlPage(uint8_t lang, bool hasCustom, uint8_t phishingPage) {
  String h = pageHeader();
  h += "<div class='container'>";
  h += navBar(PAGE_CUSTOMHTML, lang);

  h += "<div class='grid' style='grid-template-columns:100px 100px; margin-bottom:20px;'>";
  h += "<button id='html-menu-back' class='btn'>BACK</button>";
  h += "</div>";

  h += "<div id='html-menu-container' class='grid' style='grid-template-columns:repeat(auto-fit, 200px); gap:20px; align-items:start;'></div>";

  h += "<script>";
  h += "let customFiles = [";
  h += "  { id: '__default_et__', name: 'DEFAULT EVIL-TWIN', isDefault: true, pageVal: 2 },";
  h += "  { id: '__default_ra__', name: 'DEFAULT ROGUE AP', isDefault: true, pageVal: 4 },";
  
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    String fn = dir.fileName();
    if (fn.endsWith(".html")) {
      String cleanFn = fn;
      if (cleanFn.startsWith("/")) cleanFn = cleanFn.substring(1);
      h += "{ id: '" + cleanFn + "', name: '" + cleanFn + "', previewUrl: '/" + cleanFn + "' },";
    }
  }
  
  h += "];";
  h += "let selectedHtmlFile = null;";
  h += "function renderHtmlMenu() {";
  h += "  const container = document.getElementById('html-menu-container');";
  h += "  if(!container) return;";
  h += "  const backBtn = document.getElementById('html-menu-back');";
  h += "  if (!selectedHtmlFile) {";
  h += "    backBtn.onclick = () => window.location='/';";
  h += "    let html = '';";
  h += "    if (customFiles.length === 0) {";
  h += "      html = '<div class=\"empty\" style=\"grid-column: 1 / -1;\">NO HTML FILES UPLOADED</div>';";
  h += "    } else {";
  h += "      customFiles.forEach(file => {";
  h += "        if (file.isDefault) {";
  h += "          html += '<div style=\"border:1px solid var(--accent); display:flex; flex-direction:column; background:#0a0a0a; cursor:pointer;\" onclick=\"selectHtml(\\''+file.id+'\\')\">' +";
  h += "            '<div style=\"height:250px; display:flex; align-items:center; justify-content:center; background:#050505; flex-direction:column; gap:15px;\">' +";
  h += "            '<div style=\"font-size:48px; color:var(--accent); opacity:0.6\">&#9783;</div>' +";
  h += "            '<div style=\"font-size:11px; color:var(--dim); text-align:center; padding:0 10px;\">Built-in Template</div>' +";
  h += "            '</div><button class=\"btn\" style=\"border:none; border-top:1px solid var(--accent); width:100%; pointer-events:none;\">'+file.name+'</button></div>';";
  h += "        } else {";
  h += "          html += '<div style=\"border:1px solid var(--dim); display:flex; flex-direction:column; background:#111; cursor:pointer;\" onclick=\"selectHtml(\\''+file.id+'\\')\">' +";
  h += "                  '<div style=\"height:250px; background:#fff; position:relative; overflow:hidden;\">' +";
  h += "                  '<iframe src=\"'+file.previewUrl+'\" style=\"position:absolute; top:0; left:0; width:400%; height:400%; transform:scale(0.25); transform-origin:0 0; border:none; pointer-events:none; background:#fff;\"></iframe>' +";
  h += "                  '</div><button class=\"btn\" style=\"border:none; border-top:1px solid var(--dim); width:100%; pointer-events:none;\">'+file.name+'</button></div>';";
  h += "        }";
  h += "      });";
  h += "    }";
  h += "    container.innerHTML = html;";
  h += "  } else {";
  h += "    backBtn.onclick = () => { selectedHtmlFile = null; renderHtmlMenu(); };";
  h += "    let file = customFiles.find(f => f.id === selectedHtmlFile);";
  h += "    if (file.isDefault) {";
  h += "      container.innerHTML = '<div style=\"border:1px solid var(--accent); display:flex; flex-direction:column; background:#0a0a0a; max-width:200px;\">' +";
  h += "        '<div style=\"height:250px; display:flex; align-items:center; justify-content:center; background:#050505; flex-direction:column; gap:15px;\">' +";
  h += "        '<div style=\"font-size:48px; color:var(--accent); opacity:0.6\">&#9783;</div>' +";
  h += "        '<div style=\"font-size:11px; color:var(--dim); text-align:center; padding:0 10px;\">Built-in Template</div>' +";
  h += "        '</div><button class=\"btn\" style=\"border:none; border-top:1px solid var(--accent); width:100%; pointer-events:none;\">'+file.name+'</button></div>' +";
  h += "        '<div style=\"border:1px solid var(--dim); padding:15px; background:#050505; height:100%; min-width:200px;\">' +";
  h += "        '<div style=\"color:var(--text); margin-bottom:20px; text-align:center; font-size:10px; font-weight:bold;\">USE AS :</div>' +";
  h += "        '<div class=\"grid\" style=\"grid-template-columns:1fr; gap:10px;\">' +";
  h += "        '<button class=\"btn\" style=\"border-color:var(--accent); color:var(--accent);\" onclick=\"window.location=\\'/?page=\\'+file.pageVal+\'&action=eviltwin_start\\'\">EVIL-TWIN</button>' +";
  h += "        '<button class=\"btn\" style=\"border-color:var(--accent); color:var(--accent); box-shadow:0 0 8px var(--accent); background:rgba(0,255,255,0.1);\" onclick=\"window.location=\\'/?page=\\'+file.pageVal+\'&action=rogue_ap_start\\'\">ROGUE AP</button>' +";
  h += "        '</div></div></div>';";
  h += "    } else {";
  h += "      container.innerHTML = '<div style=\"border:1px solid var(--dim); display:flex; flex-direction:column; background:#111; max-width:200px;\">' +";
  h += "        '<div style=\"height:250px; background:#fff; position:relative; overflow:hidden;\">' +";
  h += "        '<iframe src=\"'+file.previewUrl+'\" style=\"position:absolute; top:0; left:0; width:400%; height:400%; transform:scale(0.25); transform-origin:0 0; border:none; pointer-events:none; background:#fff;\"></iframe>' +";
  h += "        '</div><button class=\"btn\" style=\"border:none; border-top:1px solid var(--dim); width:100%; pointer-events:none;\">'+file.name+'</button></div>' +";
  h += "        '<div style=\"border:1px solid var(--dim); padding:15px; background:#050505; height:100%; min-width:200px;\">' +";
  h += "        '<div style=\"color:var(--text); margin-bottom:20px; text-align:center; font-size:10px; font-weight:bold;\">USE HTML AS :</div>' +";
  h += "        '<div class=\"grid\" style=\"grid-template-columns:1fr; gap:10px;\">' +";
  h += "        '<button class=\"btn\" style=\"border-color:var(--accent); color:var(--accent);\" onclick=\"window.location=\\'/?action=use_custom_file&file=\\'+file.id; setTimeout(()=>window.location=\\'/?action=eviltwin_start\\',500);\">EVIL-TWIN</button>' +";
  h += "        '<button class=\"btn\" style=\"border-color:var(--accent); color:var(--accent); box-shadow:0 0 8px var(--accent); background:rgba(0,255,255,0.1);\" onclick=\"window.location=\\'/?action=use_custom_file&file=\\'+file.id; setTimeout(()=>window.location=\\'/?action=rogue_ap_start\\',500);\">ROGUE AP</button>' +";
  h += "        '</div><div class=\"grid\" style=\"grid-template-columns:1fr; margin-top:20px;\">' +";
  h += "        '<button class=\"btn\" style=\"border-color:var(--dim);\" onclick=\"window.location=\\'/edit?file=\\'+file.id\">EDIT</button>' +";
  h += "        '</div>' +";
  h += "        '<div style=\"margin-top:20px; border-top:1px solid #222; padding-top:10px\">' +";
  h += "        '<form action=\"/upload\" method=\"post\" enctype=\"multipart/form-data\">' +";
  h += "        '<input type=\"file\" name=\"file\" required style=\"margin-bottom:10px\">' +";
  h += "        '<button class=\"btn\" type=\"submit\" style=\"width:100%\">UPLOAD HTML</button>' +";
  h += "        '</form></div></div>';";
  h += "    }";
  h += "  }";
  h += "}";
  h += "function selectHtml(id) {";
  h += "  selectedHtmlFile = id;";
  h += "  renderHtmlMenu();";
  h += "}";
  h += "window.addEventListener('DOMContentLoaded', renderHtmlMenu);";
  h += "</script>";

  h += "</div>";
  h += pageFooter();
  return h;
}

static String buildLanguagePage(uint8_t lang, String langTableRows) {
  String h = pageHeader();
  h += "<div class='container'>";
  h += navBar(PAGE_LANGUAGE, lang);
  h += "<div class='sec'>Language Editor</div>";
  h += "<div class='grid'><a class='btn' href='/?lang=0'>ENGLISH</a><a class='btn' href='/?lang=1'>INDONESIAN</a></div>";
  if (langTableRows.length() > 0) {
    h += "<table><thead><tr><th>KEY</th><th>EN</th><th>ID</th></tr></thead><tbody>";
    h += langTableRows;
    h += "</tbody></table>";
  }
  h += "</div>";
  h += pageFooter();
  return h;
}

static String buildExtenderPage(uint8_t lang) {
  String h = pageHeader();
  h += "<div class='container'>";
  h += navBar(PAGE_EXTENDER, lang);
  h += "<div class='sec'>Extender Mode</div>";
  h += "<a class='btn' href='/?action=extender_scan'>SCAN FOR AP</a>";
  h += "</div>";
  h += pageFooter();
  return h;
}

static String buildPinPage(uint8_t lang, int attempts, bool lockedOut) {
  String h = pageHeader();
  h += "<div class='container'>";
  h += "<div class='hdr-box'><h1>PIN REQUIRED</h1></div>";
  if (lockedOut) {
    h += "<div class='sec' style='color:#f00'>LOCKED OUT (30s)</div>";
    h += "<p style='color:var(--dim)'>Too many failed attempts. Wait 30 seconds.</p>";
  } else if (attempts > 0) {
    h += "<div class='sec' style='color:#ff0'>ATTEMPT " + String(attempts) + "/" + String(PIN_MAX_ATTEMPTS) + "</div>";
  } else {
    h += "<div class='sec'>ENTER PIN</div>";
  }
  h += "<form action='/' method='get'>";
  h += "<input type='hidden' name='action' value='pin_verify'>";
  h += "<input type='password' name='val' placeholder='PIN' required style='margin-bottom:10px'" + String(lockedOut ? " disabled" : "") + ">";
  h += "<button class='btn' style='border-color:var(--accent);color:var(--accent)' type='submit'" + String(lockedOut ? " disabled" : "") + ">UNLOCK</button>";
  h += "</form></div>";
  h += pageFooter();
  return h;
}

static String buildHelpPage(uint8_t lang) {
  String h = pageHeader();
  h += "<div class='container'>";
  h += navBar(PAGE_HELP, lang);
  h += "<div class='sec'>HELP & CREDITS</div>";
  h += "<div style='color:var(--dim)'>Educational tool based on ESP8266 Deauther.</div>";
  h += "</div>";
  h += pageFooter();
  return h;
}

static String buildEditPage(uint8_t lang, String filename, String content) {
  String h = pageHeader();
  h += "<div class='container'>";
  h += "<div class='hdr-box'><h1>EDIT: " + filename + "</h1></div>";
  h += "<form action='/' method='post' enctype='application/x-www-form-urlencoded'>";
  h += "<input type='hidden' name='action' value='save_file'>";
  h += "<input type='hidden' name='file' value='" + filename + "'>";
  h += "<textarea name='content' style='width:100%;height:450px;background:#000;color:var(--accent);border:1px solid var(--dim);font-family:monospace;font-size:12px;padding:10px;margin-bottom:10px;'>" + content + "</textarea>";
  h += "<div class='flex'>";
  h += "<button class='btn' type='submit' style='border-color:var(--accent);color:var(--accent);'>SAVE</button>";
  h += "<a class='btn' href='/customhtml'>BACK</a>";
  h += "</div>";
  h += "</form>";
  h += "</div>";
  h += pageFooter();
  return h;
}

#endif
