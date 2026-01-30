@echo off
:: ESP32 USB Driver Installer
:: Run this if your ESP32 device is not recognized
echo.
echo ============================================
echo    ESP32 USB Driver Installer
echo ============================================
echo.
echo This will install USB drivers for ESP32-S3
echo.
pause

PowerShell -NoProfile -ExecutionPolicy Bypass -Command ^
"$driverZip = Join-Path $PSScriptRoot 'esp32-usb-driver.zip'; ^
if (-not (Test-Path $driverZip)) { ^
    Write-Host 'Downloading ESP32 USB drivers...' -ForegroundColor Yellow; ^
    Invoke-WebRequest -Uri 'https://dl.espressif.com/dl/idf-driver/idf-driver-esp32-usb-jtag-2021-07-15.zip' -OutFile $driverZip -UseBasicParsing; ^
}; ^
$driverDir = Join-Path $PSScriptRoot 'esp32-driver-temp'; ^
if (Test-Path $driverDir) { Remove-Item $driverDir -Recurse -Force }; ^
Expand-Archive -Path $driverZip -DestinationPath $driverDir -Force; ^
$installer = Get-ChildItem -Path $driverDir -Filter '*.exe' -Recurse | Select-Object -First 1; ^
if ($installer) { ^
    Write-Host 'Running driver installer...' -ForegroundColor Green; ^
    Start-Process -FilePath $installer.FullName -Wait -Verb RunAs; ^
    Write-Host 'Driver installation complete!' -ForegroundColor Green; ^
    Write-Host 'Please replug your ESP32 device.' -ForegroundColor Yellow; ^
} else { ^
    Write-Host 'Error: Could not find installer' -ForegroundColor Red; ^
}"

echo.
echo Driver installation finished.
echo Please replug your ESP32 device and try flashing again.
echo.
pause
