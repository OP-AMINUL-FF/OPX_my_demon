# OPX-MY-DEMON v1.0.2

A full-featured Wi-Fi penetration testing and social engineering firmware for **ESP8266** (NodeMCU). Built on top of heavily-modified M1z23R's ESP8266-EvilTwin v2 with Spacehuhn's Deauther framework.

> **Developer:** OP AMINUL FF

---

## What's New in v1.0.2

### Aggressive RAM Optimization
- **PROGMEM migration** — All language strings, action tables, HTML templates, and web UI strings moved to flash (PROGMEM/F() macros)
- **Buffer shrink** — Network/station/probe/log buffers reduced to fit within 80KB DRAM limit
- **Removed `customHTML` cache** — Custom phishing pages read from LittleFS on demand instead of held in RAM
- **EAPOL raw data clipped** — From 256 → 64 bytes per handshake entry
- **Dead code removal** — Unused variables and functions stripped (`computeSHA256`, `verifyOTASignature`, `sanitizeHexString`, `lastChannelHop`, `phishingSessionId`, `wifiClientStatus`, `extenderTargetSSID/pass`)

### IRAM Optimization
- **MMU=4816** — 16KB ICACHE + 48KB IRAM (vs default 32/32), yields **66% IRAM usage** (was **91%**)
- **Core patches** — 11 `IRAM_ATTR` removed from `waveform_pwm.cpp`, 1 from `gdb_hooks.cpp` (`demon_dev.bat` auto-applies)

### Bug Fixes
- **Atomic file writes** — `/state.json` writes use `.tmp` + rename pattern to prevent corruption
- **Upload handler** — Fixed `w` vs `a` mode for file uploads; sanitized path handling
- **State.json upload** — Uses atomic `.tmp` pattern + explicit rename
- **Action string table** — Moved from `String*` array (RAM) to `PROGMEM` compact table (flash)
- **hide_ap** — Properly uses `WiFi.softAP(ssid, pass, ch, hidden)` API
- **BSS Transition (802.11v)** — Uses broadcast DA instead of per-client, correct action frame body
- **EAPOL parsing** — Fixed QoS bitmask and +HTC handling
- **Log buffer** — Reduced from `LOG_BUFFER_MAX=8192` to `2048`
- **`delay(100)` before `ESP.restart()`** — Ensures HTTP response is sent before reboot
- **`WiFi.mode(WIFI_AP_STA)`** — Moved before `loadState()` to ensure proper radio init

### v1.0.2 Changelog
- `config.h`: v1.0.1→v1.0.2, buffer limits reduced, EAPOL_MAX added
- `OPX-MY-DEMON_ESP8266.ino`: PROGMEM tables, removed `customHTML` RAM cache, atomic saves, `wifiClientStatus` removed, upload path sanitized, `hide_ap` API fix, `delay(100)` before reboot
- `webui.h`: All HTML template strings via `F()` macro, navBar titles via PROGMEM
- `attacks.h`: Dead code removed, BSS Transition broadcast fix, EAPOL parsing fix, DHCP fingerprint buffers shrunk, `phishingSessionId` removed
- `language.h`: `langTable` moved to PROGMEM, `tr()` reads via `pgm_read_ptr`
- `secure_ota.h`: Unused functions removed
- New: `demon_dev.bat`, `patches/` (core IRAM_ATTR patches), `AGENTS.md`

---

## Features

### Core Attacks
| Attack | Description |
|--------|-------------|
| **Deauth** | 802.11 deauthentication with PMF bypass (CSA + BSS Transition) |
| **Evil-Twin** | Rogue AP with captive portal, auto-phishing page selection |
| **Beacon Spam** | Custom SSID beacon flooding |
| **Probe Request** | Probe request flooding |
| **Rogue AP** | Fake free WiFi AP with optional internet sharing |
| **Session Hijack** | Impersonate legitimate clients |
| **DDoS Router** | Mass deauth on all connected clients |
| **Precise Deauth** | Targeted client deauthentication |
| **True Deauth** | Aggressive deauth mode |

### Security & Hardening (2026 Upgrades)
- **Constant-Time PIN Verification** — Timing-attack resistant PIN comparison
- **Reactive Phishing** — Auto-verifies captured passwords via WiFi.begin(), shows success/retry pages
- **XXTEA Encryption** — All stored passwords and PINs encrypted with device-unique key
- **Input Sanitization** — XSS-resistant HTML entity encoding
- **Atomic File Writes** — .tmp + rename pattern prevents filesystem corruption on power loss
- **Rate-Limited Capture** — Anti-bruteforce credential capture (800ms interval, 50/min max)
- **PIN Lockout** — Auto-lockout after 5 failed attempts (30s)
- **Adaptive TX Power** — Signal strength matching for stealth
- **Heap Monitoring** — Dynamic resource limits prevent crashes
- **Wear-Leveling State Saves** — Critical saves every 5s, non-critical every 60s

### Phishing Templates (9 Built-in)
| Template | Type |
|----------|------|
| Facebook | Social media login |
| Google | Google sign-in (email + password) |
| Instagram | Instagram login (username + password) |
| Tenda | Router login |
| Generic ISP | "Connection lost, re-enter WiFi password" |
| Router Update | Firmware upgrade form with terms |
| Landing | "500 Internal Server Error" |
| Custom HTML | Upload your own HTML pages |

### Additional Features
- **Web UI** — Full terminal-style dark web interface (4 themes: CYBER, TERMINAL, RED, VAPORWAVE)
- **Internet Sharing (NAT)** — Forward internet from STA to AP interface
- **Extender Mode** — WiFi repeater functionality
- **Handshake Capture** — WPA/WPA2 EAPOL handshake logging to PCAP
- **DHCP Fingerprinting** — Client OS detection via DHCP option 55
- **DoH Mitigation** — DNS-over-HTTPS canary domain blocking
- **WebSocket Real-time Status** — Live attack statistics
- **Secure OTA** — HMAC-SHA256 firmware signature verification
- **Multi-language** — English & Indonesian (custom language support)
- **Persistent State** — All settings saved to LittleFS `/state.json`

---

## Hardware Requirements

| Component | Specification |
|-----------|--------------|
| **Board** | NodeMCU 1.0 (ESP-12E Module) or any ESP8266 with 4MB flash |
| **Flash Size** | 4MB (32Mbit) required for LittleFS |
| **Board Support** | ESP8266 Arduino Core 2.7+ |

---

## Installation Guide

### Method 1: Arduino IDE

#### Step 1: Install ESP8266 Board Support
1. Open **Arduino IDE** → File → Preferences
2. Add to **Additional Boards Manager URLs**:
   ```
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Tools → Board → Boards Manager → Search "ESP8266" → Install **esp8266 2.7.0+**

#### Step 2: Install Required Libraries
Install via Library Manager (Sketch → Include Library → Manage Libraries):
- **ESPAsyncWebServer** (by me-no-dev)
- **ESPAsyncTCP** (by me-no-dev)
- **DNSServer** (included with ESP8266 core)
- **LittleFS** (included with ESP8266 core)
- **ArduinoJson** (by Benoit Blanchon, v6.x)

#### Step 3: Compile & Upload
1. Open `OPX-MY-DEMON_ESP8266/OPX-MY-DEMON_ESP8266.ino` in Arduino IDE
2. Select Board: **Tools → Board → ESP8266 Boards → NodeMCU 1.0 (ESP-12E Module)**
3. Flash Size: **Tools → Flash Size → 4MB (FS:3MB OTA:~0.5MB)** (or `4M3M` for max storage)
4. CPU Frequency: **160 MHz**
5. Upload Speed: **115200**
6. Click **→** (Upload) button

#### Step 4: Upload LittleFS Data (for state storage)
1. Install **ESP8266 LittleFS Data Upload** plugin (or use `arduino-cli`)
2. Tools → ESP8266 LittleFS Data Upload → Upload

### Method 2: One-Click Build (Recommended)

Double-click `demon_dev.bat` or run in terminal:

```cmd
.\demon_dev.bat
```

**What it does automatically:**
1. Checks if `arduino-cli` is installed — if missing, auto-installs via winget
2. Checks/installs ESP8266 core (arduino-cli core install)
3. Checks/installs required libraries (ESPAsyncWebServer, ESPAsyncTCP, ArduinoJson)
4. Applies IRAM-optimizing core patches
5. Shows interactive menu with all options

**Menu Options:**
| Option | Action |
|--------|--------|
| `[1]` | Compile Firmware |
| `[2]` | Compile + Flash (auto-detect port) |
| `[3]` | Compile + Flash + Serial Monitor |
| `[4]` | Flash Existing Binary Only |
| `[5]` | Open Serial Monitor |
| `[6]` | List Available COM Ports |
| `[7]` | Restore Original Core Files |
| `[8]` | Full Clean Build |
| `[9]` | Upload LittleFS Data |
| `[0]` | Exit |

### Method 3: PlatformIO

#### Step 1: Install PlatformIO
- VS Code Extension: Install **PlatformIO IDE**
- Or CLI: `pip install platformio`

#### Step 2: Create `platformio.ini`
```ini
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
board_build.flash_mode = dout
board_build.f_cpu = 160000000L
board_build.max_size = 4194304
board_build.board_build.mmu = 4816
lib_deps =
    me-no-dev/ESPAsyncWebServer
    me-no-dev/ESPAsyncTCP
    bblanchon/ArduinoJson @ ^6.0.0
```

#### Step 3: Build & Upload
```bash
pio run --target upload
```

---

## Flashing Guide (Pre-built Binary)

Download `OPX-MY-DEMON_ESP8266_v1.0.2.bin` from [Releases](https://github.com/OP-AMINUL-FF/OPX_my_demon/releases).

> **Firmware:** Built with `4M3M` flash layout (4MB flash → ~3MB for LittleFS) and `mmu=4816` (16KB ICACHE + 48KB IRAM).

### Using esptool.py (Recommended)
```bash
# Flash firmware (replace COM3 with your port)
esptool.py --port COM3 --baud 115200 write_flash \
  --flash_mode dout --flash_size 4MB \
  0x00000 OPX-MY-DEMON_ESP8266_v1.0.2.bin
```

### Using ESP8266 Flash Download Tool
1. Open **ESP Flash Download Tool (ESP8266)**
2. Configure: SPI Speed **40MHz**, SPI Mode **DOUT**, Flash Size **32Mbit (4MB)**
3. Address `0x00000` → select `OPX-MY-DEMON_ESP8266_v1.0.2.bin`
4. Press **START** (connect GPIO0 to GND, power cycle if needed)

### Verify Flash (Optional)
```bash
esptool.py --port COM3 flash_id
# Expected: Manufacturer: ef, Device: 4016 (for 4MB Winbond flash)

---

## Usage Guide

### Default Access
| Parameter | Value |
|-----------|-------|
| **SSID** | `OPX-MY-DEMON` (configurable) |
| **Password** | `deauther` (configurable) |
| **Web UI** | `http://8.8.8.8` |
| **Serial** | 115200 baud |

### Quick Start
1. Power on the ESP8266 — it creates a WiFi AP
2. Connect to the AP using the default credentials
3. Open browser → navigate to `http://8.8.8.8`
4. Click **SCAN** to discover nearby networks
5. Select a target → choose attack type

### Web UI Pages
| Page | Function |
|------|----------|
| **SCAN** | Network scanner, target selection, attack controls |
| **ATTACK** | Beacon spam config, phishing page selector |
| **MONITOR** | Packet statistics, system logs |
| **SETTINGS** | Device info, PIN config, WiFi client, themes |
| **FILE MANAGER** | LittleFS file browser, upload/download |
| **CUSTOM HTML** | Upload & select custom phishing pages |
| **LANGUAGE** | Language editor (EN/ID) |
| **EXTENDER** | WiFi extender mode |
| **HELP** | Credits |

### PIN Protection
1. Go to Settings → **SET PIN** (4-8 digit)
2. Sensitive actions (deauth, attacks, reboot) require PIN
3. 5 failed attempts → 30s lockout
4. PIN stored encrypted in state.json

### Custom Phishing Pages
1. Upload `.html` files via **FILE MANAGER** or **CUSTOM HTML**
2. Go to **CUSTOM HTML** → click on uploaded file
3. Choose **EVIL-TWIN** or **ROGUE AP** to use it
4. Selection persists across reboots

---

## File Structure

```
OPX-MY-DEMON_ESP8266/            — Firmware project root
├── OPX-MY-DEMON_ESP8266.ino     — Main firmware (setup, loop, HTTP handlers)
├── config.h                 — Constants, pin mappings, feature flags
├── attacks.h                — Core engine: packet injection, scanning, encryption
├── phishing.h               — 9 built-in HTML phishing templates (PROGMEM)
├── webui.h                  — Web UI page builders with inline CSS/JS (4 themes)
├── websockets.h             — WebSocket real-time broadcast
├── secure_ota.h             — HMAC-SHA256 firmware verification
├── language.h               — EN/ID translation table (160 entries, PROGMEM)
├── forensic_yara.yar        — YARA rules for memory forensics
└── patches/                 — IRAM-optimized ESP8266 core patches
    ├── core_esp8266_waveform_pwm.cpp
    ├── gdb_hooks.cpp
    └── backup/              — Original core files (auto-backed up on first build)
```

## Persistence (state.json)

All settings auto-save to `/state.json` on LittleFS:
- Attack states (deauth, beacon, probe, etc.)
- WiFi client credentials (encrypted)
- PIN (encrypted with XXTEA)
- Selected phishing page
- Language preference
- Attack timer configuration

On boot, the firmware restores the last known state.

---

## Technical Notes

- **Packet Injection**: Uses `wifi_send_pkt_freedom()` from ESP8266 SDK
- **Promiscuous Mode**: Captures EAPOL handshakes and probe requests
- **Encryption**: XXTEA with device-unique key (MAC + Chip ID + Flash ID)
- **IRAM Usage**: 43727/65536 bytes (**66%**) — 25% improvement over default (91%) via `mmu=4816`
- **DRAM Usage**: 57304/80192 bytes (**71%**) — under the 80KB limit for stable operation
- **File System**: LittleFS (4MB flash → ~3MB for filesystem via `4M3M` layout)
- **Web Server**: Async (non-blocking), handles multiple clients
- **DNS**: Captive portal via custom DNS server (resolves all to 8.8.8.8)

---

## Credits

- **M1z23R** — ESP8266-EvilTwin v2 (base framework)
- **Spacehuhn** — Deauther project (packet injection reference)
- **OP AMINUL FF** — OPX-MY-DEMON modifications & 2026 upgrades

---

## License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

---

## Disclaimer

This firmware is intended for **educational purposes only**. Use only on networks you own or have explicit permission to test. The developers are not responsible for any misuse or illegal activities.
