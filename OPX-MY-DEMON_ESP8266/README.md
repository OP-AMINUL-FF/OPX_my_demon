# OPX-MY-DEMON ESP8266 v1.0.2

Complete ESP8266 firmware with Evil Twin, Deauther, Beacon, Probe, Session Hijack attacks + Web UI.

## Quick Build

Double-click **`demon_dev.bat`** (in project root) — auto-installs everything, shows interactive menu.

## Manual Flash

```cmd
arduino-cli upload --fqbn "esp8266:esp8266:nodemcuv2" --port COM3 --input-dir ..\firmware
```

Or esptool:
```bash
esptool.py --port COM3 --baud 115200 write_flash --flash_mode dout --flash_size 4MB 0x00000 OPX-MY-DEMON_ESP8266_v1.0.2.bin
```

## Pages
- **Scan** — Network scanner, target selection, attack controls
- **Attack** — Deauth / Beacon / Probe / EvilTwin / Session Hijack + phishing selector
- **Monitor** — Packet stats, system logs, power config
- **Settings** — Device info, AP config, PIN, themes, WiFi client, NAT sharing
- **Files** — LittleFS file manager
- **Custom HTML** — Upload custom phishing pages
- **Language** — English / Indonesian editor
- **Extender** — WiFi extender mode

## Memory Usage (v1.0.2)
| Section | Used | Total | % |
|---------|------|-------|---|
| DRAM | 57304 | 80192 | 71% |
| IRAM (ICACHE+IRAM) | 43727 | 65536 | 66% |
| Flash (IROM) | 395956 | 1048576 | 37% |

## Usage
- AP: `NETHERCAP` / Pass: `deauther`
- Web UI: `http://8.8.8.8`
