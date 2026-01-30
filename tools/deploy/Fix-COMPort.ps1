# Force COM Port Driver Installation for ESP32-S3
# Run this as Administrator if automatic installation fails

Write-Host "`n╔════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║  ESP32-S3 COM Port Driver Fix                             ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════════╝`n" -ForegroundColor Cyan

# Check if running as admin
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "This script needs Administrator privileges." -ForegroundColor Yellow
    Write-Host "Restarting as Administrator...`n" -ForegroundColor Yellow
    Start-Process powershell -Verb RunAs -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
    exit
}

Write-Host "Running as Administrator...`n" -ForegroundColor Green

# Find ESP32 device
Write-Host "[1/3] Looking for ESP32 device..." -ForegroundColor Cyan
$esp32Device = Get-PnpDevice | Where-Object { $_.InstanceId -like '*303A*1001*' }

if (-not $esp32Device) {
    Write-Host "ERROR: ESP32 device not found!" -ForegroundColor Red
    Write-Host "Make sure the device is plugged in.`n" -ForegroundColor Yellow
    pause
    exit
}

Write-Host "Found: $($esp32Device.FriendlyName)" -ForegroundColor Green
Write-Host "Status: $($esp32Device.Status)" -ForegroundColor Yellow
Write-Host "ID: $($esp32Device.InstanceId)`n" -ForegroundColor DarkGray

# Try Method 1: Update driver using pnputil
Write-Host "[2/3] Attempting to install USB Serial driver..." -ForegroundColor Cyan

try {
    # Create temporary INF file for USB CDC driver
    $infContent = @"
[Version]
Signature="`$Windows NT`$"
Class=Ports
ClassGuid={4D36E978-E325-11CE-BFC1-08002BE10318}
Provider=%ProviderName%
DriverVer=01/21/2026,1.0.0.0

[Manufacturer]
%ProviderName%=DeviceList,NTamd64

[DeviceList.NTamd64]
%DeviceName%=DriverInstall,USB\VID_303A`&PID_1001

[DriverInstall]
Include=mdmcpq.inf
CopyFiles=FakeModemCopyFileSection
AddReg=DriverInstall.AddReg

[DriverInstall.AddReg]
HKR,,DevLoader,,*ntkern
HKR,,NTMPDriver,,usbser.sys
HKR,,EnumPropPages32,,"MsPorts.dll,SerialPortPropPageProvider"

[DriverInstall.Services]
Include=mdmcpq.inf
AddService=usbser,0x00000002,DriverService

[DriverService]
DisplayName=%ServiceName%
ServiceType=1
StartType=3
ErrorControl=1
ServiceBinary=%12%\usbser.sys
LoadOrderGroup=Base

[Strings]
ProviderName="Espressif Systems"
DeviceName="ESP32-S3 USB Serial Port"
ServiceName="USB Serial Driver"
"@

    $tempDir = Join-Path $env:TEMP "esp32-driver-fix"
    if (-not (Test-Path $tempDir)) {
        New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
    }
    
    $infPath = Join-Path $tempDir "esp32-serial.inf"
    $infContent | Set-Content -Path $infPath -Encoding ASCII
    
    Write-Host "Installing driver..." -ForegroundColor Yellow
    $result = pnputil /add-driver $infPath /install 2>&1
    Write-Host $result -ForegroundColor DarkGray
    
    # Trigger device re-enumeration
    Write-Host "`nTriggering device refresh..." -ForegroundColor Yellow
    pnputil /scan-devices 2>&1 | Out-Null
    
    Write-Host "Driver installation attempted.`n" -ForegroundColor Green
    
} catch {
    Write-Host "Automatic installation failed: $_`n" -ForegroundColor Red
}

# Check if COM port appeared
Write-Host "[3/3] Checking for COM port..." -ForegroundColor Cyan
Start-Sleep -Seconds 3

$comPorts = Get-CimInstance Win32_SerialPort | Where-Object { 
    $_.Description -like '*ESP*' -or 
    $_.Description -like '*USB Serial*' -or
    $_.Description -like '*303A*'
}

if ($comPorts) {
    Write-Host "`n✓ SUCCESS! COM port detected:" -ForegroundColor Green
    $comPorts | Format-Table DeviceID, Description -AutoSize
} else {
    Write-Host "`n✗ No COM port detected yet.`n" -ForegroundColor Red
    Write-Host "MANUAL STEPS REQUIRED:" -ForegroundColor Yellow
    Write-Host "1. Unplug the ESP32 device" -ForegroundColor White
    Write-Host "2. Wait 5 seconds" -ForegroundColor White
    Write-Host "3. Plug it back in" -ForegroundColor White
    Write-Host "4. If still no COM port, use Device Manager:`n" -ForegroundColor White
    
    Write-Host "   Device Manager Steps:" -ForegroundColor Cyan
    Write-Host "   a. Open Device Manager" -ForegroundColor White
    Write-Host "   b. Find device under 'Other devices' or 'USB'" -ForegroundColor White
    Write-Host "   c. Right-click → Update Driver" -ForegroundColor White
    Write-Host "   d. Browse → Let me pick" -ForegroundColor White
    Write-Host "   e. Select 'Ports (COM & LPT)'" -ForegroundColor White
    Write-Host "   f. Choose 'USB Serial Device'" -ForegroundColor White
    Write-Host "   g. Click Next`n" -ForegroundColor White
    
    Write-Host "Opening Device Manager for you..." -ForegroundColor Yellow
    Start-Process devmgmt.msc
}

Write-Host "`nPress any key to exit..." -ForegroundColor DarkGray
$null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown')
