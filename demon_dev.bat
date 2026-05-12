@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"
title DEMON DEV TOOL v2.0
cls

:: ---- Color Support ----
for /f "tokens=1,2 delims=#" %%a in ('"prompt #$H#$E# & echo on & for %%b in (1) do rem"') do set "ESC=%%b"

:: ---- Config ----
set "PS=powershell -NoProfile -ExecutionPolicy Bypass -Command"
set "SCRIPT_DIR=%CD%\OPX-MY-DEMON_ESP8266"
set "FIRMWARE_DIR=%CD%\firmware"
set "FQBN=esp8266:esp8266:nodemcuv2:eesz=4M3M,xtal=160,mmu=4816,dbg=Disabled,lvl=None____,ip=lm2f,vt=flash,exception=disabled,stacksmash=disabled,ssl=all,non32xfer=fast"
set "LIBS=ESP AsyncWebServer ESPAsyncTCP ArduinoJson"

:: ---- Banner ----
:menu
cls
echo.
echo %ESC%[36m  +==============================================+%ESC%[0m
echo %ESC%[36m  |        DEMON DEV TOOL v2.0                  |%ESC%[0m
echo %ESC%[36m  |     OPX-MY-DEMON Build System               |%ESC%[0m
echo %ESC%[36m  +==============================================+%ESC%[0m
echo.

:: ---- Check Arduino-CLI (auto-install if missing) ----
where arduino-cli >nul 2>&1
if %errorlevel% neq 0 (
    echo %ESC%[33m  [!] arduino-cli not found! Installing...%ESC%[0m
    winget install Arduino.ArduinoCLI >nul 2>&1
    if !errorlevel! neq 0 (
        echo %ESC%[31m  [X] Failed to install arduino-cli via winget%ESC%[0m
        echo %ESC%[33m  [!] Try manual install: https://arduino.github.io/arduino-cli/installation/%ESC%[0m
        pause
        exit /b 1
    )
    for /f "tokens=2*" %%a in ('reg query HKCU\Environment /v PATH 2^>nul') do set "USER_PATH=%%b"
    if defined USER_PATH set "PATH=%PATH%;%USER_PATH%"
    where arduino-cli >nul 2>&1
    if !errorlevel! neq 0 (
        echo %ESC%[33m  [!] arduino-cli installed but not in PATH. Restart terminal or add manually.%ESC%[0m
    ) else (
        echo %ESC%[32m  [+] arduino-cli installed successfully%ESC%[0m
    )
) else (
    echo %ESC%[32m  [+] arduino-cli found%ESC%[0m
)

:: ---- Auto-install ESP8266 Core ----
echo %ESC%[36m  ==^> Checking ESP8266 core...%ESC%[0m
%PS% "arduino-cli core list 2>&1" | find "esp8266" >nul
if %errorlevel% neq 0 (
    echo %ESC%[33m  [!] ESP8266 core not installed. Installing...%ESC%[0m
    echo %ESC%[36m  ==^> Updating core index...%ESC%[0m
    arduino-cli core update-index >nul 2>&1
    echo %ESC%[36m  ==^> Installing esp8266:esp8266...%ESC%[0m
    arduino-cli core install esp8266:esp8266 >nul 2>&1
    if !errorlevel! equ 0 (
        echo %ESC%[32m  [+] ESP8266 core installed%ESC%[0m
    ) else (
        echo %ESC%[31m  [X] ESP8266 core install failed%ESC%[0m
    )
) else (
    echo %ESC%[32m  [+] ESP8266 core already installed%ESC%[0m
)

:: ---- Auto-install Libraries ----
echo %ESC%[36m  ==^> Checking libraries...%ESC%[0m
for %%L in (%LIBS%) do (
    %PS% "arduino-cli lib list 2>&1" | find "%%L" >nul
    if !errorlevel! neq 0 (
        echo %ESC%[33m  [!] Installing library: %%L%ESC%[0m
        arduino-cli lib install "%%L" >nul 2>&1
        if !errorlevel! equ 0 (
            echo %ESC%[32m  [+] Library installed: %%L%ESC%[0m
        ) else (
            echo %ESC%[31m  [X] Failed to install: %%L%ESC%[0m
        )
    ) else (
        echo %ESC%[32m  [+] Library: %%L%ESC%[0m
    )
)

echo.
echo %ESC%[36m  +==============================================+%ESC%[0m
echo %ESC%[36m  |             AVAILABLE OPTIONS                |%ESC%[0m
echo %ESC%[36m  +==============================================+%ESC%[0m
echo.
echo %ESC%[33m   [1]%ESC%[0m  Compile Firmware
echo %ESC%[33m   [2]%ESC%[0m  Compile + Flash (auto-detect port)
echo %ESC%[33m   [3]%ESC%[0m  Compile + Flash + Serial Monitor
echo %ESC%[33m   [4]%ESC%[0m  Flash Existing Binary Only
echo %ESC%[33m   [5]%ESC%[0m  Open Serial Monitor
echo %ESC%[33m   [6]%ESC%[0m  List Available COM Ports
echo %ESC%[33m   [7]%ESC%[0m  Restore Original Core Files
echo %ESC%[33m   [8]%ESC%[0m  Full Clean Build
echo %ESC%[33m   [9]%ESC%[0m  Upload LittleFS Data
echo %ESC%[33m   [0]%ESC%[0m  Exit
echo.

:: ---- Auto-patch core files ----
set "BACKUP_DIR=%SCRIPT_DIR%\patches\backup"
set "PATCHES_DIR=%SCRIPT_DIR%\patches"
set "CORE_DIR=%LOCALAPPDATA%\Arduino15\packages\esp8266\hardware\esp8266\3.1.2\cores\esp8266"

if not exist "%CORE_DIR%\gdb_hooks.cpp" (
    echo %ESC%[33m  [!] ESP8266 core directory not found. Run option [1] to set up.%ESC%[0m
) else (
    if not exist "%BACKUP_DIR%\gdb_hooks.cpp" (
        mkdir "%BACKUP_DIR%" >nul 2>&1
        copy "%CORE_DIR%\core_esp8266_waveform_pwm.cpp" "%BACKUP_DIR%\" >nul 2>&1
        copy "%CORE_DIR%\gdb_hooks.cpp" "%BACKUP_DIR%\" >nul 2>&1
    )
    copy /y "%PATCHES_DIR%\core_esp8266_waveform_pwm.cpp" "%CORE_DIR%\" >nul 2>&1
    copy /y "%PATCHES_DIR%\gdb_hooks.cpp" "%CORE_DIR%\" >nul 2>&1
)

:: ---- User Input ----
set /p "CHOICE=%ESC%[36m  Select option [0-9]: %ESC%[0m"

if "%CHOICE%"=="1" goto compile
if "%CHOICE%"=="2" goto compile_flash
if "%CHOICE%"=="3" goto compile_flash_monitor
if "%CHOICE%"=="4" goto flash_only
if "%CHOICE%"=="5" goto monitor
if "%CHOICE%"=="6" goto list_ports
if "%CHOICE%"=="7" goto restore
if "%CHOICE%"=="8" goto clean_build
if "%CHOICE%"=="9" goto littlefs
if "%CHOICE%"=="0" goto end
goto menu

:: ==============================================
:: OPTION 1: COMPILE ONLY
:: ==============================================
:compile
cls
echo.
echo %ESC%[36m  +==============================================+%ESC%[0m
echo %ESC%[36m  |           Compiling Firmware...              |%ESC%[0m
echo %ESC%[36m  +==============================================+%ESC%[0m
echo.

%PS% "
    & '%SCRIPT_DIR%\build.ps1'
"

echo.
echo %ESC%[32m  ====== Done ======%ESC%[0m
pause
goto menu

:: ==============================================
:: OPTION 2: COMPILE + FLASH
:: ==============================================
:compile_flash
cls
echo.
echo %ESC%[36m  +==============================================+%ESC%[0m
echo %ESC%[36m  |       Compiling + Flashing...               |%ESC%[0m
echo %ESC%[36m  +==============================================+%ESC%[0m
echo.

echo %ESC%[33m  Available COM ports:%ESC%[0m
%PS% "arduino-cli board list 2>&1" | findstr /r "^COM"
echo.
set /p "PORT=%ESC%[36m  Enter COM port (e.g. COM3) or press Enter for auto: %ESC%[0m"

if "%PORT%"=="" (
    %PS% "& '%SCRIPT_DIR%\build.ps1' -Flash"
) else (
    %PS% "& '%SCRIPT_DIR%\build.ps1' -Flash -Port '%PORT%'"
)

echo.
echo %ESC%[32m  ====== Done ======%ESC%[0m
pause
goto menu

:: ==============================================
:: OPTION 3: COMPILE + FLASH + MONITOR
:: ==============================================
:compile_flash_monitor
cls
echo.
echo %ESC%[36m  +==============================================+%ESC%[0m
echo %ESC%[36m  |     Compile + Flash + Monitor               |%ESC%[0m
echo %ESC%[36m  +==============================================+%ESC%[0m
echo.

echo %ESC%[33m  Available COM ports:%ESC%[0m
%PS% "arduino-cli board list 2>&1" | findstr /r "^COM"
echo.
set /p "PORT=%ESC%[36m  Enter COM port (e.g. COM3) or press Enter for auto: %ESC%[0m"

if "%PORT%"=="" (
    %PS% "& '%SCRIPT_DIR%\build.ps1' -Flash -Monitor"
) else (
    %PS% "& '%SCRIPT_DIR%\build.ps1' -Flash -Monitor -Port '%PORT%'"
)

echo.
echo %ESC%[32m  ====== Done ======%ESC%[0m
pause
goto menu

:: ==============================================
:: OPTION 4: FLASH EXISTING BINARY
:: ==============================================
:flash_only
cls
echo.
echo %ESC%[36m  +==============================================+%ESC%[0m
echo %ESC%[36m  |         Flashing Existing Binary             |%ESC%[0m
echo %ESC%[36m  +==============================================+%ESC%[0m
echo.

echo %ESC%[33m  Available COM ports:%ESC%[0m
%PS% "arduino-cli board list 2>&1" | findstr /r "^COM"
echo.
set /p "PORT=%ESC%[36m  Enter COM port (e.g. COM3): %ESC%[0m"

if "%PORT%"=="" (
    echo %ESC%[31m  [X] No port specified%ESC%[0m
    pause
    goto menu
)

echo %ESC%[36m  ==^> Flashing %PORT%...%ESC%[0m
arduino-cli upload --fqbn "esp8266:esp8266:nodemcuv2" --port "%PORT%" --input-dir "%FIRMWARE_DIR%"
if %errorlevel% equ 0 (
    echo %ESC%[32m  [+] Flash successful on %PORT%%ESC%[0m
) else (
    echo %ESC%[31m  [X] Flash failed on %PORT%%ESC%[0m
)

pause
goto menu

:: ==============================================
:: OPTION 5: SERIAL MONITOR
:: ==============================================
:monitor
cls
echo.
echo %ESC%[36m  +==============================================+%ESC%[0m
echo %ESC%[36m  |         Serial Monitor (115200)              |%ESC%[0m
echo %ESC%[36m  +==============================================+%ESC%[0m
echo.

echo %ESC%[33m  Available COM ports:%ESC%[0m
%PS% "arduino-cli board list 2>&1" | findstr /r "^COM"
echo.
set /p "PORT=%ESC%[36m  Enter COM port (e.g. COM3): %ESC%[0m"

if "%PORT%"=="" (
    echo %ESC%[31m  [X] No port specified%ESC%[0m
    pause
    goto menu
)

echo %ESC%[36m  ==^> Opening serial monitor on %PORT%...%ESC%[0m
echo %ESC%[33m  [!] Press Ctrl+C to exit monitor%ESC%[0m
echo.
arduino-cli monitor -p "%PORT%" --config baudrate=115200

pause
goto menu

:: ==============================================
:: OPTION 6: LIST PORTS
:: ==============================================
:list_ports
cls
echo.
echo %ESC%[36m  +==============================================+%ESC%[0m
echo %ESC%[36m  |         Available COM Ports                 |%ESC%[0m
echo %ESC%[36m  +==============================================+%ESC%[0m
echo.
%PS% "arduino-cli board list 2>&1"
echo.
pause
goto menu

:: ==============================================
:: OPTION 7: RESTORE CORE FILES
:: ==============================================
:restore
cls
echo.
echo %ESC%[36m  +==============================================+%ESC%[0m
echo %ESC%[36m  |     Restoring Original Core Files            |%ESC%[0m
echo %ESC%[36m  +==============================================+%ESC%[0m
echo.

if not exist "%BACKUP_DIR%\gdb_hooks.cpp" (
    echo %ESC%[31m  [X] No backups found in patches\backup\%ESC%[0m
    pause
    goto menu
)

copy /y "%BACKUP_DIR%\core_esp8266_waveform_pwm.cpp" "%CORE_DIR%\" >nul 2>&1
copy /y "%BACKUP_DIR%\gdb_hooks.cpp" "%CORE_DIR%\" >nul 2>&1
echo %ESC%[32m  [+] Original core files restored%ESC%[0m

pause
goto menu

:: ==============================================
:: OPTION 8: FULL CLEAN BUILD
:: ==============================================
:clean_build
cls
echo.
echo %ESC%[36m  +==============================================+%ESC%[0m
echo %ESC%[36m  |        Full Clean Build                      |%ESC%[0m
echo %ESC%[36m  +==============================================+%ESC%[0m
echo.

%PS% "& '%SCRIPT_DIR%\build.ps1' -Clean"

echo.
echo %ESC%[32m  ====== Done ======%ESC%[0m
pause
goto menu

:: ==============================================
:: OPTION 9: LITTLEFS UPLOAD
:: ==============================================
:littlefs
cls
echo.
echo %ESC%[36m  +==============================================+%ESC%[0m
echo %ESC%[36m  |       Uploading LittleFS Data                |%ESC%[0m
echo %ESC%[36m  +==============================================+%ESC%[0m
echo.

if not exist "%SCRIPT_DIR%\data" (
    echo %ESC%[33m  [!] No 'data' directory found in sketch folder%ESC%[0m
    pause
    goto menu
)

echo %ESC%[33m  Available COM ports:%ESC%[0m
%PS% "arduino-cli board list 2>&1" | findstr /r "^COM"
echo.
set /p "PORT=%ESC%[36m  Enter COM port (e.g. COM3): %ESC%[0m"

if "%PORT%"=="" (
    echo %ESC%[31m  [X] No port specified%ESC%[0m
    pause
    goto menu
)

echo %ESC%[36m  ==^> Uploading LittleFS data to %PORT%...%ESC%[0m
arduino-cli upload --fqbn "esp8266:esp8266:nodemcuv2" --port "%PORT%" --input-dir "%SCRIPT_DIR%\data"
if %errorlevel% equ 0 (
    echo %ESC%[32m  [+] LittleFS upload successful%ESC%[0m
) else (
    echo %ESC%[31m  [X] LittleFS upload failed%ESC%[0m
)

pause
goto menu

:: ==============================================
:: END
:: ==============================================
:end
cls
echo.
echo %ESC%[36m  Goodbye!%ESC%[0m
echo.
timeout /t 2 >nul
exit /b 0
