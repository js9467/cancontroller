<#
.SYNOPSIS
Bronco Controls - One-Click ESP32 Flasher

.DESCRIPTION
Downloads the latest firmware and flashes your ESP32-S3-Box device.
No extraction needed - just run this script!

.PARAMETER Port
Specify COM port manually (e.g., COM3). Auto-detects if not provided.

.PARAMETER ListPorts
List available serial ports and exit.

.PARAMETER OfflineMode
Use cached files instead of downloading fresh firmware.

.EXAMPLE
.\BroncoFlasher.ps1
Auto-detects device and flashes latest firmware

.EXAMPLE
.\BroncoFlasher.ps1 -Port COM3
Flash using specific COM port

.EXAMPLE
.\BroncoFlasher.ps1 -ListPorts
Show available serial ports
#>

[CmdletBinding()]
param(
    [string]$Port,
    [switch]$ListPorts,
    [switch]$OfflineMode,
    [ValidateSet('4.3','7.0')]
    [string]$PanelVariant = '4.3',
    [string]$GitHubRepo = "js9467/autotouchscreen",
    [string]$GitHubBranch = "main",
    [string]$OtaServer = "https://image-optimizer-still-flower-1282.fly.dev",
    [string]$EsptoolVersion = "v4.7.0"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = 'SilentlyContinue'  # Faster downloads

# Colors
function Write-Header { param([string]$Message) Write-Host "`n[BRONCO] $Message" -ForegroundColor Cyan }
function Write-Success { param([string]$Message) Write-Host "  [OK] $Message" -ForegroundColor Green }
function Write-Step { param([string]$Message) Write-Host "  --> $Message" -ForegroundColor Yellow }
function Write-ErrorMsg { param([string]$Message) Write-Host "  [ERROR] $Message" -ForegroundColor Red }

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

# Setup working directory
$WorkDir = Join-Path $env:LOCALAPPDATA "BroncoControls\flash-temp"
if (-not (Test-Path $WorkDir)) {
    New-Item -ItemType Directory -Path $WorkDir -Force | Out-Null
}

Write-Header "Bronco Controls ESP32 Flasher"
Write-Host "  Workspace: $WorkDir`n" -ForegroundColor DarkGray

# Detect serial ports
function Get-SerialPorts {
    $ports = @()

    try {
        $ports = @(Get-CimInstance -ClassName Win32_SerialPort -ErrorAction Stop |
            Select-Object DeviceID, Description |
            Sort-Object DeviceID)
    } catch {
        $ports = @()
    }

    if ($ports.Count -gt 0) {
        return $ports
    }

    try {
        $pnpPorts = @(Get-PnpDevice -Class Ports -ErrorAction Stop |
            Where-Object { $_.FriendlyName -match '\(COM\d+\)' } |
            ForEach-Object {
                [PSCustomObject]@{
                    DeviceID    = ($_.FriendlyName -replace '.*\((COM\d+)\).*', '$1')
                    Description = $_.FriendlyName
                }
            } |
            Sort-Object DeviceID)

        if ($pnpPorts.Count -gt 0) {
            return $pnpPorts
        }
    } catch {
        # Fall through to final fallback
    }

    $rawPorts = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object
    return @($rawPorts | ForEach-Object {
        [PSCustomObject]@{
            DeviceID    = $_
            Description = "Serial Port ($_)"
        }
    })
}

function Detect-ESP32Port {
    param([array]$Ports)
    if ($Ports.Count -eq 0) { return $null }
    
    $patterns = @('CP210', 'Silicon Labs', 'USB Serial', 'USB JTAG', 'ESP32', 'CH910', 'CH343')
    foreach ($pattern in $patterns) {
        $match = $Ports | Where-Object { ([string]$_.Description) -like "*$pattern*" }
        if ($match) { return $match[0].DeviceID }
    }
    return $Ports[0].DeviceID
}

function Resolve-PackagedFile {
    param([string]$Filename)
    $candidate = Join-Path $ScriptRoot $Filename
    if (Test-Path $candidate) {
        return $candidate
    }
    return $null
}

function Get-PackagedFirmwarePath {
    param([string]$Variant)
    $variantMap = @{
        '4.3' = 'firmware.bin'
        '7.0' = 'firmware-7.0.bin'
    }

    if (-not $variantMap.ContainsKey($Variant)) {
        return $null
    }

    return Resolve-PackagedFile -Filename $variantMap[$Variant]
}

$ports = @(Get-SerialPorts)
if ($ListPorts) {
    Write-Header "Available Serial Ports"
    if ($ports.Count -eq 0) {
        Write-Host "  No serial ports found`n"
    } else {
        $ports | Format-Table -AutoSize
    }
    exit 0
}

# Download esptool
function Get-Esptool {
    param([string]$Version)
    $esptoolDir = Join-Path $env:LOCALAPPDATA "BroncoControls\esptool-$Version"
    $esptoolExe = Join-Path $esptoolDir "esptool.exe"
    
    if (Test-Path $esptoolExe) {
        return $esptoolExe
    }
    
    # Check for any cached esptool version
    $cachedEsptool = Get-ChildItem -Path "$env:LOCALAPPDATA\BroncoControls" -Filter "esptool-*" -Directory -ErrorAction SilentlyContinue | 
        ForEach-Object { Get-ChildItem -Path $_.FullName -Filter "esptool.exe" -Recurse -ErrorAction SilentlyContinue } | 
        Select-Object -First 1
    
    Write-Step "Downloading esptool $Version..."
    $downloadUrl = "https://github.com/espressif/esptool/releases/download/$Version/esptool-$Version-win64.zip"
    $tempZip = Join-Path $env:TEMP "esptool-$([guid]::NewGuid()).zip"
    
    $maxRetries = 3
    for ($i = 1; $i -le $maxRetries; $i++) {
        try {
            if ($i -gt 1) {
                Write-Step "Retry $i of $maxRetries..."
                Start-Sleep -Seconds 2
            }
            
            Invoke-WebRequest -Uri $downloadUrl -OutFile $tempZip -UseBasicParsing -TimeoutSec 30
            
            if (Test-Path $esptoolDir) {
                Remove-Item $esptoolDir -Recurse -Force
            }
            New-Item -ItemType Directory -Path $esptoolDir -Force | Out-Null
            Expand-Archive -Path $tempZip -DestinationPath $esptoolDir -Force
            Remove-Item $tempZip -Force
            
            $tool = Get-ChildItem -Path $esptoolDir -Filter esptool.exe -Recurse | Select-Object -First 1
            if (-not $tool) {
                throw "esptool.exe not found after extraction"
            }
            
            Write-Success "esptool downloaded"
            return $tool.FullName
            
        } catch {
            if ($i -eq $maxRetries) {
                if ($cachedEsptool) {
                    Write-Step "Using cached esptool (download failed, but cache available)"
                    return $cachedEsptool.FullName
                }
                throw "Failed to download esptool after $maxRetries attempts and no cache available: $_"
            }
        }
    }
}

# Download static binaries from GitHub
function Get-StaticBinary {
    param([string]$Filename)
    
    $localPath = Join-Path $WorkDir $Filename
    $packaged = Resolve-PackagedFile -Filename $Filename
    if ($packaged) {
        Write-Step "Using packaged $Filename"
        Copy-Item -Path $packaged -Destination $localPath -Force
        return $localPath
    }
    
    $cacheAge = if (Test-Path $localPath) { (Get-Date) - (Get-Item $localPath).LastWriteTime } else { $null }
    
    # Use cached file if less than 7 days old or in offline mode
    if ($OfflineMode -and (Test-Path $localPath)) {
        Write-Step "Using cached $Filename"
        return $localPath
    }
    
    if ($cacheAge -and $cacheAge.TotalDays -lt 7) {
        Write-Step "Using cached $Filename ($(($cacheAge.TotalDays).ToString('0.0')) days old)"
        return $localPath
    }
    
    Write-Step "Downloading $Filename from GitHub..."
    $url = "https://raw.githubusercontent.com/$GitHubRepo/$GitHubBranch/tools/deploy/$Filename"
    
    try {
        Invoke-WebRequest -Uri $url -OutFile $localPath -UseBasicParsing
        Write-Success "$Filename downloaded"
        return $localPath
    } catch {
        if (Test-Path $localPath) {
            Write-Step "Using cached $Filename (download failed, but cache available)"
            return $localPath
        }
        throw "Failed to download $Filename and no cache available: $_"
    }
}

# Download latest firmware from OTA server
function Get-LatestFirmware {
    $firmwarePath = Join-Path $WorkDir "firmware.bin"
    $versionPath = Join-Path $WorkDir "firmware_version.txt"
    $packagedFirmware = Get-PackagedFirmwarePath -Variant $PanelVariant

    if ($packagedFirmware) {
        Write-Step "Using packaged firmware for panel $PanelVariant";
        Copy-Item -Path $packagedFirmware -Destination $firmwarePath -Force
        @"
Firmware Source: Packaged
Panel Variant: $PanelVariant
Copied: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
"@ | Set-Content -Path $versionPath
        return $firmwarePath
    }

    Write-Step "Packaged firmware not found for panel $PanelVariant; falling back to OTA download"
    
    if ($OfflineMode -and (Test-Path $firmwarePath)) {
        Write-Step "Using cached firmware (offline mode)"
        return $firmwarePath
    }
    
    Write-Step "Fetching latest firmware from OTA server..."
    
    # Force download flag - delete cached firmware
    $forceDownload = $env:BRONCO_FORCE_DOWNLOAD -eq 'true'
    if ($forceDownload) {
        if (Test-Path $firmwarePath) {
            Remove-Item $firmwarePath -Force
            Write-Step "Cleared cached firmware (force download enabled)"
        }
        if (Test-Path $versionPath) {
            Remove-Item $versionPath -Force
        }
    }
    
    try {
        $manifest = Invoke-RestMethod -Uri "$OtaServer/ota/manifest" -Method Get
        $version = $manifest.version
        $firmwareUrl = $manifest.firmware.url
        $expectedMd5 = $manifest.firmware.md5.ToLower()
        
        # Check if we have the latest version cached
        $needsDownload = $true
        
        if ((Test-Path $versionPath)) {
            $cachedInfo = Get-Content $versionPath -Raw
            if ($cachedInfo -match "Firmware Version: (.+)") {
                $cachedVersion = $matches[1]
                if ($cachedVersion -eq $version -and (Test-Path $firmwarePath)) {
                    Write-Step "Latest firmware v$version already cached"
                    $needsDownload = $false
                }
            }
        }
        
        if ($needsDownload) {
            Write-Header "Latest Firmware: v$version"
            Write-Step "Downloading firmware.bin ($(($manifest.firmware.size / 1MB).ToString('0.0')) MB)..."
            
            Invoke-WebRequest -Uri $firmwareUrl -OutFile $firmwarePath -UseBasicParsing
            
            # Verify MD5
            $actualMd5 = (Get-FileHash -Path $firmwarePath -Algorithm MD5).Hash.ToLower()
            if ($actualMd5 -ne $expectedMd5) {
                throw "MD5 mismatch! Expected: $expectedMd5, Got: $actualMd5"
            }
            
            # Save version info
            @"
Firmware Version: $version
Downloaded: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Source: $OtaServer
MD5: $expectedMd5
Size: $($manifest.firmware.size) bytes
"@ | Set-Content -Path $versionPath
            
            Write-Success "Firmware v$version downloaded and verified"
        }
        
        return $firmwarePath
        
    } catch {
        if (Test-Path $firmwarePath) {
            Write-Step "Using cached firmware (download failed, but cache available)"
            return $firmwarePath
        }
        throw "Failed to download firmware and no cache available: $_"
    }
}

function Invoke-EsptoolCommand {
    param(
        [string]$ToolPath,
        [string[]]$CommandArgs,
        [string]$Action,
        [bool]$AllowManualRetry = $false,
        [string[]]$ManualCommandArgs,
        [bool]$ManualOnly = $false
    )

    $attempts = 1
    if ($AllowManualRetry) {
        $attempts = 2
    }

    for ($i = 1; $i -le $attempts; $i++) {
        $useManual = $ManualOnly -or ($i -gt 1 -and $AllowManualRetry -and $ManualCommandArgs)

        if ($useManual) {
            Write-Host "" 
            Write-Step "Manual boot mode: Hold BOOT and tap RESET, keep holding BOOT"
            Write-Host "  1. Hold BOOT now (keep holding it)." -ForegroundColor Yellow
            Write-Host "  2. Tap RESET once while still holding BOOT." -ForegroundColor Yellow
            Write-Host "  3. Keep holding BOOT and press Enter to retry $Action." -ForegroundColor Yellow
            Write-Host "  4. Release BOOT only after 'Connecting...' appears." -ForegroundColor Yellow
            $null = Read-Host
        }

        $argsToUse = $CommandArgs
        if ($useManual -and $ManualCommandArgs) {
            $argsToUse = $ManualCommandArgs
        }

        & $ToolPath @argsToUse

        if ($LASTEXITCODE -eq 0) {
            return
        }
    }

    throw "$Action failed with exit code $LASTEXITCODE"
}

# Main execution
try {
    Write-Header "Preparing Flash Files"
    Write-Step "Panel variant: $PanelVariant"
    
    # Download all required files
    $bootloader = Get-StaticBinary "bootloader.bin"
    $partitions = Get-StaticBinary "partitions.bin"
    $bootApp0 = Get-StaticBinary "boot_app0.bin"
    $firmware = Get-LatestFirmware
    $esptool = Get-Esptool -Version $EsptoolVersion
    
    Write-Header "Detecting ESP32 Device"
    
    if (-not $Port) {
        $Port = Detect-ESP32Port -Ports $ports
    }
    
    if (-not $Port) {
        # Check if ESP32 USB device is connected but drivers are missing
        $esp32Device = Get-PnpDevice | Where-Object { 
            ($_.FriendlyName -like '*USB JTAG*' -or $_.FriendlyName -like '*303A*' -or $_.FriendlyName -like '*Composite*') -and 
            $_.Status -eq 'Unknown' -and
            $_.InstanceId -like '*303A*'
        }
        
        if ($esp32Device) {
            Write-Header "ESP32 Device Detected - Installing Drivers"
            Write-Host "  ESP32-S3 found but needs USB drivers..." -ForegroundColor Yellow
            Write-Host "  Installing official Espressif drivers...`n" -ForegroundColor Yellow
            
            try {
                Write-Step "Downloading official ESP32 USB drivers..."
                $espDriverUrl = "https://dl.espressif.com/dl/idf-driver/idf-driver-esp32-usb-jtag-2021-07-15.zip"
                $driverZip = Join-Path $WorkDir "esp-driver.zip"
                
                Invoke-WebRequest -Uri $espDriverUrl -OutFile $driverZip -UseBasicParsing
                Write-Success "Driver package downloaded"
                
                $driverExtractDir = Join-Path $WorkDir "esp-driver"
                if (Test-Path $driverExtractDir) {
                    Remove-Item $driverExtractDir -Recurse -Force
                }
                Expand-Archive -Path $driverZip -DestinationPath $driverExtractDir -Force
                
                $driverInstaller = Get-ChildItem -Path $driverExtractDir -Filter "*.exe" -Recurse | Select-Object -First 1
                
                if ($driverInstaller) {
                    Write-Step "Running official ESP32 driver installer..."
                    Write-Host "  The installer window will open - click through the prompts" -ForegroundColor Cyan
                    Write-Host "  Click 'Install' or 'Next' when prompted`n" -ForegroundColor Cyan
                    
                    Start-Process -FilePath $driverInstaller.FullName -Wait
                    Write-Success "Driver installer completed"
                    
                    Write-Host ""
                    Write-Host "============================================================" -ForegroundColor Yellow
                    Write-Host "  IMPORTANT: Unplug and Replug Your ESP32 Device" -ForegroundColor Yellow
                    Write-Host "============================================================" -ForegroundColor Yellow
                    Write-Host "`n  Steps:" -ForegroundColor Cyan
                    Write-Host "  1. UNPLUG the USB cable from your computer" -ForegroundColor White
                    Write-Host "  2. Wait 5 seconds" -ForegroundColor White
                    Write-Host "  3. PLUG it back in" -ForegroundColor White
                    Write-Host "  4. Wait another 5 seconds for Windows to detect it`n" -ForegroundColor White
                    
                    Write-Host "  Press any key after you've done this..." -ForegroundColor Cyan
                    $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
                    
                    Write-Step "Waiting for Windows to recognize device..."
                    Start-Sleep -Seconds 5
                    
                    # Refresh port list
                    $ports = @(Get-SerialPorts)
                    $Port = Detect-ESP32Port -Ports $ports
                    
                    if ($Port) {
                        Write-Success "ESP32 detected on $Port!"
                    } else {
                        Write-Warning "Device not detected yet"
                        Write-Host "`n  Let's try one more time..." -ForegroundColor Yellow
                        Write-Host "  Make absolutely sure:" -ForegroundColor Cyan
                        Write-Host "    • The USB cable is DATA cable (not charge-only)" -ForegroundColor White
                        Write-Host "    • Device screen is lit up" -ForegroundColor White
                        Write-Host "    • You've waited at least 10 seconds after replugging`n" -ForegroundColor White
                        
                        Write-Host "  Unplug, wait 10 seconds, replug, wait 10 seconds..." -ForegroundColor Yellow
                        Write-Host "  Press any key after doing this..." -ForegroundColor Cyan
                        $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
                        
                        Start-Sleep -Seconds 10
                        $ports = @(Get-SerialPorts)
                        $Port = Detect-ESP32Port -Ports $ports
                        
                        if ($Port) {
                            Write-Success "ESP32 detected on $Port!"
                        } else {
                            # Show what Windows sees
                            Write-Host "`nLet's see what Windows detects:" -ForegroundColor Yellow
                            Get-PnpDevice | Where-Object { $_.InstanceId -like '*303A*' -or $_.InstanceId -like '*USB*' } | 
                                Select-Object -First 10 | 
                                Format-Table FriendlyName, Status, Class -AutoSize
                        }
                    }
                } else {
                    throw "Could not find driver installer in package"
                }
                
            } catch {
                Write-ErrorMsg "Driver installation failed: $_"
                Write-Host "`nManual driver installation steps:" -ForegroundColor Yellow
                Write-Host "  1. Open Device Manager (Win+X, then M)" -ForegroundColor Cyan
                Write-Host "  2. Find device with yellow warning (likely under 'Other devices')" -ForegroundColor Cyan
                Write-Host "  3. Right-click → Update Driver" -ForegroundColor Cyan
                Write-Host "  4. Choose 'Browse my computer for drivers'" -ForegroundColor Cyan
                Write-Host "  5. Click 'Let me pick from a list'" -ForegroundColor Cyan
                Write-Host "  6. Select 'Ports (COM and LPT)'" -ForegroundColor Cyan
                Write-Host "  7. Choose 'USB Serial Device' or any COM port driver`n" -ForegroundColor Cyan
            }
        }
        
        # Final check
        if (-not $Port) {
            Write-ErrorMsg "Could not auto-detect ESP32 COM port"
            
            # Check if any USB device with ESP32 vendor ID is present
            $anyESP32Device = Get-PnpDevice | Where-Object { 
                $_.InstanceId -like '*303A*'
            }
            
            if ($anyESP32Device) {
                Write-Host "`nESP32 device detected but no COM port found." -ForegroundColor Yellow
                Write-Host "This usually means the USB driver is not installed correctly.`n" -ForegroundColor Yellow
                
                Write-Host "Quick fix options:" -ForegroundColor Cyan
                Write-Host "  1. Unplug and replug the device" -ForegroundColor White
                Write-Host "  2. Try a different USB port (USB 2.0 recommended)" -ForegroundColor White
                Write-Host "  3. Try a different USB cable" -ForegroundColor White
                Write-Host "`nAfter trying above, press R to retry detection" -ForegroundColor Yellow
                Write-Host "Or press any other key to exit`n" -ForegroundColor Yellow
                
                $key = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
                if ($key.Character -eq 'r' -or $key.Character -eq 'R') {
                    Write-Step "Retrying detection..."
                    Start-Sleep -Seconds 2
                    $ports = @(Get-SerialPorts)
                    $Port = Detect-ESP32Port -Ports $ports
                    
                    if ($Port) {
                        Write-Success "ESP32 detected on $Port!"
                    } else {
                        Write-Host "`nStill cannot detect. Please run Install-Drivers.bat first.`n" -ForegroundColor Red
                        exit 1
                    }
                } else {
                    exit 1
                }
            } else {
                Write-Host "`nAvailable ports:" -ForegroundColor Yellow
                if ($ports.Count -eq 0) {
                    Write-Host "  No serial ports found" -ForegroundColor Red
                    Write-Host "`nTroubleshooting:" -ForegroundColor Yellow
                    Write-Host "  1. Make sure ESP32 is connected via USB"
                    Write-Host "  2. Unplug and replug the device"
                    Write-Host "  3. Try a different USB cable or port"
                    Write-Host "  4. Run Install-Drivers.bat to install USB drivers`n"
                } else {
                    $ports | Format-Table -AutoSize
                    Write-Host "Run with -Port COMx to specify manually`n" -ForegroundColor Yellow
                }
                exit 1
            }
        }
    }
    
    Write-Success "Using port: $Port"

    $selectedPort = $ports | Where-Object { $_.DeviceID -eq $Port } | Select-Object -First 1
    if ($selectedPort) {
        $requiresManualBoot = ([string]$selectedPort.Description) -match 'CH34\d|CH910'
        if ($requiresManualBoot) {
            Write-Host "  Detected CH340/CH343 style USB bridge - manual boot assist may be required." -ForegroundColor Yellow
            Write-Host "  Keep the BOOT button handy; you'll be prompted if we need your help." -ForegroundColor Yellow
        } else {
            $requiresManualBoot = $false
        }
    } else {
        # Non-COM or USB-JTAG mode (e.g. using esptool --port usb) - no manual boot handling
        $requiresManualBoot = $false
    }
    
    Write-Header "Erasing Flash"
    Write-Host "  This ensures a clean installation...`n" -ForegroundColor DarkGray
    
    $eraseArgs = @(
        "--chip", "esp32s3",
        "--port", $Port,
        "--baud", "921600",
        "erase_flash"
    )
    $eraseManualArgs = @(
        "--chip", "esp32s3",
        "--port", $Port,
        "--baud", "921600",
        "--before", "no_reset",
        "erase_flash"
    )
    
    Invoke-EsptoolCommand -ToolPath $esptool -CommandArgs $eraseArgs -Action "Flash erase" -AllowManualRetry:$requiresManualBoot -ManualCommandArgs $eraseManualArgs -ManualOnly:$requiresManualBoot
    
    Write-Header "Flashing ESP32"
    Write-Host "  This may take 30-60 seconds...`n" -ForegroundColor DarkGray
    
    $arguments = @(
        "--chip", "esp32s3",
        "--port", $Port,
        "--baud", "921600",
        "--before", "default_reset",
        "--after", "hard_reset",
        "write_flash",
        "--flash_mode", "qio",
        "--flash_size", "16MB",
        "0x0", $bootloader,
        "0x8000", $partitions,
        "0xe000", $bootApp0,
        "0x10000", $firmware
    )
    $manualFlashArgs = @(
        "--chip", "esp32s3",
        "--port", $Port,
        "--baud", "921600",
        "--before", "no_reset",
        "--after", "hard_reset",
        "write_flash",
        "--flash_mode", "qio",
        "--flash_size", "16MB",
        "0x0", $bootloader,
        "0x8000", $partitions,
        "0xe000", $bootApp0,
        "0x10000", $firmware
    )
    
    Invoke-EsptoolCommand -ToolPath $esptool -CommandArgs $arguments -Action "Firmware flash" -AllowManualRetry:$requiresManualBoot -ManualCommandArgs $manualFlashArgs -ManualOnly:$requiresManualBoot
    
    Write-Host ""
    Write-Success "Flash complete!"
    
    # Monitor serial for IP address and open browser
    Write-Header "Monitoring Device Startup"
    Write-Step "Waiting for device to boot and connect to WiFi..."
    Write-Host "  (This will timeout after 30 seconds if no WiFi is configured)`n" -ForegroundColor DarkGray
    
    try {
        Start-Sleep -Seconds 2  # Wait for device to reset
        
        $serialPort = New-Object System.IO.Ports.SerialPort $Port, 115200
        $serialPort.Open()
        
        $timeout = 30
        $startTime = Get-Date
        $ipAddress = $null
        
        while (((Get-Date) - $startTime).TotalSeconds -lt $timeout) {
            if ($serialPort.BytesToRead -gt 0) {
                $line = $serialPort.ReadLine()
                Write-Host "  $line" -ForegroundColor DarkGray
                
                # Look for station IP (WiFi connected)
                if ($line -match 'Station connected:\s*(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})') {
                    $ipAddress = $matches[1]
                    break
                }
                # Fallback to AP IP if no station connection
                if (!$ipAddress -and $line -match 'AP ready at\s*(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})') {
                    $ipAddress = $matches[1]
                }
            }
            Start-Sleep -Milliseconds 100
        }
        
        $serialPort.Close()
        
        if ($ipAddress) {
            Write-Success "Device connected! IP: $ipAddress"
            Write-Host "`n  Opening web interface in browser..." -ForegroundColor Green
            Start-Process "http://$ipAddress"
            Write-Host "`n  Web interface: http://$ipAddress" -ForegroundColor Cyan
        } else {
            Write-Host "`n  No WiFi connection detected (timeout)" -ForegroundColor Yellow
            Write-Host "  Configure WiFi on device, then access web interface at device IP`n" -ForegroundColor Yellow
        }
        
    } catch {
        Write-Host "`n  Could not monitor serial output: $_" -ForegroundColor Yellow
        Write-Host "  Device is ready - configure WiFi to access web interface`n" -ForegroundColor Yellow
    }
    
    Write-Header "Setup Complete!"
    Write-Host "  You may now disconnect the USB cable.`n" -ForegroundColor Green
    
} catch {
    Write-Host ""
    Write-ErrorMsg "Flash failed: $_"
    Write-Host "`nTroubleshooting:" -ForegroundColor Yellow
    Write-Host "  1. Close any programs using the COM port (Arduino IDE, PuTTY, etc.)"
    Write-Host "  2. Try a different USB cable or port"
    Write-Host "  3. Press and hold BOOT button during flashing"
    Write-Host "  4. Run with -Port COM3 to specify port manually`n"
    exit 1
}
