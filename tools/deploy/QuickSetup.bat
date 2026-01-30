@echo off
:: Bronco Controls - Quick Setup (Direct PowerShell Installer)
echo.
echo ====================================================
echo    Bronco Controls - Quick Setup
echo ====================================================
echo.
echo This will download and run the installer automatically.
echo.
echo BEFORE YOU CONTINUE:
echo   1. Make sure ESP32 device is connected via USB
echo   2. Close Arduino IDE, PuTTY, or any serial monitors
echo   3. Have a data USB cable (not charge-only)
echo.
pause

echo.
echo Downloading and running installer...
echo.

PowerShell -NoProfile -ExecutionPolicy Bypass -Command ^
"try { ^
    $timestamp = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds(); ^
    $url = 'https://raw.githubusercontent.com/js9467/autotouchscreen/main/tools/deploy/BroncoFlasher.ps1?nocache=' + $timestamp; ^
    Write-Host 'Downloading installer...' -ForegroundColor Cyan; ^
    $script = Invoke-RestMethod -Uri $url -UseBasicParsing; ^
    Write-Host 'Running installer...' -ForegroundColor Green; ^
    Invoke-Expression $script; ^
} catch { ^
    Write-Host 'Error: ' $_.Exception.Message -ForegroundColor Red; ^
    exit 1; ^
}"

if %errorlevel% equ 0 (
    echo.
    echo ====================================================
    echo    Installation Complete!
    echo ====================================================
    echo.
    echo Your device is ready to use!
    echo.
) else (
    echo.
    echo ====================================================
    echo    Installation Issue
    echo ====================================================
    echo.
    echo Common fixes:
    echo   - Make sure ESP32 is connected
    echo   - Use a USB data cable (not charge-only)
    echo   - Close other programs using COM ports
    echo   - Try a different USB port
    echo.
)

pause
