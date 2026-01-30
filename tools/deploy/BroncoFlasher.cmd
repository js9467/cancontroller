@echo off
:: Bronco Controls - One-Click ESP32 Flasher
:: Double-click to flash your device with the latest firmware
setlocal EnableDelayedExpansion

:: Check for admin rights (not required, but helpful for some USB drivers)
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
if '%errorlevel%' NEQ '0' (
    echo.
    echo [WARNING] Not running as Administrator
    echo If flashing fails, try right-click -^> Run as Administrator
    echo.
    timeout /t 3 >nul
)

    set "SCRIPT=%~dp0BroncoFlasher.ps1"
    if exist "%SCRIPT%" (
        PowerShell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" %*
        exit /b %errorlevel%
    )

:: Run the embedded PowerShell script
PowerShell -NoProfile -ExecutionPolicy Bypass -Command "& {
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

# Colors
function Write-Header { param([string]$Message) Write-Host \"`n[BRONCO] $Message\" -ForegroundColor Cyan }
function Write-Success { param([string]$Message) Write-Host \"  [OK] $Message\" -ForegroundColor Green }
function Write-Step { param([string]$Message) Write-Host \"  --> $Message\" -ForegroundColor Yellow }
function Write-ErrorMsg { param([string]$Message) Write-Host \"  [ERROR] $Message\" -ForegroundColor Red }

$WorkDir = Join-Path $env:LOCALAPPDATA \"BroncoControls\flash-temp\"
if (-not (Test-Path $WorkDir)) { New-Item -ItemType Directory -Path $WorkDir -Force | Out-Null }

Write-Header \"Bronco Controls ESP32 Flasher\"
Write-Host \"  Workspace: $WorkDir`n\" -ForegroundColor DarkGray

# Config
$GitHubRepo = \"js9467/autotouchscreen\"
$GitHubBranch = \"main\"
$OtaServer = \"https://image-optimizer-still-flower-1282.fly.dev\"
$EsptoolVersion = \"v4.7.0\"

# Detect serial ports
function Get-SerialPorts {
    try {
        return @(Get-CimInstance Win32_SerialPort | Select-Object DeviceID, Description | Sort-Object DeviceID)
    } catch { return @() }
}

function Detect-ESP32Port {
    param([array]$Ports)
    if ($Ports.Count -eq 0) { return `$null }
    `$patterns = @('CP210', 'Silicon Labs', 'USB Serial', 'USB JTAG', 'ESP32', 'CH910')
    foreach (`$pattern in `$patterns) {
        `$match = `$Ports | Where-Object { `$_.Description -like \"*`$pattern*\" }
        if (`$match) { return `$match[0].DeviceID }
    }
    return `$Ports[0].DeviceID
}

# Download esptool
function Get-Esptool {
    param([string]$Version)
    `$esptoolDir = Join-Path `$env:LOCALAPPDATA \"BroncoControls\esptool-`$Version\"
    `$esptoolExe = Join-Path `$esptoolDir \"esptool.exe\"
    if (Test-Path `$esptoolExe) { return `$esptoolExe }
    
    Write-Step \"Downloading esptool `$Version...\"
    `$downloadUrl = \"https://github.com/espressif/esptool/releases/download/`$Version/esptool-`$Version-win64.zip\"
    `$tempZip = Join-Path `$env:TEMP \"esptool-$([guid]::NewGuid()).zip\"
    
    Invoke-WebRequest -Uri `$downloadUrl -OutFile `$tempZip -UseBasicParsing
    if (Test-Path `$esptoolDir) { Remove-Item `$esptoolDir -Recurse -Force }
    New-Item -ItemType Directory -Path `$esptoolDir -Force | Out-Null
    Expand-Archive -Path `$tempZip -DestinationPath `$esptoolDir -Force
    Remove-Item `$tempZip -Force
    
    `$tool = Get-ChildItem -Path `$esptoolDir -Filter esptool.exe -Recurse | Select-Object -First 1
    if (-not `$tool) { throw \"esptool.exe not found after extraction\" }
    Write-Success \"esptool downloaded\"
    return `$tool.FullName
}

# Download static binaries from GitHub
function Get-StaticBinary {
    param([string]$Filename)
    `$localPath = Join-Path `$WorkDir `$Filename
    `$cacheAge = if (Test-Path `$localPath) { (Get-Date) - (Get-Item `$localPath).LastWriteTime } else { `$null }
    
    if (`$cacheAge -and `$cacheAge.TotalDays -lt 7) {
        Write-Step \"Using cached `$Filename ($(((`$cacheAge.TotalDays).ToString('0.0'))) days old)\"
        return `$localPath
    }
    
    Write-Step \"Downloading `$Filename from GitHub...\"
    `$url = \"https://raw.githubusercontent.com/`$GitHubRepo/`$GitHubBranch/tools/deploy/`$Filename\"
    Invoke-WebRequest -Uri `$url -OutFile `$localPath -UseBasicParsing
    Write-Success \"`$Filename downloaded\"
    return `$localPath
}

# Download latest firmware from OTA server
function Get-LatestFirmware {
    `$firmwarePath = Join-Path `$WorkDir \"firmware.bin\"
    Write-Step \"Fetching latest firmware from OTA server...\"
    
    `$manifest = Invoke-RestMethod -Uri \"`$OtaServer/ota/manifest\" -Method Get
    `$version = `$manifest.version
    `$firmwareUrl = `$manifest.firmware.url
    `$expectedMd5 = `$manifest.firmware.md5.ToLower()
    
    Write-Header "Latest Firmware: v`$version"
    Write-Step "Downloading firmware.bin ($(((`$manifest.firmware.size / 1MB).ToString('0.0'))) MB)..."
    
    Invoke-WebRequest -Uri `$firmwareUrl -OutFile `$firmwarePath -UseBasicParsing
    
    `$actualMd5 = (Get-FileHash -Path `$firmwarePath -Algorithm MD5).Hash.ToLower()
    if (`$actualMd5 -ne `$expectedMd5) {
        throw \"MD5 mismatch! Expected: `$expectedMd5, Got: `$actualMd5\"
    }
    
    Write-Success \"Firmware v`$version verified (MD5: `$actualMd5)\"
    return `$firmwarePath
}

# Main execution
try {
    Write-Header \"Preparing Flash Files\"
    `$bootloader = Get-StaticBinary \"bootloader.bin\"
    `$partitions = Get-StaticBinary \"partitions.bin\"
    `$bootApp0 = Get-StaticBinary \"boot_app0.bin\"
    `$firmware = Get-LatestFirmware
    `$esptool = Get-Esptool -Version `$EsptoolVersion
    
    Write-Header \"Detecting ESP32 Device\"
    `$ports = @(Get-SerialPorts)
    `$Port = Detect-ESP32Port -Ports `$ports
    
    if (-not `$Port) {
        Write-ErrorMsg \"Could not auto-detect ESP32 device\"
        Write-Host \"`nAvailable ports:\" -ForegroundColor Yellow
        if (`$ports.Count -eq 0) {
            Write-Host \"  No serial ports found\" -ForegroundColor Red
            Write-Host \"`nTroubleshooting:\" -ForegroundColor Yellow
            Write-Host \"  1. Connect your ESP32 device via USB\"
            Write-Host \"  2. Install USB drivers (CP210x or CH340)\"
            Write-Host \"  3. Check Device Manager for COM ports\"
        } else {
            `$ports | Format-Table -AutoSize
        }
        Write-Host \"`nPress any key to exit...\" -ForegroundColor Yellow
        `$null = `$Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
        exit 1
    }
    
    Write-Success \"Using port: `$Port\"
    Write-Header \"Flashing ESP32\"
    Write-Host \"  This may take 30-60 seconds...`n\" -ForegroundColor DarkGray
    
    `$arguments = @(
        \"--chip\", \"esp32s3\",
        \"--port\", `$Port,
        \"--baud\", \"921600\",
        \"--before\", \"default_reset\",
        \"--after\", \"hard_reset\",
        \"write_flash\",
        \"--flash_mode\", \"qio\",
        \"--flash_size\", \"16MB\",
        \"0x0\", `$bootloader,
        \"0x8000\", `$partitions,
        \"0xe000\", `$bootApp0,
        \"0x10000\", `$firmware
    )
    
    & `$esptool @arguments
    
    if (`$LASTEXITCODE -ne 0) { throw \"esptool reported exit code `$LASTEXITCODE\" }
    
    Write-Host \"\"
    Write-Success \"Flash complete!\"
    Write-Header \"Your device is ready!\"
    Write-Host \"  You may now disconnect the USB cable.`n\" -ForegroundColor Green
    Write-Host \"Press any key to exit...\" -ForegroundColor DarkGray
    `$null = `$Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
    
} catch {
    Write-Host \"\"
    Write-ErrorMsg \"Flash failed: `$_\"
    Write-Host \"`nTroubleshooting:\" -ForegroundColor Yellow
    Write-Host \"  1. Close any programs using the COM port (Arduino IDE, PuTTY, etc.)\"
    Write-Host \"  2. Try a different USB cable or port\"
    Write-Host \"  3. Press and hold BOOT button during flashing\"
    Write-Host \"  4. Right-click this file and 'Run as Administrator'`n\"
    Write-Host \"Press any key to exit...\" -ForegroundColor Red
    `$null = `$Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
    exit 1
}
}"

if %errorlevel% neq 0 (
    echo.
    echo Script execution failed.
    pause
)
