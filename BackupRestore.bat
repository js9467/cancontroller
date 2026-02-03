@echo off
REM Bronco Controls - Backup & Restore Manager Launcher
REM Double-click this file to launch the GUI application

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

REM Check if backup_restore_gui.py exists
if not exist "backup_restore_gui.py" (
    echo [ERROR] backup_restore_gui.py not found!
    echo Please run this from the Bronco-Controls directory.
    echo.
    pause
    exit /b 1
)

REM Install requirements if needed (first run only)
if not exist ".deps_installed" (
    echo [Setup] First-time setup - installing dependencies...
    python -m pip install --quiet --upgrade pip
    python -m pip install --quiet requests esptool pyserial
    echo. > .deps_installed
    echo [Setup] Dependencies installed!
    echo.
)

REM Launch GUI
echo [Launch] Starting Backup ^& Restore Manager GUI...
echo.
python backup_restore_gui.py

if errorlevel 1 (
    echo.
    echo [ERROR] Application exited with error!
    echo Check console output above for details.
    echo.
    pause
)
