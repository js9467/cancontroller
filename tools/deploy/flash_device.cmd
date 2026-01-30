@echo off
setlocal
cd /d "%~dp0" 2>nul
powershell -ExecutionPolicy Bypass -NoProfile -NoLogo -File "%~dp0flash_device.ps1" %*
if %errorlevel% neq 0 (
	echo.
	echo Flashing failed. Review the messages above.
	pause
	exit /b %errorlevel%
)
echo.
echo Flash completed successfully. You may disconnect the device.
pause
