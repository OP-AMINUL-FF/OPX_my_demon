param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Resolve-Path "$PSScriptRoot"
$CoreDir = "$env:LOCALAPPDATA\Arduino15\packages\esp8266\hardware\esp8266\3.1.2\cores\esp8266"
$PatchesDir = "$ProjectRoot\patches"
$BackupDir = "$PatchesDir\backup"
$FQBN = "esp8266:esp8266:nodemcuv2:eesz=4M3M,xtal=160,mmu=4816,dbg=Disabled,lvl=None____,ip=lm2f,vt=flash,exception=disabled,stacksmash=disabled,ssl=all,non32xfer=fast"
$InoFile = "$ProjectRoot\OPX-MY-DEMON_ESP8266.ino"

function Write-Step($msg) {
    Write-Host "==> $msg" -ForegroundColor Cyan
}

# ---- Patch: backup originals (first run only) ----
Write-Step "Patching ESP8266 core files (IRAM_ATTR removed)..."
if (-not (Test-Path "$BackupDir\gdb_hooks.cpp")) {
    Write-Step "First run - backing up originals to patches\backup\"
    New-Item -ItemType Directory -Force -Path $BackupDir | Out-Null
    Copy-Item "$CoreDir\core_esp8266_waveform_pwm.cpp" "$BackupDir\" -Force
    Copy-Item "$CoreDir\gdb_hooks.cpp" "$BackupDir\" -Force
}

Copy-Item "$PatchesDir\core_esp8266_waveform_pwm.cpp" "$CoreDir\" -Force
Copy-Item "$PatchesDir\gdb_hooks.cpp" "$CoreDir\" -Force
Write-Step "Patches applied."

# ---- Clean caches if requested ----
if ($Clean) {
    Write-Step "Cleaning cached build artifacts..."
    Remove-Item -Recurse -Force "$env:LOCALAPPDATA\arduino\cores" -ErrorAction SilentlyContinue
    Write-Step "Cache cleared."
}

# ---- Compile ----
Write-Step "Compiling firmware..."
# Use cmd.exe to avoid PowerShell stderr ErrorRecord noise
$allLines = cmd /c "arduino-cli compile --fqbn `"$FQBN`" `"$InoFile`" 2>&1"
$exitCode = $LASTEXITCODE

if ($exitCode -ne 0) {
    Write-Host "BUILD FAILED" -ForegroundColor Red
    $allLines
    exit 1
}

# ---- Show memory summary ----
Write-Step "Build successful! Memory usage:"
$allLines | Select-String -Pattern "RAM|flash"
