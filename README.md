# OPX-MY-DEMON

> **Version 1.0.2** — A full-featured Wi-Fi penetration testing and social engineering firmware for ESP8266 (NodeMCU), built on heavily-modified M1z23R's ESP8266-EvilTwin v2 with Spacehuhn's Deauther framework.

<p align="center">
  <a href="https://opaminulff.netlify.app/" target="_blank">
    <img src="https://raw.githubusercontent.com/OP-AMINUL-FF/OPX_my_demon/master/channels4_profile.jpg" alt="OPX-MY-DEMON" width="120" style="border-radius:16px">
  </a>
</p>

<p align="center">
  <a href="https://opaminulff.netlify.app/"><strong>Visit Official Website &rarr;</strong></a>
</p>

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Web Flasher](#web-flasher)
- [Project Structure](#project-structure)
- [Technical Notes](#technical-notes)
- [Changelog](#changelog)
- [Credits](#credits)
- [License](#license)
- [Disclaimer](#disclaimer)

---

## Overview

OPX-MY-DEMON is an advanced ESP8266 firmware designed for wireless security auditing and social engineering demonstrations. It packs deauthentication attacks, rogue access points, phishing captures, beacon flooding, and dozens more offensive and defensive tools into a single device that fits in your pocket.

**Developer:** OP AMINUL FF  
**Website:** [https://opaminulff.netlify.app/](https://opaminulff.netlify.app/)  
**Repository:** [github.com/OP-AMINUL-FF/OPX_my_demon](https://github.com/OP-AMINUL-FF/OPX_my_demon)

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

### Security & Hardening

- Constant-time PIN verification (timing-attack resistant)
- Reactive phishing with auto-credential verification via WiFi.begin()
- XXTEA encryption for all stored passwords and PINs
- XSS-resistant HTML entity encoding
- Atomic file writes with .tmp + rename pattern
- Rate-limited credential capture (800ms interval, 50/min max)
- PIN lockout after 5 failed attempts (30s)
- Adaptive TX power for stealth operation
- Heap monitoring with dynamic resource limits
- Wear-leveling state saves

### Phishing Templates (9 Built-in)

| Template | Type |
|----------|------|
| Facebook | Social media login |
| Google | Google sign-in |
| Instagram | Instagram login |
| Tenda | Router login |
| Generic ISP | WiFi password re-entry |
| Router Update | Firmware upgrade form |
| Landing | 500 Internal Server Error |
| Custom HTML | Upload your own pages |

### Additional Features

- Dark web UI with 4 themes (CYBER, TERMINAL, RED, VAPORWAVE)
- Internet sharing via NAT
- WiFi extender / repeater mode
- WPA/WPA2 EAPOL handshake capture to PCAP
- DHCP fingerprinting for OS detection
- DoH mitigation with canary domain blocking
- WebSocket real-time attack statistics
- Secure OTA with HMAC-SHA256 verification
- Multi-language support (English & Indonesian)
- Persistent state saved to LittleFS

---

## Hardware Requirements

| Component | Specification |
|-----------|--------------|
| **Board** | NodeMCU 1.0 (ESP-12E Module) or any ESP8266 with 4MB flash |
| **Flash Size** | 4MB (32Mbit) required for LittleFS |
| **Core** | ESP8266 Arduino Core 2.7+ |

---

## Installation

### Method 1: One-Click Build (Recommended)

Double-click `demon_dev.bat` or run:

```cmd
.\demon_dev.bat
```

The script automatically:
1. Installs arduino-cli (if missing)
2. Installs ESP8266 core and required libraries
3. Applies IRAM-optimizing core patches
4. Presents an interactive menu

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

### Method 2: Arduino IDE

1. Add ESP8266 board URL to Preferences
2. Install ESP8266 core via Boards Manager
3. Install libraries: ESPAsyncWebServer, ESPAsyncTCP, ArduinoJson
4. Select board: NodeMCU 1.0, Flash Size: 4MB (FS:3MB OTA:~0.5MB)
5. Upload the sketch and LittleFS data

### Method 3: PlatformIO

Create `platformio.ini` in the project root and run `pio run --target upload`.

---

## Usage

### Default Access

| Parameter | Value |
|-----------|-------|
| **SSID** | `OPX-MY-DEMON` |
| **Password** | `deauther` |
| **Web UI** | `http://8.8.8.8` |
| **Serial** | 115200 baud |

### Quick Start

1. Power on the ESP8266
2. Connect to the `OPX-MY-DEMON` AP
3. Open `http://8.8.8.8` in your browser
4. Click SCAN to discover networks
5. Select a target and choose an attack

### Web UI Pages

| Page | Function |
|------|----------|
| **SCAN** | Network scanner, target selection, attack controls |
| **ATTACK** | Beacon spam, phishing page selector |
| **MONITOR** | Packet statistics, system logs |
| **SETTINGS** | Device info, PIN config, WiFi client, themes |
| **FILE MANAGER** | LittleFS file browser, upload/download |
| **CUSTOM HTML** | Upload custom phishing pages |
| **LANGUAGE** | Language editor (EN/ID) |
| **EXTENDER** | WiFi extender mode |
| **HELP** | Credits and information |

---

## Web Flasher

Flash your device directly from the browser — no tools required.

> **Website:** [https://opaminulff.netlify.app/](https://opaminulff.netlify.app/)

The web flasher (`FLASHER-WEB/index.html`) uses the Web Serial API to:
- Download firmware releases from GitHub
- Flash your ESP8266 with one click
- Works in Chrome/Edge on desktop and Android

You can also open it locally from `FLASHER-WEB/index.html` (manual upload only).

---

## Project Structure

```
OPX-MY-DEMON/
├── FLASHER-WEB/                  # Web-based flasher interface
│   ├── index.html                # Main web flasher (Tailwind CSS, inline SVGs)
│   ├── logo.jpg                  # Project logo
│   ├── netlify.toml              # Netlify deployment config
│   └── netlify/functions/proxy.js  # GitHub release download proxy
├── OPX-MY-DEMON_ESP8266/         # Firmware source
│   ├── OPX-MY-DEMON_ESP8266.ino  # Main firmware entry point
│   ├── config.h                  # Constants, pin mappings, feature flags
│   ├── attacks.h                 # Core attack engine
│   ├── phishing.h                # Phishing HTML templates (PROGMEM)
│   ├── webui.h                   # Web UI with 4 themes
│   ├── websockets.h              # WebSocket broadcast
│   ├── secure_ota.h              # Secure OTA verification
│   ├── language.h                # EN/ID translation (PROGMEM)
│   └── patches/                  # IRAM-optimized core patches
│       ├── core_esp8266_waveform_pwm.cpp
│       └── gdb_hooks.cpp
├── demon_dev.bat                 # One-click build/flash script v3.0
├── build.ps1                     # PowerShell build engine
├── LICENSE                       # MIT License
└── README.md                     # This file
```

---

## Technical Notes

- Packet injection via `wifi_send_pkt_freedom()` from ESP8266 SDK
- Promiscuous mode captures EAPOL handshakes and probe requests
- XXTEA encryption with device-unique key (MAC + Chip ID + Flash ID)
- IRAM: 43,727 / 65,536 bytes (**66%**) — 25% improvement over default
- DRAM: 57,304 / 80,192 bytes (**71%**) — stable under 80KB limit
- Filesystem: LittleFS with 4MB flash (3MB usable via `4M3M`)
- Async web server for non-blocking multi-client handling
- Captive portal via custom DNS (all requests resolve to 8.8.8.8)

---

## Changelog

### v1.0.2

- **RAM Optimization:** PROGMEM migration for strings, buffers reduced, customHTML cache removed, EAPOL data clipped
- **IRAM Optimization:** MMU=4816 (16KB ICACHE + 48KB IRAM), IRAM_ATTR removed from 12 core functions
- **Bug Fixes:** Atomic file writes, upload handler fix, hide_ap API fix, BSS Transition broadcast fix, EAPOL QoS parsing fix, log buffer reduced
- **New:** `demon_dev.bat`, IRAM core patches, AGENTS.md, FLASHER-WEB

---

## Credits

- **M1z23R** — ESP8266-EvilTwin v2 (base framework)
- **Spacehuhn** — Deauther project (packet injection reference)
- **OP AMINUL FF** — OPX-MY-DEMON modifications and 2026 upgrades

---

## License

This project is licensed under the **MIT License** — see [LICENSE](LICENSE) for details.

---

## Disclaimer

This firmware is intended for **educational purposes only**. Use only on networks you own or have explicit permission to test. The developers are not responsible for any misuse or illegal activities.
