# Build

## Quick Build

```cmd
.\demon_dev.bat
```

Or double-click `demon_dev.bat` in File Explorer.

## Build Configuration
- FQBN: `esp8266:esp8266:nodemcuv2:eesz=4M3M,xtal=160,mmu=4816,dbg=Disabled,lvl=None____,ip=lm2f,vt=flash,exception=disabled,stacksmash=disabled,ssl=all,non32xfer=fast`
- MMU=4816 → 16KB ICACHE + 48KB IRAM (vs default 32KB+32KB)
- This is the main contributor to IRAM reduction (91% → 66%)

## Core Patches (IRAM_ATTR removed)
- `patches/core_esp8266_waveform_pwm.cpp`: IRAM_ATTR removed from 11 functions (timer1Interrupt, forceTimerInterrupt, disableIdleTimer, _notifyPWM, _stopPWM_weak, _stopPWM, stopWaveform_weak, stopWaveform, GetCycleCountIRQ, earliest)
- `patches/gdb_hooks.cpp`: IRAM_ATTR removed from `__gdb_no_op()`
- `postmortem.cpp` NOT patched (required for abort/panic linker symbols)
- First run of `demon_dev.bat` backs up originals to `patches/backup/`
- To restore: Option 10 in `demon_dev.bat` or `build.ps1 -Restore`
