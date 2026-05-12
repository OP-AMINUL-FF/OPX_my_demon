@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"
title DEMON DEV TOOL v3.0
cls

:: ============================================================
:: DEMON DEV TOOL v3.0 - Build & Flash Utility for ESP8266
:: ============================================================

set "APP_VER=3.0"
set "FW_VER=1.0.2"

set "PSCMD=powershell -NoProfile -ExecutionPolicy Bypass -Command"
set "PSFILE=powershell -NoProfile -ExecutionPolicy Bypass -File"
set "SCRIPT_DIR=%CD%\OPX-MY-DEMON_ESP8266"
set "FIRMWARE_DIR=%SCRIPT_DIR%"
set "CORE_DIR=%LOCALAPPDATA%\Arduino15\packages\esp8266\hardware\esp8266\3.1.2\cores\esp8266"
set "BACKUP_DIR=%SCRIPT_DIR%\patches\backup"
set "PATCHES_DIR=%SCRIPT_DIR%\patches"
set "FQBN=esp8266:esp8266:nodemcuv2:eesz=4M3M,xtal=160,mmu=4816,dbg=Disabled,lvl=None____,ip=lm2f,vt=flash,exception=disabled,stacksmash=disabled,ssl=all,non32xfer=fast"
set "LIBS=ESP AsyncWebServer ESPAsyncTCP ArduinoJson"

:: ============================================================
:: MAIN MENU
:: ============================================================
:menu
cls
echo.
echo  +==============================================+
echo  ^|        DEMON DEV TOOL v%APP_VER%                 ^|
echo  ^|     OPX-MY-DEMON Build System               ^|
echo  +==============================================+
echo.
echo  === BUILD ======================================
echo.
echo    [1]  Compile Firmware
echo    [2]  Compile + Flash (Auto detect)
echo    [3]  Compile + Flash (Manual - pick .bin + port)
echo.
echo  === FLASH ======================================
echo.
echo    [4]  Flash Binary - Auto (auto port + latest .bin)
echo    [5]  Flash Binary - Manual (pick .bin + port)
echo.
echo  === MONITOR ^& PORTS ===========================
echo.
echo    [6]  Open Serial Monitor
echo    [7]  List COM Ports
echo.
echo  === UTILITIES ==================================
echo.
echo    [8]  Clean Build Cache
echo    [9]  List Available Binaries
echo    [10] Restore Original Core Files
echo    [11] Upload LittleFS Data
echo    [12] Firmware Version Info
echo.
echo  ================================================
echo    [0]  Exit
echo.

:: ---- Auto-Setup All Requirements (Background) ----
where arduino-cli >nul 2>&1
if %errorlevel% neq 0 (
    winget install Arduino.ArduinoCLI >nul 2>&1
    for /f "tokens=2*" %%a in ('reg query HKCU\Environment /v PATH 2^>nul') do set "USER_PATH=%%b"
    if defined USER_PATH set "PATH=!PATH!;!USER_PATH!"
)
where arduino-cli >nul 2>&1
if %errorlevel% equ 0 (
    arduino-cli core update-index >nul 2>&1
    arduino-cli core install esp8266:esp8266 >nul 2>&1
    for %%L in (%LIBS%) do (
        arduino-cli lib list 2>nul | find "%%L" >nul
        if !errorlevel! neq 0 ( arduino-cli lib install "%%L" >nul 2>&1 )
    )
)
if exist "%CORE_DIR%\gdb_hooks.cpp" (
    if not exist "%BACKUP_DIR%\gdb_hooks.cpp" (
        mkdir "%BACKUP_DIR%" >nul 2>&1
        copy "%CORE_DIR%\core_esp8266_waveform_pwm.cpp" "%BACKUP_DIR%\" >nul 2>&1
        copy "%CORE_DIR%\gdb_hooks.cpp" "%BACKUP_DIR%\" >nul 2>&1
    )
    copy /y "%PATCHES_DIR%\core_esp8266_waveform_pwm.cpp" "%CORE_DIR%\" >nul 2>&1
    copy /y "%PATCHES_DIR%\gdb_hooks.cpp" "%CORE_DIR%\" >nul 2>&1
)

echo.
set /p "CHOICE=  Select option [0-12]: "

if "%CHOICE%"=="1" goto compile
if "%CHOICE%"=="2" goto compile_flash_auto
if "%CHOICE%"=="3" goto compile_flash_manual
if "%CHOICE%"=="4" goto flash_auto
if "%CHOICE%"=="5" goto flash_manual
if "%CHOICE%"=="6" goto monitor
if "%CHOICE%"=="7" goto list_ports
if "%CHOICE%"=="8" goto clean_build
if "%CHOICE%"=="9" goto list_binaries
if "%CHOICE%"=="10" goto restore
if "%CHOICE%"=="11" goto littlefs
if "%CHOICE%"=="12" goto version_info
if "%CHOICE%"=="0" goto end
goto menu

:: ============================================================
:: OPTION 1: COMPILE
:: ============================================================
:compile
cls
echo.
echo  +==============================================+
echo  ^|           Compiling Firmware...              ^|
echo  +==============================================+
echo.
%PSFILE% "%SCRIPT_DIR%\build.ps1"
echo.
echo   ====== Done ======
pause
goto menu

:: ============================================================
:: OPTION 2: COMPILE + FLASH AUTO
:: ============================================================
:compile_flash_auto
cls
echo.
echo  +==============================================+
echo  ^|     Compile + Flash (Auto)                  ^|
echo  +==============================================+
echo.
%PSFILE% "%SCRIPT_DIR%\build.ps1"
if %errorlevel% neq 0 (
    echo   [X] Build failed. Flash aborted.
    pause
    goto menu
)
echo.
echo   ==^> Detecting ESP8266 port...
call :detect_port_auto
if "!SELECTED_PORT!"=="" (
    echo   [X] No ESP8266 detected. Connect board and try Manual flash.
    pause
    goto menu
)
echo   [+] Auto-detected: !SELECTED_PORT!
echo.
echo   ==^> Picking latest binary...
call :pick_binary_latest
if "!SELECTED_BIN!"=="" (
    echo   [X] No .bin found in !FIRMWARE_DIR!
    pause
    goto menu
)
echo   [+] Using: !SELECTED_BIN!
echo.
echo   ==^> Flashing !SELECTED_BIN! to !SELECTED_PORT!...
arduino-cli upload --fqbn "esp8266:esp8266:nodemcuv2" --port "!SELECTED_PORT!" --input-dir "!FIRMWARE_DIR!"
if %errorlevel% equ 0 ( echo   [+] Flash successful ) else ( echo   [X] Flash failed )
echo.
pause
goto menu

:: ============================================================
:: OPTION 3: COMPILE + FLASH MANUAL
:: ============================================================
:compile_flash_manual
cls
echo.
echo  +==============================================+
echo  ^|    Compile + Flash (Manual)                 ^|
echo  +==============================================+
echo.
%PSFILE% "%SCRIPT_DIR%\build.ps1"
if %errorlevel% neq 0 (
    echo   [X] Build failed. Flash aborted.
    pause
    goto menu
)
echo.
call :pick_binary
if "!SELECTED_BIN!"=="" (
    echo   [X] No binary selected.
    pause
    goto menu
)
call :pick_port
if "!SELECTED_PORT!"=="" (
    echo   [X] No port selected.
    pause
    goto menu
)
echo.
echo   ==^> Flashing !SELECTED_BIN! to !SELECTED_PORT!...
arduino-cli upload --fqbn "esp8266:esp8266:nodemcuv2" --port "!SELECTED_PORT!" --input-dir "!FIRMWARE_DIR!"
if %errorlevel% equ 0 ( echo   [+] Flash successful ) else ( echo   [X] Flash failed )
echo.
pause
goto menu

:: ============================================================
:: OPTION 4: FLASH AUTO (auto port + latest .bin)
:: ============================================================
:flash_auto
cls
echo.
echo  +==============================================+
echo  ^|         Flash Binary - Auto                 ^|
echo  +==============================================+
echo.
call :detect_port_auto
if "!SELECTED_PORT!"=="" (
    echo   [X] No ESP8266 detected.
    pause
    goto menu
)
echo   [+] Auto-detected: !SELECTED_PORT!
echo.
call :pick_binary_latest
if "!SELECTED_BIN!"=="" (
    echo   [X] No .bin found.
    pause
    goto menu
)
echo   [+] Using: !SELECTED_BIN!
echo.
echo   ==^> Flashing !SELECTED_BIN! to !SELECTED_PORT!...
arduino-cli upload --fqbn "esp8266:esp8266:nodemcuv2" --port "!SELECTED_PORT!" --input-dir "!FIRMWARE_DIR!"
if %errorlevel% equ 0 ( echo   [+] Flash successful ) else ( echo   [X] Flash failed )
echo.
pause
goto menu

:: ============================================================
:: OPTION 5: FLASH MANUAL (pick .bin + port)
:: ============================================================
:flash_manual
cls
echo.
echo  +==============================================+
echo  ^|         Flash Binary - Manual               ^|
echo  +==============================================+
echo.
call :pick_binary
if "!SELECTED_BIN!"=="" (
    echo   [X] No binary selected.
    pause
    goto menu
)
call :pick_port
if "!SELECTED_PORT!"=="" (
    echo   [X] No port selected.
    pause
    goto menu
)
echo.
echo   ==^> Flashing !SELECTED_BIN! to !SELECTED_PORT!...
arduino-cli upload --fqbn "esp8266:esp8266:nodemcuv2" --port "!SELECTED_PORT!" --input-dir "!FIRMWARE_DIR!"
if %errorlevel% equ 0 ( echo   [+] Flash successful ) else ( echo   [X] Flash failed )
echo.
pause
goto menu

:: ============================================================
:: OPTION 6: SERIAL MONITOR
:: ============================================================
:monitor
cls
echo.
echo  +==============================================+
echo  ^|         Serial Monitor (115200)              ^|
echo  +==============================================+
echo.
call :pick_port
if "!SELECTED_PORT!"=="" (
    pause
    goto menu
)
echo   ==^> Opening serial monitor on !SELECTED_PORT!...
echo   [!] Press Ctrl+C to exit
echo.
arduino-cli monitor -p "!SELECTED_PORT!" --config baudrate=115200
pause
goto menu

:: ============================================================
:: OPTION 7: LIST PORTS
:: ============================================================
:list_ports
cls
echo.
echo  +==============================================+
echo  ^|         Available COM Ports                 ^|
echo  +==============================================+
echo.
%PSCMD% "arduino-cli board list 2>&1"
echo.
pause
goto menu

:: ============================================================
:: OPTION 8: CLEAN BUILD
:: ============================================================
:clean_build
cls
echo.
echo  +==============================================+
echo  ^|        Clean Build Cache                    ^|
echo  +==============================================+
echo.
%PSFILE% "%SCRIPT_DIR%\build.ps1" -Clean
echo.
echo   [+] Build cache cleared
echo.
pause
goto menu

:: ============================================================
:: OPTION 9: LIST BINARIES
:: ============================================================
:list_binaries
cls
echo.
echo  +==============================================+
echo  ^|      Available Firmware Binaries             ^|
echo  +==============================================+
echo.
echo   Location: !FIRMWARE_DIR!
echo.
if not exist "!FIRMWARE_DIR!\*.bin" (
    echo   [X] No .bin files found.
    echo   [!] Compile firmware first ^(Option 1^).
    pause
    goto menu
)
echo   File Name                                     Size
echo   --------------------------------------------- -------
set "_total=0"
for %%f in ("!FIRMWARE_DIR!\*.bin") do (
    set "_size=%%~zf"
    set /a "_size_kb=!_size!/1024"
    if !_size_kb! lss 1 set "_size_kb=1"
    echo   %%~nxf                          !_size_kb! KB
    set /a "_total+=!_size!"
)
set /a "_total_kb=!_total!/1024"
if !_total_kb! lss 1 set "_total_kb=1"
echo   --------------------------------------------- -------
echo   Total: !_total_kb! KB
echo.
pause
goto menu

:: ============================================================
:: OPTION 10: RESTORE CORE
:: ============================================================
:restore
cls
echo.
echo  +==============================================+
echo  ^|     Restoring Original Core Files            ^|
echo  +==============================================+
echo.
if not exist "%BACKUP_DIR%\gdb_hooks.cpp" (
    echo   [X] No backups found in patches\backup\
    pause
    goto menu
)
copy /y "%BACKUP_DIR%\core_esp8266_waveform_pwm.cpp" "%CORE_DIR%\" >nul 2>&1
copy /y "%BACKUP_DIR%\gdb_hooks.cpp" "%CORE_DIR%\" >nul 2>&1
echo   [+] Original core files restored
pause
goto menu

:: ============================================================
:: OPTION 11: LITTLEFS UPLOAD
:: ============================================================
:littlefs
cls
echo.
echo  +==============================================+
echo  ^|       Uploading LittleFS Data                ^|
echo  +==============================================+
echo.
if not exist "%SCRIPT_DIR%\data" (
    echo   [!] No 'data' directory found in sketch folder
    pause
    goto menu
)
call :pick_port
if "!SELECTED_PORT!"=="" (
    pause
    goto menu
)
echo   ==^> Uploading LittleFS data to !SELECTED_PORT!...
arduino-cli upload --fqbn "esp8266:esp8266:nodemcuv2" --port "!SELECTED_PORT!" --input-dir "%SCRIPT_DIR%\data"
if %errorlevel% equ 0 ( echo   [+] LittleFS upload successful ) else ( echo   [X] LittleFS upload failed )
pause
goto menu

:: ============================================================
:: OPTION 12: VERSION INFO
:: ============================================================
:version_info
cls
echo.
echo  +==============================================+
echo  ^|        Firmware Version Info                 ^|
echo  +==============================================+
echo.
echo   Demon Dev Tool  : v%APP_VER%
echo   Firmware Version : v%FW_VER%
echo.
echo   Project : OPX-MY-DEMON_ESP8266
echo   FQBN    : esp8266:esp8266:nodemcuv2
echo   MMU     : 4816  (16KB ICACHE + 48KB IRAM)
echo.
if exist "%SCRIPT_DIR%\OPX-MY-DEMON_ESP8266_v%FW_VER%.bin" (
    for %%f in ("%SCRIPT_DIR%\OPX-MY-DEMON_ESP8266_v%FW_VER%.bin") do (
        set /a _sz=%%~zf/1024
        if !_sz! lss 1 set _sz=1
        echo   Binary   : OPX-MY-DEMON_ESP8266_v%FW_VER%.bin ^(!_sz! KB^)
    )
)
echo.
if exist "..\.build-memory.log" (
    echo   --- Memory Log ^(last build^) ---
    for /f "skip=5 tokens=*" %%a in ('type "..\.build-memory.log"') do (
        echo   %%a
        goto :memlog_done
    )
    echo     ^(no recent entries^)
)
:memlog_done
echo.
if not exist "..\.build-memory.log" (
    echo   ^(No build memory log found^)
    echo.
)
echo   Patches Applied:
if exist "%CORE_DIR%\gdb_hooks.cpp" (
    findstr /i "IRAM_ATTR" "%CORE_DIR%\core_esp8266_waveform_pwm.cpp" >nul 2>&1
    if !errorlevel! equ 0 ( echo     - core_esp8266_waveform_pwm.cpp: Original
    ) else ( echo     - core_esp8266_waveform_pwm.cpp: Patched )
) else ( echo     - Core not yet installed )
echo.
echo   Available Binaries:
if exist "!FIRMWARE_DIR!\*.bin" (
    for %%f in ("!FIRMWARE_DIR!\*.bin") do (
        set /a _sz=%%~zf/1024
        if !_sz! lss 1 set _sz=1
        echo     %%~nxf ^(!_sz! KB^)
    )
) else (
    echo     ^(No binaries - compile first^)
)
echo.
pause
goto menu

:: ============================================================
:: SUBROUTINES
:: ============================================================

:: ---- Pick Binary: user selects from list ----
:pick_binary
if not exist "!FIRMWARE_DIR!\*.bin" (
    echo   [X] No .bin files found in !FIRMWARE_DIR!
    echo   [!] Compile firmware first ^(Option 1^).
    set "SELECTED_BIN="
    exit /b 1
)
set "BIN_COUNT=0"
for %%f in ("!FIRMWARE_DIR!\*.bin") do (
    set /a BIN_COUNT+=1
    set "BIN_!BIN_COUNT!=%%~nxf"
)
if !BIN_COUNT! equ 1 (
    set "SELECTED_BIN=!BIN_1!"
    echo   [+] Auto-selected: !SELECTED_BIN!
    exit /b 0
)
echo.
echo   Select .bin file to flash:
echo.
for /l %%i in (1,1,!BIN_COUNT!) do (
    echo   [%%i] !BIN_%%i!
)
echo.
set /p "BIN_CHOICE=  Enter number [1-!BIN_COUNT!]: "
if "!BIN_CHOICE!"=="" set "BIN_CHOICE=0"
if !BIN_CHOICE! lss 1 (
    echo   [X] Invalid choice
    set "SELECTED_BIN="
    exit /b 1
)
if !BIN_CHOICE! gtr !BIN_COUNT! (
    echo   [X] Invalid choice
    set "SELECTED_BIN="
    exit /b 1
)
for %%i in (!BIN_CHOICE!) do set "SELECTED_BIN=!BIN_%%i!"
echo   [+] Selected: !SELECTED_BIN!
exit /b 0

:: ---- Pick Binary Latest: auto-pick newest ----
:pick_binary_latest
if not exist "!FIRMWARE_DIR!\*.bin" (
    set "SELECTED_BIN="
    exit /b 1
)
for /f "delims=" %%f in ('dir /b /o-d "!FIRMWARE_DIR!\*.bin" 2^>nul') do (
    set "SELECTED_BIN=%%f"
    exit /b 0
)
set "SELECTED_BIN="
exit /b 1

:: ---- Pick COM Port: user selects from detected ----
:pick_port
%PSCMD% "[System.IO.Ports.SerialPort]::GetPortNames()" > "%TEMP%\demon_ports.txt" 2>&1
set "PORT_COUNT=0"
for /f "tokens=*" %%a in ('type "%TEMP%\demon_ports.txt"') do (
    if not "%%a"=="" (
        set /a PORT_COUNT+=1
        set "PORT_!PORT_COUNT!=%%a"
    )
)
del "%TEMP%\demon_ports.txt" >nul 2>&1

if !PORT_COUNT! equ 0 (
    echo   [X] No COM ports detected.
    set "SELECTED_PORT="
    exit /b 1
)
if !PORT_COUNT! equ 1 (
    set "SELECTED_PORT=!PORT_1!"
    echo   [+] Auto-detected: !SELECTED_PORT!
    exit /b 0
)
echo.
echo   Available COM ports:
echo.
for /l %%i in (1,1,!PORT_COUNT!) do (
    echo   [%%i] !PORT_%%i!
)
echo.
set /p PORT_CHOICE=  Enter port number [1-!PORT_COUNT!]: 
if "!PORT_CHOICE!"=="" set "PORT_CHOICE=0"
if !PORT_CHOICE! lss 1 (
    echo   [X] Invalid choice
    set "SELECTED_PORT="
    exit /b 1
)
if !PORT_CHOICE! gtr !PORT_COUNT! (
    echo   [X] Invalid choice
    set "SELECTED_PORT="
    exit /b 1
)
for %%i in (!PORT_CHOICE!) do set "SELECTED_PORT=!PORT_%%i!"
echo   [+] Selected: !SELECTED_PORT!
exit /b 0

:: ---- Detect Port Auto: find ESP8266 via arduino-cli ----
:detect_port_auto
for /f "tokens=1" %%a in ('%PSCMD% "arduino-cli board list 2>&1" ^| findstr /r "^COM"') do (
    set "SELECTED_PORT=%%a"
    exit /b 0
)
set "SELECTED_PORT="
exit /b 1

:: ============================================================
:: END
:: ============================================================
:end
cls
echo.
echo   Goodbye!
echo.
timeout /t 2 >nul
exit /b 0
