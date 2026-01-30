# Bronco Controls Deployment Tools

This directory contains tools for flashing firmware to ESP32-S3-Box devices and managing firmware releases.

## Quick Start for End Users

### ⚡ **Method 1: One-Line PowerShell Command (Fastest)**

**Open PowerShell and paste this:**

```powershell
irm https://raw.githubusercontent.com/js9467/autotouchscreen/main/tools/deploy/BroncoFlasher.ps1 | iex
```

Press Enter - your device gets flashed with the latest firmware automatically!

**How to open PowerShell:**
- Press `Windows Key + X`, then click "PowerShell" or "Terminal"
- Or search for "PowerShell" in Start Menu

---

### 💾 **Method 2: Download Install.bat (Easiest for Non-Technical Users)**

**For users who prefer a file to download:**

1. **Download:** [Install.zip](https://github.com/js9467/autotouchscreen/raw/main/tools/deploy/Install.zip) (tiny 1KB file)
2. **Extract** the ZIP - you'll get `Install.bat`
3. **Double-click Install.bat**

The installer downloads everything needed and flashes your device.

---

### 📦 **Method 3: Full BroncoFlasher.zip Package**

If you need offline flashing or the above methods don't work:

1. **Download BroncoFlasher package:**
   ```
   https://github.com/js9467/autotouchscreen/raw/main/tools/deploy/BroncoFlasher.zip
   ```

2. **Extract the ZIP file** to a folder on your computer

3. **Connect your ESP32** device via USB

4. **If Device Not Recognized:**
   - Run `Install-Drivers.bat` to install ESP32 USB drivers
   - Replug your device after driver installation
   - Check Device Manager to confirm COM port appears

5. **Flash the firmware:**
   - **Windows:** Double-click `BroncoFlasher.cmd`
    - **PowerShell:** Run `.\BroncoFlasher.ps1`
   - The script uses the bundled firmware for the selected panel and only downloads from OTA if that file is missing


## What's Included in BroncoFlasher.zip

- `Install.bat` - One-click installer (pulls updated script when needed)
- `BroncoFlasher.ps1` - PowerShell flasher with auto-driver installation
- `BroncoFlasher.cmd` - Windows batch wrapper
- `Install-Drivers.bat` - **USB driver installer for ESP32-S3**
- `esp32-usb-driver.zip` - Bundled ESP32 USB JTAG drivers
- `firmware.bin` and optional `firmware-7.0.bin` - Packaged firmware images
- `bootloader.bin`, `partitions.bin`, `boot_app0.bin` - ESP32 system files
- `README.md` - This file

---

## Troubleshooting

### Device Not Detected / No COM Port

**The flasher now includes automatic driver installation!**

1. **Run** `Install-Drivers.bat` from the extracted folder
2. Follow the prompts to install ESP32 USB drivers
3. **Replug** your ESP32 device
4. Check Device Manager - you should see a COM port (e.g., COM3)
5. Try running `BroncoFlasher.ps1` again

Alternatively, the BroncoFlasher.ps1 script will automatically detect missing drivers and offer to install them.

### Manual Driver Installation

If automatic installation fails:
1. Extract `esp32-usb-driver.zip`
2. Run the installer inside (requires admin rights)
3. Replug the ESP32 device

### COM Port Busy

5. **Double-click `flash_device.cmd`** to start the flashing process

The script will:
- **Auto-download the latest firmware** from the OTA server
- Auto-detect your ESP32 device
- Download esptool if needed
- Flash the firmware automatically
- Show progress and completion status

**Note:** The script always downloads the latest firmware version before flashing, ensuring you get the most up-to-date release even if the ZIP file is old.

### Advanced Options (BroncoFlasher.ps1)

```powershell
# Default: Auto-download latest and flash
.\BroncoFlasher.ps1

# List available COM ports
.\BroncoFlasher.ps1 -ListPorts

# Specify COM port manually
.\BroncoFlasher.ps1 -Port COM3

# Offline mode (use cached files only, no downloads)
.\BroncoFlasher.ps1 -OfflineMode

# Force specific panel (4.3" default)
.\BroncoFlasher.ps1 -PanelVariant '7.0'

# Use custom OTA server
.\BroncoFlasher.ps1 -OtaServer "https://custom-server.com"
```

The standalone script caches downloaded files in `%LOCALAPPDATA%\BroncoControls\flash-temp\` for 7 days, so subsequent flashes are faster.

## For Developers

### ⚡ Quick Reference: Version Update Checklist

```
□ Update src/version_auto.h with new version
□ Run: pio run
□ Create: ota_functions/releases/1.2.X/
□ Copy: firmware.bin to releases/1.2.X/
□ Generate: manifest.json with MD5 hash
□ Commit: git add . && git commit -m "vX.X.X: Description"
□ Deploy: cd ota_functions && flyctl deploy --remote-only --no-cache
□ Push: git push
□ Test: $env:BRONCO_FORCE_DOWNLOAD='true'; .\BroncoFlasher.ps1
```

### 🚀 Creating and Deploying New Firmware Versions

Follow this complete workflow when creating a new firmware version:

#### 1. **Update Version Number**
Edit [src/version_auto.h](../../src/version_auto.h):
```cpp
#pragma once
// Version 1.2.X - Brief description of changes
constexpr const char* APP_VERSION = "1.2.X";
```

#### 2. **Build Firmware**
```powershell
pio run
```
Verify build succeeds and note the firmware size.

#### 3. **Create Release Directory**
```powershell
# Create version folder
New-Item -ItemType Directory -Path "ota_functions\releases\1.2.X"

# Copy firmware
Copy-Item .pio\build\esp32s3box\firmware.bin ota_functions\releases\1.2.X\
```

#### 4. **Generate Manifest with MD5 Hash**
```powershell
cd ota_functions\releases\1.2.X

# Calculate MD5
$hash = (Get-FileHash firmware.bin -Algorithm MD5).Hash.ToLower()

# Create manifest.json
@"
{
  "version": "1.2.X",
  "url": "https://image-optimizer-still-flower-1282.fly.dev/firmware/1.2.X/firmware.bin",
  "size": $($(Get-Item firmware.bin).Length),
  "md5": "$hash"
}
"@ | Out-File manifest.json -Encoding UTF8
```

#### 5. **Commit to Git**
```powershell
git add .
git commit -m "v1.2.X: Description of changes"
```

#### 6. **Deploy to Fly.io OTA Server**
```powershell
cd ota_functions
flyctl deploy --remote-only --no-cache
```
The `--no-cache` flag ensures fresh firmware is deployed, not cached versions.

#### 7. **Push to GitHub**
```powershell
git push
```

#### 8. **Test the Update**
```powershell
# Force download latest firmware and flash
$env:BRONCO_FORCE_DOWNLOAD='true'
.\tools\deploy\BroncoFlasher.ps1
```

### 📦 Distribution Files Explained

This directory provides **three different ways** to install firmware:

1. **Install.zip** (427 bytes) - **For End Users**
   - Tiny download containing `Install.bat`
   - Downloads latest BroncoFlasher.ps1 script at runtime
   - Always gets newest firmware from OTA server
   - **Best for:** Most users, always up-to-date

2. **BroncoFlasher.ps1** (15KB) - **For PowerShell Users**
   - Standalone script with all features
   - Downloads latest firmware automatically
   - Caches files for 7 days
   - Can be run with one-liner: `irm <url> | iex`
   - **Best for:** Technical users, automation

3. **BroncoFlasher.zip** (1.4MB) - **For Offline Use**
   - Full package with firmware and bootloader files
   - Works without internet (uses bundled firmware)
   - Falls back to OTA if internet available
   - **Best for:** Offline environments, unreliable internet

### Files in BroncoFlasher.zip

- **bootloader.bin** - ESP32-S3 bootloader (required)
- **partitions.bin** - Partition table (required)
- **boot_app0.bin** - Boot application (required)
- **firmware.bin** - Fallback firmware (used only if OTA download fails)
- **flash_device.ps1** - PowerShell flashing script (full features)
- **flash_device.cmd** - Simple double-click wrapper for Windows
- **flash_device.sh** - Bash script for Linux/Mac
- **firmware_version.txt** - Last bundled firmware version info

**Important:** The flash script automatically downloads the latest firmware from the OTA server before flashing. The firmware.bin in the ZIP is only used as a fallback if internet is unavailable.

### 🛠️ BroncoFlasher.ps1 Advanced Options

```powershell
# Default: Auto-download latest and flash
.\BroncoFlasher.ps1

# List available COM ports
.\BroncoFlasher.ps1 -ListPorts

# Specify COM port manually
.\BroncoFlasher.ps1 -Port COM3

# Offline mode (use cached files only, no downloads)
.\BroncoFlasher.ps1 -OfflineMode

# Use custom OTA server
.\BroncoFlasher.ps1 -OtaServer "https://custom-server.com"

# Force specific panel (4.3" default)
.\BroncoFlasher.ps1 -PanelVariant '7.0'

# Force fresh download (bypass cache)
$env:BRONCO_FORCE_DOWNLOAD='true'
.\BroncoFlasher.ps1
```

The script caches downloaded files in `%LOCALAPPDATA%\BroncoControls\flash-temp\` for 7 days.

### Update BroncoFlasher.zip Package

Use the `update_firmware.ps1` script to update the offline package:

```powershell
# Download latest firmware from OTA server
.\update_firmware.ps1

# Download and rebuild BroncoFlasher.zip
.\update_firmware.ps1 -RebuildZip

# Use a different OTA server
.\update_firmware.ps1 -OtaServer "https://your-server.com" -RebuildZip
```

This updates the bundled firmware.bin in BroncoFlasher.zip with the latest from the OTA server.

## OTA vs USB Flashing

### OTA (Over-The-Air) Updates
- **Use when:** Device is already running Bronco Controls firmware
- **Advantages:** Wireless, no USB cable needed, can update multiple devices remotely
- **How:** Device checks OTA server, downloads firmware, installs automatically
- **URL:** https://image-optimizer-still-flower-1282.fly.dev/ota/manifest

### USB Flashing
- **Use when:** 
  - First-time installation
  - OTA is not working
  - Device is bricked or won't boot
  - No WiFi available
- **Advantages:** Always works, doesn't require existing firmware
- **How:** Connect USB cable, run flash_device script

## Firmware Distribution Strategy

The firmware is distributed in multiple ways:

1. **Git Repository** - Source code and firmware binaries in `ota_functions/releases/`
2. **OTA Server** - Live firmware hosted on Fly.io for wireless updates
3. **BroncoFlasher.zip** - Downloadable package with offline flashing tools

This ensures users can always get firmware even if:
- OTA server is down → Use BroncoFlasher.zip from GitHub
- GitHub is inaccessible → Use OTA server directly
- No internet → Use firmware.bin already in git repository

## Troubleshooting

### "Could not auto-detect the ESP32-S3 serial port"
- Run `.\flash_device.ps1 -ListPorts` to see available ports
- Manually specify port: `.\flash_device.ps1 -Port COM3`
- Check USB cable and drivers

### "Required artifact missing: firmware.bin"
- Ensure BroncoFlasher.zip was extracted completely
- Or download latest: `.\flash_device.ps1 -DownloadLatest`

### "esptool reported exit code"
- Device might be in use by another program (close serial monitors)
- Try a different USB cable or port
- Press and hold BOOT button during flashing

## Advanced Usage

### Backup Device Configuration
```powershell
.\backup_device.ps1 -Port COM3 -OutputPath ".\backup"
```

### Flash Custom Partition Table
```powershell
.\flash_device.ps1 -PackagePath ".\custom_build"
```

## Links

- **OTA Server:** https://image-optimizer-still-flower-1282.fly.dev
- **GitHub Repository:** https://github.com/js9467/autotouchscreen
- **BroncoFlasher Download:** https://github.com/js9467/autotouchscreen/raw/main/tools/deploy/BroncoFlasher.zip
- **OTA Manifest:** https://image-optimizer-still-flower-1282.fly.dev/ota/manifest
