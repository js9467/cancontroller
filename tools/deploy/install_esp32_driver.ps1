param()

$ErrorActionPreference = 'Stop'

Write-Host "[Driver] ESP32-S3 USB driver helper" -ForegroundColor Cyan

$driverZip = Join-Path $PSScriptRoot 'esp32-usb-driver.zip'
if (-not (Test-Path $driverZip)) {
    Write-Host '[Driver] Downloading ESP32 USB drivers...' -ForegroundColor Yellow
    $url = 'https://dl.espressif.com/dl/idf-driver/idf-driver-esp32-usb-jtag-2021-07-15.zip'
    Invoke-WebRequest -Uri $url -OutFile $driverZip -UseBasicParsing
}

$driverDir = Join-Path $PSScriptRoot 'esp32-driver-temp'
if (Test-Path $driverDir) {
    Remove-Item $driverDir -Recurse -Force
}

Expand-Archive -Path $driverZip -DestinationPath $driverDir -Force

$installer = Get-ChildItem -Path $driverDir -Filter '*.exe' -Recurse | Select-Object -First 1
if (-not $installer) {
    Write-Host '[Driver] Error: Could not find installer in esp32-usb-driver.zip' -ForegroundColor Red
    exit 1
}

Write-Host "[Driver] Running ESP32 USB driver installer (a UAC prompt may appear)..." -ForegroundColor Green
Write-Host "[Driver] Please complete the installer GUI, then unplug and replug your ESP32-S3." -ForegroundColor Yellow

Start-Process -FilePath $installer.FullName -Verb RunAs
