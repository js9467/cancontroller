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
```

The standalone script caches downloaded files in `%LOCALAPPDATA%\BroncoControls\flash-temp\` for 7 days, so subsequent flashes are faster.

## For Developers

### ⚡ Quick Reference: Version Update Checklist

```
□ Increment .version_state.json (major/minor/patch)
□ Run: pio run -e waveshare_7in
□ Copy: .pio\build\waveshare_7in\firmware.bin → ..\master-wt\versions\bronco_vX.Y.Z.bin
□ Commit Bronco-Controls-4: git add . && git commit -m "vX.Y.Z: Description"
□ Commit master-wt:          git add . && git commit -m "vX.Y.Z: binary"
□ Push both repos: git push
```

Devices poll GitHub for the highest `bronco_vX.Y.Z.bin` and download it automatically.

See [OTA_PUBLISHING.md](../../OTA_PUBLISHING.md) for the full step-by-step guide.

### 🚀 Creating and Deploying New Firmware Versions

#### 1. **Bump Version**
Edit `.version_state.json` in the repo root and increment the version number.

#### 2. **Build Firmware**
```powershell
pio run -e waveshare_7in
```
Verify build succeeds and note the version logged by the pre-build script.

#### 3. **Copy Binary to master-wt**
```powershell
Copy-Item .pio\build\waveshare_7in\firmware.bin ..\master-wt\versions\bronco_vX.Y.Z.bin
```

#### 4. **Commit and Push Both Repos**
```powershell
# Bronco-Controls-4
git add .
git commit -m "vX.Y.Z: Description of changes"
git push

# master-wt
cd ..\master-wt
git add versions\bronco_vX.Y.Z.bin
git commit -m "vX.Y.Z: binary"
git push
```

Devices will detect the new version on their next OTA check and update automatically.

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
# Download latest firmware from GitHub
.\update_firmware.ps1

# Download and rebuild BroncoFlasher.zip
.\update_firmware.ps1 -RebuildZip
```

This downloads the latest `bronco_vX.Y.Z.bin` from GitHub and replaces `firmware.bin` in BroncoFlasher.zip.

## OTA vs USB Flashing

### OTA (Over-The-Air) Updates
- **Use when:** Device is already running Bronco Controls firmware
- **Advantages:** Wireless, no USB cable needed, can update multiple devices remotely
- **How:** Device checks GitHub for the latest `bronco_vX.Y.Z.bin`, downloads and installs automatically
- **Source:** `https://github.com/js9467/cancontroller/tree/master/versions`

### USB Flashing
- **Use when:** 
  - First-time installation
  - OTA is not working
  - Device is bricked or won't boot
  - No WiFi available
- **Advantages:** Always works, doesn't require existing firmware
- **How:** Connect USB cable, run flash_device script

## Firmware Distribution

Firmware is distributed in two ways:

1. **GitHub `versions/` folder** (`js9467/cancontroller`) — primary OTA source; devices download directly
2. **BroncoFlasher.zip** — downloadable package for USB flashing (offline or first-install)

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

- **GitHub Repository:** https://github.com/js9467/cancontroller
- **OTA Versions:** https://github.com/js9467/cancontroller/tree/master/versions
- **BroncoFlasher Download:** https://github.com/js9467/autotouchscreen/raw/main/tools/deploy/BroncoFlasher.zip
