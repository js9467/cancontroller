@echo off
REM Bronco Controls - Backup & Restore Manager Launcher
REM Simple launcher for Windows users

echo ============================================================
echo   Bronco Controls - Backup ^& Restore Manager
echo ============================================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python not found!
    echo.
    echo Please install Python 3.8 or later from:
    echo https://www.python.org/downloads/
    echo.
    pause
    exit /b 1
)

REM Install requirements if needed
if not exist ".venv\" (
    echo [Setup] First-time setup - installing dependencies...
    python -m pip install --quiet requests esptool
    echo.
)

REM Launch GUI
echo [Launch] Starting Backup ^& Restore Manager...
echo.
python backup_restore_gui.py

if errorlevel 1 (
    echo.
    echo [ERROR] Failed to start application!
    pause
)
