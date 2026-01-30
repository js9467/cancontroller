# Bronco Controls - Backup & Restore Manager

Complete backup and restore solution for ESP32-S3 Touch LCD devices.

## Features

✅ **Full Device Backup**
- Bootloader and partition table
- Application firmware (both OTA slots)
- Non-volatile storage (NVS) - preserves WiFi credentials, settings, etc.
- OTA data partition
- SPIFFS filesystem
- Complete 16MB flash dump

✅ **Automatic Version Management**
- Auto-increment version numbers
- Updates firmware version (`src/version_auto.h`)
- Updates web interface version display
- Version stamped on backup files

✅ **GitHub Integration**
- Automatic upload to GitHub repository
- Stores backups in `versions/` folder
- Requires `GITHUB_TOKEN` environment variable

✅ **Full Restore Capability**
- Complete device erase
- Restoration from any backup
- Standalone restore scripts included with each backup
- Fresh-from-box device can be restored to any saved state

✅ **Testing & Validation**
- Full backup → erase → restore test cycle
- Verifies backup integrity
- Safe rollback capability

## Quick Start

### Option 1: GUI Application (Easiest)

Double-click `BackupRestore.bat` to launch the graphical interface.

**First time only:** Will automatically install Python dependencies.

### Option 2: Command Line

```powershell
# Create backup with auto-incremented version
python backup_restore_manager.py backup

# Create backup and upload to GitHub
python backup_restore_manager.py backup --upload

# List all available backups
python backup_restore_manager.py list

# Restore from latest backup
python backup_restore_manager.py restore

# Restore from specific backup
python backup_restore_manager.py restore --backup "backups/bronco_v1.3.79_20260130_123456"

# Run full test cycle (backup → erase → restore)
python backup_restore_manager.py test
```

### Option 3: Windows Executable

Build a standalone `.exe` (no Python required for end users):

```powershell
# Build executable
.\build_backup_tool.ps1 -Install -Build

# Run it
.\dist\BroncoBackupRestore.exe
```

## Requirements

- **Python 3.8+** (only for CLI/source version, not needed for .exe)
- **esptool** - Installed automatically
- **USB Connection** - Device connected to COM5 (configurable)
- **GITHUB_TOKEN** (optional) - Only needed for automatic GitHub uploads

## Backup Structure

Each backup creates a folder with:

```
bronco_v1.3.79_20260130_123456/
├── bootloader.bin          # ESP32 bootloader
├── partitions.bin          # Partition table
├── nvs.bin                 # Non-volatile storage (WiFi, settings)
├── otadata.bin             # OTA data
├── app0.bin                # Main application
├── app1.bin                # OTA application slot
├── spiffs.bin              # File system
├── full_flash_16MB.bin     # Complete flash dump
├── backup_metadata.json    # Backup information
└── RESTORE.bat             # Standalone restore script
```

## GitHub Auto-Upload Setup

To enable automatic GitHub uploads:

1. **Create a GitHub Personal Access Token**
   - Go to: https://github.com/settings/tokens
   - Click "Generate new token (classic)"
   - Select scope: `repo` (full repository access)
   - Copy the token

2. **Set environment variable**

   **Windows (PowerShell - Permanent):**
   ```powershell
   [Environment]::SetEnvironmentVariable("GITHUB_TOKEN", "your_token_here", "User")
   ```

   **Windows (CMD - Current session):**
   ```cmd
   set GITHUB_TOKEN=your_token_here
   ```

3. **Verify**
   ```powershell
   $env:GITHUB_TOKEN
   ```

## Typical Workflows

### 📦 Create a Backup Before Making Changes

```powershell
python backup_restore_manager.py backup
```
- Version auto-increments (e.g., 1.3.78 → 1.3.79)
- Backup saved to `backups/bronco_v1.3.79_[timestamp]/`
- Ready to restore if something goes wrong

### ♻️ Restore After a Failed Update

```powershell
python backup_restore_manager.py list
python backup_restore_manager.py restore
```
- Lists all backups
- Restores from most recent backup
- Device returns to working state

### 🆕 Set Up a New Device

1. Create backup from working device:
   ```powershell
   python backup_restore_manager.py backup --upload
   ```

2. On new device, restore from GitHub:
   - Download backup from `github.com/js9467/autotouchscreen/tree/main/versions`
   - Extract backup folder
   - Run `RESTORE.bat` inside the folder
   
   OR
   
   - Clone repo with backups
   - Run: `python backup_restore_manager.py restore --backup "path/to/backup"`

### 🧪 Test Backup Integrity

```powershell
python backup_restore_manager.py test
```
- Creates backup
- Erases device
- Restores from backup
- Verifies everything works

## Version Management

### How Versioning Works

1. **Version State File** (`.version_state.json`):
   ```json
   {
     "major": 1,
     "minor": 3,
     "build": 78
   }
   ```

2. **Auto-Generated Header** (`src/version_auto.h`):
   ```cpp
   #pragma once
   // Auto-generated on 2026-01-30T15:24:19.330840Z
   constexpr const char* APP_VERSION = "1.3.78";
   ```

3. **Web Interface** - Version displayed in:
   - Header: "Firmware v1.3.78"
   - Settings page status chip
   - API responses

### Manual Version Control

To manually set a version:

```powershell
# Edit .version_state.json
{
  "major": 2,
  "minor": 0,
  "build": 0
}

# Rebuild
pio run -e waveshare_7in
```

## Troubleshooting

### "Failed to connect to ESP32"
- Check USB cable (must support data, not just charging)
- Try different COM port (change in GUI or use `--port COM6`)
- Close other serial monitors (Arduino IDE, PuTTY, etc.)
- Press BOOT button while connecting

### "esptool not found"
```powershell
python -m pip install esptool
```

### "Permission denied" on COM port
- Close other applications using serial ports
- Run as Administrator
- Restart computer (Windows sometimes locks COM ports)

### Backup/Restore is slow
- Normal! 16MB flash takes 5-10 minutes
- Use `--baud 921600` for faster transfers (may be unstable)
- Default 460800 is reliable

### GitHub upload fails
- Check `GITHUB_TOKEN` is set correctly
- Verify token has `repo` permissions
- Check internet connection
- Ensure repository exists and is accessible

## Technical Details

### Flash Memory Map (16MB ESP32-S3)

| Region      | Offset    | Size     | Purpose                          |
|-------------|-----------|----------|----------------------------------|
| Bootloader  | 0x0       | 32 KB    | ESP32 bootloader                 |
| Partitions  | 0x8000    | 4 KB     | Partition table                  |
| NVS         | 0x9000    | 20 KB    | Non-volatile storage             |
| OTA Data    | 0xE000    | 8 KB     | OTA boot selection               |
| App 0       | 0x10000   | ~4 MB    | Main application firmware        |
| App 1       | 0x400000  | ~4 MB    | OTA update slot                  |
| SPIFFS      | 0x7F0000  | 64 KB    | File system                      |

### What Gets Backed Up

**Backed up (preserves state):**
- ✅ Application code
- ✅ WiFi credentials
- ✅ All settings (CAN, display, etc.)
- ✅ Custom configurations
- ✅ Uploaded assets (images, logos)
- ✅ OTA update status

**Not needed (hardware-specific):**
- ❌ Chip-specific eFuses (read-only, managed by ESP32)
- ❌ MAC address (chip-specific, unchangeable)
- ❌ Flash encryption keys (if enabled, handled separately)

## Files Created

- `backup_restore_manager.py` - Core backup/restore logic (CLI)
- `backup_restore_gui.py` - Graphical user interface
- `BackupRestore.bat` - Simple launcher for Windows
- `build_backup_tool.ps1` - Build script for .exe
- `backups/` - Folder containing all backups

## Safety Features

1. **Metadata Validation** - Each backup includes checksums and metadata
2. **Standalone Restore** - Each backup has its own `RESTORE.bat`
3. **Full Flash Dump** - Complete 16MB backup as safety net
4. **Test Mode** - Validate backups before relying on them
5. **Confirmation Prompts** - Prevents accidental erases

## Support

Device: [Waveshare ESP32-S3-Touch-LCD-7](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-7)  
Repository: https://github.com/js9467/autotouchscreen  
Issues: Report via GitHub Issues

## License

Same as main Bronco Controls project.
