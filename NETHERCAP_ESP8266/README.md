# NETHERCAP ESP8266 v4.2.0

Complete ESP8266 firmware with Evil Twin, Deauther, Beacon, Probe, Session Hijack attacks + Web UI.

## Pages
- **Scan** - Network scanner, target selection, attack status
- **Attack** - Deauth / Beacon / Probe / EvilTwin / Session Hijack controls + phishing page selector
- **Monitor** - Packet counter
- **Settings** - Device info, AP settings, dynamic button pin config, reboot/reset/format
- **Files** - SPIFFS file manager (upload/delete)
- **Custom HTML** - Upload custom phishing page
- **Language** - English / Indonesian language editor (default: Bahasa Indonesia)
- **Extender** - WiFi extender mode

## Features (v4.2.0)
- **Session Hijack** - Make ESP8266 pretend as legit clients while real clients get disconnected
- **Dynamic Pin Assignment** - Map physical button pins in Settings (no hardcoded schematic)
- **Dynamic OLED Detection** - Auto-detect SSD1306/SH1106 OLED, configurable in Settings
- **Extender** - WiFi repeater functionality
- **Virtual Display** - Smoother virtual display rendering
- **Enhanced Language Support** - Full English & Bahasa Indonesia support

## Build
1. Arduino IDE → Board: NodeMCU 1.0 (ESP-12E Module)
2. Install ESP8266 board support (2.7+)
3. Enable `wifi_send_pkt_freedom` (see Spacehuhn docs)
4. Open `NETHERCAP_ESP8266.ino` → Upload

## Usage
- AP: `NETHERCAP` / Pass: `deauther`
- Web UI: `http://8.8.8.8`
- Select target → Start attacks
