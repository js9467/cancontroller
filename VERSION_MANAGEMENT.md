# Bronco Controls - Version Management System

## Overview

The version management system provides three distinct operations for managing firmware versions and backups:

### 🎯 Operations

#### 1. 📝 Version Update (Incremental)
**Purpose:** Create lightweight version snapshots from current project state  
**Version Change:** Build number increments (e.g., 1.3.82 → 1.3.83)  
**Device Required:** ❌ No  
**Upload:** ✅ Automatic to GitHub

**What it does:**
- Increments build number in `.version_state.json` and `src/version_auto.h`
- Builds firmware from current source code using PlatformIO
- Commits version files to git repository
- Automatically uploads firmware binary to GitHub
- Target: `https://github.com/js9467/cancontroller/tree/master/versions`

**Use when:**
- You've made code changes and want to publish a new version
- Testing new features
- Making incremental improvements
- No device connection needed

**Command line:**
```bash
python backup_restore_manager.py version
```

---

#### 2. 📦 Full Backup (Major Version)
**Purpose:** Create complete hardware + software backup from device  
**Version Change:** MAJOR version increments (e.g., 1.3.82 → 2.0.0)  
**Device Required:** ✅ Yes (USB connected)  
**Upload:** ✅ Automatic to GitHub

**What it does:**
- Increments MAJOR version number (resets minor and build to 0)
- Reads ALL flash memory regions from connected ESP32-S3 device:
  - Bootloader (0x0, 24KB)
  - Partition table (0x8000, 12KB)
  - OTA data (0xD000, 8KB)
  - NVS (0xE000, 24KB)
  - Application slot 0 (0x10000, 3.5MB)
  - Application slot 1 (0x390000, 3.5MB)
  - SPIFFS file system (0x710000, 896KB)
- Creates complete restoration point for known-good hardware state
- Automatically uploads to GitHub
- Can restore EXACT device state including WiFi credentials, CAN config, etc.

**Use when:**
- You have a groundbreaking change or stable milestone
- Creating a "golden" backup before major changes
- Hardware issues require complete restoration capability
- Want to clone device configuration to another unit

**Command line:**
```bash
python backup_restore_manager.py full-backup --port COM5
```

---

#### 3. 📡 OTA Update (Coming Soon)
**Purpose:** Over-the-air firmware updates from GitHub  
**Device Required:** ✅ Yes (WiFi connected)  
**Current Status:** 🚧 GUI interface ready, ESP32 OTA code to be added

**Planned functionality:**
- Check GitHub for available versions
- Display version list with types (Full Backup vs Version Update)
- Select version to install
- Download and flash firmware wirelessly
- No USB cable required (after initial setup)

---

## Version Numbering System

Format: `MAJOR.MINOR.BUILD`

- **MAJOR** - Incremented by Full Backup (e.g., 1.x.x → 2.0.0)
- **MINOR** - Manual changes only (not auto-incremented)
- **BUILD** - Incremented by Version Update (e.g., 1.3.82 → 1.3.83)

### Files Updated
- `.version_state.json` - Persistent version tracking
- `src/version_auto.h` - C++ header with version constant
- Git commits automatically tag version files

---

## GitHub Integration

### Repository Structure
```
https://github.com/js9467/cancontroller/tree/master/versions/
├── bronco_v1.3.83.zip          (Version Update)
├── bronco_v2.0.0_FULL.zip      (Full Backup)
├── bronco_v2.0.1.zip           (Version Update)
└── ...
```

### Authentication
Set environment variable:
```powershell
$env:GITHUB_TOKEN = "ghp_your_token_here"
```

Or token will be prompted when uploading.

### Automatic Uploads
Both Version Update and Full Backup operations automatically upload to GitHub:
- No `--upload` flag needed (removed from CLI)
- Uploads happen immediately after successful operation
- Creates ZIP archive of version/backup
- Uploads via GitHub API to `versions` folder

---

## GUI Application

Launch with:
```bash
python backup_restore_gui.py
```

### Tabs

1. **📝 Version Update** - Create incremental version (no device needed)
2. **📦 Full Backup** - Create major version backup (device required)
3. **📡 OTA Update** - Check/install GitHub versions (coming soon)
4. **♻️ Restore** - Restore from local backup (device required)

---

## Command Line Usage

```bash
# Create version update (no device needed)
python backup_restore_manager.py version

# Create full backup from device
python backup_restore_manager.py full-backup --port COM5

# List local backups
python backup_restore_manager.py list

# List GitHub versions
python backup_restore_manager.py list-github

# Restore from latest backup
python backup_restore_manager.py restore --port COM5

# Restore specific backup
python backup_restore_manager.py restore --backup "backups/bronco_v2.0.0_20260130_123456_FULL" --port COM5
```

---

## Backup Contents

### Version Update Package
```
bronco_v1.3.83/
├── firmware.bin           # Compiled firmware binary
└── version_metadata.json  # Version info, build timestamp
```

### Full Backup Package
```
bronco_v2.0.0_20260130_123456_FULL/
├── bootloader.bin         # Bootloader (24KB)
├── partition_table.bin    # Partition table (12KB)
├── ota_data_initial.bin   # OTA data (8KB)
├── nvs.bin               # NVS storage (24KB)
├── app0.bin              # Application slot 0 (3.5MB)
├── app1.bin              # Application slot 1 (3.5MB)
├── spiffs.bin            # File system (896KB)
├── metadata.json         # Backup metadata
└── restore.bat           # Windows restore script
```

---

## Restoration

### From Full Backup
1. Connect device via USB
2. Select backup in GUI Restore tab
3. Confirm (this ERASES all device data!)
4. Wait for restore process (3-5 minutes)
5. Device automatically reboots

### Restoration Script
Each full backup includes `restore.bat`:
```bash
cd backups/bronco_v2.0.0_20260130_123456_FULL
restore.bat COM5
```

---

## Workflow Examples

### Daily Development
```bash
# Make code changes
code src/main.cpp

# Create version update
python backup_restore_manager.py version
# → Creates v1.3.84, uploads to GitHub
```

### Major Milestone
```bash
# Connect device via USB to COM5
# Create full backup
python backup_restore_manager.py full-backup --port COM5
# → Creates v2.0.0 FULL backup, uploads to GitHub
```

### Clone Device
```bash
# Full backup from source device
python backup_restore_manager.py full-backup --port COM5

# Restore to target device
python backup_restore_manager.py restore --port COM6
# Select the full backup from list
```

---

## Troubleshooting

### "Build failed"
- Ensure PlatformIO is installed: `pip install platformio`
- Check `platformio.ini` configuration
- Verify all source files compile

### "GitHub upload failed"
- Check `GITHUB_TOKEN` environment variable
- Verify token has `repo` permissions
- Check network connectivity

### "Device not found"
- Verify correct COM port
- Install ESP32-S3 drivers (run `tools/deploy/Install-Drivers.bat`)
- Check USB cable supports data transfer

### "Restore failed"
- Ensure device is in bootloader mode (usually automatic)
- Try lower baud rate: `--baud 115200`
- Verify backup files are complete and not corrupted

---

## Future Enhancements

- [ ] OTA update implementation in ESP32 firmware
- [ ] Web interface for version management
- [ ] Automated testing before version publication
- [ ] Changelog generation from git commits
- [ ] Version comparison and diff tools
- [ ] Rollback to previous version with one click
