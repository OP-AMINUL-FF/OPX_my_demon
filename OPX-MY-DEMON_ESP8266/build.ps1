param(
    [switch]$Flash,
    [string]$Port,
    [switch]$Clean,
    [switch]$Monitor,
    [switch]$Restore,
    [switch]$ListPorts,
    [switch]$NoPatch,
    [switch]$Verbose,
    [switch]$LittleFS,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

# ---- Config ----
$ProjectRoot = Resolve-Path "$PSScriptRoot"
$CoreDir = "$env:LOCALAPPDATA\Arduino15\packages\esp8266\hardware\esp8266\3.1.2\cores\esp8266"
$PatchesDir = "$ProjectRoot\patches"
$BackupDir = "$PatchesDir\backup"
$FirmwareDir = "$ProjectRoot"
$FQBN = "esp8266:esp8266:nodemcuv2:eesz=4M3M,xtal=160,mmu=4816,dbg=Disabled,lvl=None____,ip=lm2f,vt=flash,exception=disabled,stacksmash=disabled,ssl=all,non32xfer=fast"
$InoFile = "$ProjectRoot\OPX-MY-DEMON_ESP8266.ino"
$BinName = "OPX-MY-DEMON_ESP8266_v1.0.2.bin"
$MemLogFile = "$ProjectRoot\..\.build-memory.log"
$LibsNeeded = @("ESP AsyncWebServer", "ESPAsyncTCP", "ArduinoJson")

# ---- Help ----
if ($Help) {
    Write-Host @"

 OPX-MY-DEMON Build Script v2.0

 USAGE:
   .\build.ps1 [options]

 OPTIONS:
   -Flash       Compile + flash to ESP8266
   -Port COM3   Specify COM port (auto-detect if omitted)
   -Clean       Clean cached build artifacts
   -Monitor     Launch serial monitor after flash
   -Restore     Restore original ESP8266 core files
   -ListPorts   Show available COM ports
   -NoPatch     Skip core patching (IRAM_ATTR removal)
   -Verbose     Show full compile output
   -LittleFS    Upload LittleFS data files
   -Help        Show this help

 EXAMPLES:
   .\build.ps1                         Compile only
   .\build.ps1 -Flash                  Compile + flash (auto port)
   .\build.ps1 -Flash -Port COM5       Compile + flash on COM5
   .\build.ps1 -Flash -Monitor         Compile + flash + serial
   .\build.ps1 -Clean -Flash           Full clean build + flash
   .\build.ps1 -Restore                Restore original core files
   .\build.ps1 -ListPorts              Show ports

"@
    exit 0
}

$script:startTime = Get-Date

function Write-Step($m) { Write-Host "==> $m" -ForegroundColor Cyan }
function Write-Ok($m)   { Write-Host "  [+] $m" -ForegroundColor Green }
function Write-Warn($m) { Write-Host "  [!] $m" -ForegroundColor Yellow }
function Write-Err($m)  { Write-Host "  [X] $m" -ForegroundColor Red }

function Write-Banner {
    Write-Host "`n  ___ ___ ___ ___ ___ ___ ___ ___" -ForegroundColor DarkCyan
    Write-Host "  | _ \| _ \| _ \|_ _/ __|_ _| _ \" -ForegroundColor DarkCyan
    Write-Host "  |  _/|   /|   / | |\__ \| ||  _/" -ForegroundColor DarkCyan
    Write-Host "  |_|  |_|_\|_|_\|___|___/___|_|" -ForegroundColor DarkCyan
    Write-Host "  OPX-MY-DEMON Build Script v2.0`n" -ForegroundColor DarkCyan
}

function Run-Cmd([string]$cmd) {
    $tmp = [System.IO.Path]::GetTempFileName()
    cmd /c $cmd 2>$tmp
    $exitCode = $LASTEXITCODE
    $stderr = Get-Content $tmp -ErrorAction SilentlyContinue
    Remove-Item $tmp -ErrorAction SilentlyContinue
    return @{ ExitCode = $exitCode; Stderr = $stderr }
}

function Run-CmdCapture([string]$cmd) {
    $tmpOut = [System.IO.Path]::GetTempFileName()
    $tmpErr = [System.IO.Path]::GetTempFileName()
    cmd /c "$cmd >`"$tmpOut`" 2>&1"
    $exitCode = $LASTEXITCODE
    $output = Get-Content $tmpOut -ErrorAction SilentlyContinue
    Remove-Item $tmpOut, $tmpErr -ErrorAction SilentlyContinue
    return @{ ExitCode = $exitCode; Output = $output }
}

# ---- Dependency Check ----
function Check-Deps {
    Write-Step "Checking dependencies..."
    $ok = $true

    $cli = Get-Command "arduino-cli" -ErrorAction SilentlyContinue
    if (-not $cli) {
        Write-Err "arduino-cli not found. Install: winget install Arduino.ArduinoCLI"
        $ok = $false
    } else {
        Write-Ok "arduino-cli: $($cli.Source)"
    }

    $r = Run-CmdCapture "arduino-cli core list"
    $allOut = $r.Output -join "`n"
    if ($allOut -match "esp8266") {
        Write-Ok "ESP8266 core installed"
    } else {
        Write-Warn "ESP8266 core not found. Run: arduino-cli core update-index; arduino-cli core install esp8266:esp8266"
    }

    $libList = Run-CmdCapture "arduino-cli lib list"
    $libOut = $libList.Output -join "`n"
    foreach ($lib in $LibsNeeded) {
        if ($libOut -match [regex]::Escape($lib)) {
            Write-Ok "Library: $lib"
        } else {
            Write-Warn "Library missing: $lib"
        }
    }

    if (-not $ok) { exit 1 }
}

# ---- List Ports ----
function Show-Ports {
    Write-Step "Available COM ports:"
    $r = Run-Cmd "arduino-cli board list"
    if ($r.Stderr.Count -eq 0 -or ($r.Stderr -match "No boards")) {
        Write-Warn "No boards found"
    } else {
        $r.Stderr | ForEach-Object { Write-Host "  $_" }
    }
}

# ---- Detect Port ----
function Get-Port {
    $r = Run-Cmd "arduino-cli board list"
    foreach ($line in $r.Stderr) {
        if ($line -match "^(COM\d+)") { return $matches[1] }
    }
    return $null
}

# ---- Patch Core ----
function Invoke-Patch {
    if ($NoPatch) { Write-Step "Skipping core patches (-NoPatch)"; return }

    Write-Step "Patching ESP8266 core files (IRAM_ATTR removed)..."
    if (-not (Test-Path "$BackupDir\gdb_hooks.cpp")) {
        Write-Step "First run - backing up originals to patches\backup\"
        New-Item -ItemType Directory -Force -Path $BackupDir | Out-Null
        Copy-Item "$CoreDir\core_esp8266_waveform_pwm.cpp" "$BackupDir\" -Force
        Copy-Item "$CoreDir\gdb_hooks.cpp" "$BackupDir\" -Force
        Write-Ok "Originals backed up"
    }

    Copy-Item "$PatchesDir\core_esp8266_waveform_pwm.cpp" "$CoreDir\" -Force
    Copy-Item "$PatchesDir\gdb_hooks.cpp" "$CoreDir\" -Force
    Write-Ok "Core patches applied"
}

# ---- Restore Core ----
function Invoke-Restore {
    Write-Step "Restoring original ESP8266 core files..."
    if (Test-Path "$BackupDir\core_esp8266_waveform_pwm.cpp") {
        Copy-Item "$BackupDir\core_esp8266_waveform_pwm.cpp" "$CoreDir\" -Force
        Copy-Item "$BackupDir\gdb_hooks.cpp" "$CoreDir\" -Force
        Write-Ok "Original core files restored"
    } else {
        Write-Warn "No backups found in patches\backup\"
    }
    exit 0
}

# ---- Clean Cache ----
function Invoke-Clean {
    Write-Step "Cleaning cached build artifacts..."
    Remove-Item -Recurse -Force "$env:LOCALAPPDATA\arduino\cores" -ErrorAction SilentlyContinue
    Write-Ok "Build cache cleared"
}

# ---- Compile ----
function Invoke-Compile {
    Write-Step "Compiling firmware..."
    $escFqbn = $FQBN -replace '"', '\"'
    $r = Run-CmdCapture "arduino-cli compile --fqbn `"$escFqbn`" --output-dir `"$ProjectRoot`" `"$InoFile`""

    if ($r.ExitCode -ne 0) {
        Write-Host ""
        Write-Err "BUILD FAILED (exit code: $($r.ExitCode))"
        Write-Host ""
        $r.Output | ForEach-Object { Write-Host "  $_" }
        Write-Host ""
        exit 1
    }

    Write-Ok "Compile successful"
    return $r.Output
}

# ---- Copy Binary ----
function Copy-Binary {
    Write-Step "Copying binary..."
    $srcBin = Get-ChildItem -Path "$FirmwareDir" -Filter "*.bin" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($srcBin) {
        $dst = "$ProjectRoot\$BinName"
        Copy-Item $srcBin.FullName $dst -Force
        $size = [math]::Round((Get-Item $dst).Length / 1024)
        Write-Ok "Binary: $BinName ($size KB)"
    }
}

# ---- Log Memory ----
function Log-Memory($lines) {
    $memLine = $lines | Select-String "Instruction RAM"
    if ($memLine) {
        $prev = ""
        if (Test-Path $MemLogFile) { $prev = Get-Content $MemLogFile -Tail 1 }
        $curr = $memLine.ToString().Trim()
        Add-Content -Path $MemLogFile -Value "$(Get-Date -Format 'yyyy-MM-dd HH:mm') | $curr"
        if ($prev -and ($prev -ne $curr) -and $Verbose) {
            Write-Warn "IRAM changed from previous build!"
        }
    }
}

# ---- Flash ----
function Invoke-Flash($port) {
    if (-not $port) {
        $port = Get-Port
        if (-not $port) {
            Write-Err "No ESP8266 detected. Use -Port COMx or -ListPorts"
            Show-Ports
            exit 1
        }
        Write-Ok "Auto-detected port: $port"
    }

    Write-Step "Flashing firmware to $port ..."
    $r = Run-Cmd "arduino-cli upload --fqbn esp8266:esp8266:nodemcuv2 --port $port --input-dir `"$FirmwareDir`""
    if ($r.ExitCode -eq 0) {
        Write-Ok "Flash successful on $port"
    } else {
        Write-Err "Flash failed on $port"
        $r.Stderr | ForEach-Object { Write-Host "  $_" }
        exit 1
    }
}

# ---- LittleFS Upload ----
function Invoke-LittleFS($port) {
    if (-not $port) {
        $port = Get-Port
        if (-not $port) { Write-Err "No board detected"; exit 1 }
    }

    $dataDir = "$ProjectRoot\data"
    if (-not (Test-Path $dataDir)) {
        Write-Warn "No data\ directory found"
        return
    }

    Write-Step "Uploading LittleFS data to $port ..."
    $r = Run-Cmd "arduino-cli upload --fqbn esp8266:esp8266:nodemcuv2 --port $port --input-dir `"$dataDir`""
    if ($r.ExitCode -eq 0) {
        Write-Ok "LittleFS upload successful"
    } else {
        Write-Err "LittleFS upload failed"
        $r.Stderr | ForEach-Object { Write-Host "  $_" }
    }
}

# ---- Serial Monitor ----
function Invoke-Monitor($port) {
    if (-not $port) {
        $port = Get-Port
        if (-not $port) { Write-Err "No board detected"; exit 1 }
    }
    Write-Step "Opening serial monitor on $port (115200 baud)...`n"
    cmd /c "arduino-cli monitor -p $port --config baudrate=115200"
}

# ---- Show Summary ----
function Show-Summary($lines) {
    Write-Host "`n===========================================" -ForegroundColor DarkCyan
    $time = [math]::Round(((Get-Date) - $script:startTime).TotalSeconds, 1)
    Write-Host " Build complete in ${time}s" -ForegroundColor Cyan
    Write-Host "===========================================" -ForegroundColor DarkCyan

    $lines | ForEach-Object {
        if ($_ -match "RAM|Flash|IRAM|Variables|used") { Write-Host "  $_" -ForegroundColor Gray }
    }
    Write-Host ""
}

# ============================================================
# ---- MAIN ----
# ============================================================
Write-Banner
if ($ListPorts) { Show-Ports; exit 0 }

Check-Deps
Invoke-Patch
if ($Restore) { Invoke-Restore }
if ($Clean) { Invoke-Clean }

$output = Invoke-Compile
if ($Verbose) { $output | ForEach-Object { Write-Host "  $_" } }

Copy-Binary
Log-Memory $output
Show-Summary $output

if ($Flash) { Invoke-Flash $Port }
if ($LittleFS) { Invoke-LittleFS $Port }
if ($Monitor) { Invoke-Monitor $Port }

Write-Ok "All done!"
