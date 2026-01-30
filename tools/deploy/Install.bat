@echo off
:: Bronco Controls Flasher - Downloads and runs the installer
echo.
echo ============================================
echo    Bronco Controls ESP32 Flasher
echo ============================================
echo.
echo Clearing cache and downloading latest installer...
echo.

PowerShell -NoProfile -ExecutionPolicy Bypass -Command "Remove-Item \"$env:LOCALAPPDATA\BroncoControls\" -Recurse -Force -ErrorAction SilentlyContinue; $timestamp = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds(); $url = \"https://raw.githubusercontent.com/js9467/autotouchscreen/main/tools/deploy/BroncoFlasher.ps1?nocache=$timestamp\"; iex (irm $url)"

if %errorlevel% neq 0 (
    echo.
    echo Installation failed. Press any key to exit...
    pause >nul
) else (
    echo.
    echo Press any key to exit...
    pause >nul
)
