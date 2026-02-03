# Bronco Controls - Version Management & OTA Update System

## Overview

This document describes the comprehensive version management, backup, and OTA (Over-The-Air) update system for the Bronco Controls ESP32-S3 project.

## Version Management System

### Version Format
- **Format**: `MAJOR.MINOR.BUILD` (e.g., 2.1.4)
- **Major**: Breaking changes, full device backup, automatic Git push
- **Minor**: New features, minor updates
- **Build**: Bug fixes, incremental changes

### Version State File
- Location: `.version_state.json`
- Tracks: major, minor, build numbers, last increment type, last update timestamp
- Auto-generated header: `src/version_auto.h`

## Backup System

### Backup Types

#### 1. Normal Backup (Build/Minor Increments)
- Backs up all flash regions individually:
  - Bootloader (32KB)
  - Partition table (4KB)
  - NVS - Non-volatile storage (20KB)
  - OTA data (8KB)
  - App0 - Main app (~4MB)
  - App1 - OTA app (~4MB)
  - SPIFFS - File system (64KB)
- Creates full 16MB flash dump for safety
- Naming: `bronco_v{VERSION}_{TIMESTAMP}/`

#### 2. Full Backup (Major Increments)
- Same as normal backup PLUS:
  - Automatic Git commit and push of all source code
  - Naming: `bronco_v{VERSION}_{TIMESTAMP}_FULL/`
  - Creates restore batch script
  - Pushes firmware .bin to GitHub for OTA distribution

### Using the Backup Tool

#### GUI Application (Recommended)
```powershell
python backup_restore_gui.py
```

Features:
- Visual version type selection (Build/Minor/Major)
- Real-time version preview
- Backup management (list, restore, test cycle)
- Progress tracking
- Optional GitHub upload

**Version Selection:**
- **Build (x.x.N)**: Increments build number only
- **Minor (x.N.0)**: Increments minor, resets build
- **MAJOR (N.0.0) + GIT PUSH**: Increments major, resets minor/build, pushes to Git

#### Command Line
```powershell
# Create backup with build increment
python backup_restore_manager.py backup

# Create backup and upload to GitHub
python backup_restore_manager.py backup --upload

# List all backups
python backup_restore_manager.py list

# Restore from latest backup
python backup_restore_manager.py restore

# Restore from specific backup
python backup_restore_manager.py restore --backup "backups/bronco_v2.1.4_20260203_123456"

# Full test cycle (backup → erase → restore)
python backup_restore_manager.py test
```

### Major Release Workflow

1. **GUI**: Select "MAJOR (N.0.0) + GIT PUSH" radio button
2. **Confirm**: Major release confirmation dialog appears
3. **Automatic Steps**:
   - Version incremented to next major (e.g., 2.1.4 → 3.0.0)
   - Full device backup created
   - All source code committed to Git
   - Changes pushed to remote repository
   - Firmware .bin uploaded to GitHub versions folder
4. **Result**: Complete snapshot ready for OTA distribution

## OTA Update System

### Architecture

The OTA system supports two update sources:
1. **GitHub Versions** (Primary): Firmware .bin files in `versions/` folder
2. **Manifest URL** (Alternative): JSON manifest with firmware URL

### GitHub OTA Distribution

**File Naming Convention:**
- OTA binaries: `versions/bronco_v{VERSION}.bin` (e.g., `bronco_v2.1.4.bin`)
- Full backups: `versions/bronco_v{VERSION}_FULL.zip` (USB restore only)

**GitHub Repository:**
- Repo: `js9467/cancontroller` (update in `ota_manager.cpp` if different)
- Branch: `master`
- Folder: `versions/`

### Checking for Updates

#### Web Interface
1. Navigate to Settings tab
2. Click "Check Updates" button
3. View available versions
4. Select version from dropdown
5. Click "Install Selected Version"
6. Device downloads, installs, and reboots automatically

**Web Endpoints:**
- `GET /api/ota/github/versions` - List available versions
- `POST /api/ota/github/install` - Install specific version

#### Device Display
1. Tap settings icon (top-right corner of screen)
2. Navigate to "Updates" section
3. Tap "Check for Updates" button
4. Select version (if multiple available)
5. Confirm installation
6. Device downloads, installs, and reboots

#### Command Line (Testing)
```cpp
// In main.cpp or via serial console
OTAUpdateManager::instance().checkForUpdatesNow();
```

### Update Process

1. **Version Check**: Device queries GitHub API for available `.bin` files
2. **User Selection**: User chooses version to install
3. **Download**: Firmware downloaded from `https://raw.githubusercontent.com/{repo}/{branch}/versions/bronco_v{VERSION}.bin`
4. **Installation**: 
   - OTA partition prepared
   - Firmware written in chunks with progress updates
   - MD5 verification (if available)
   - Boot partition marked
5. **Reboot**: Device restarts with new firmware
6. **Auto-Detection**: New version detected on boot and saved to config

### Progress Monitoring

**Display UI:**
- Full-screen overlay with progress bar
- Percentage display
- Version being installed
- Automatic during installation

**Web UI:**
- Status messages
- Progress updates via polling
- Auto-refresh after completion

### Safety Features

- **Minimum Brightness**: Settings screen always visible (25% minimum)
- **Watchdog Feeding**: Aggressive during download to prevent timeout
- **Size Verification**: Content length checked before download
- **MD5 Verification**: Optional checksum validation
- **Dual Partition**: OTA uses app1 partition, app0 remains intact until successful boot
- **Fallback**: If new firmware fails, ESP32 boots back to previous version

## Configuration

### OTA Manager Settings

**In `config_manager.h` / device config:**
```json
{
  "ota": {
    "enabled": true,
    "manifest_url": "https://example.com/manifest.json",
    "channel": "stable"
  }
}
```

**GitHub Token** (for private repos):
- Edit `src/ota_manager.cpp`
- Update `kGitHubToken` constant
- Update `kGitHubApiUrl` and `kGitHubRawBase` if repo changes

### Version Header

**Auto-generated `src/version_auto.h`:**
```cpp
#pragma once
// Auto-generated on 2026-02-03T06:45:06.697954Z
constexpr const char* APP_VERSION = "2.1.4";
```

**Never edit manually** - always use backup tool to increment version!

## File Structure

```
Bronco-Controls/
├── .version_state.json          # Version tracking state
├── backup_restore_manager.py    # Core backup/restore logic
├── backup_restore_gui.py        # GUI application
├── backups/                     # All device backups
│   ├── bronco_v2.1.4_20260203_123456/
│   │   ├── backup_metadata.json
│   │   ├── bootloader.bin
│   │   ├── partitions.bin
│   │   ├── nvs.bin
│   │   ├── otadata.bin
│   │   ├── app0.bin
│   │   ├── app1.bin
│   │   ├── spiffs.bin
│   │   ├── full_flash_16MB.bin
│   │   └── RESTORE.bat
│   └── bronco_v3.0.0_20260203_145623_FULL/  # Major release
├── src/
│   ├── version_auto.h           # Auto-generated version
│   ├── ota_manager.cpp/.h       # OTA update logic
│   ├── web_server.cpp           # Web API endpoints
│   ├── web_interface.h          # Web UI HTML
│   └── ui_builder.cpp           # Display UI
└── versions/                    # (Optional local mirror of GitHub)
```

## Workflow Examples

### Scenario 1: Bug Fix Release
1. Fix bug in code
2. Run GUI: Select "Build (x.x.N)"
3. Click "Create Backup"
4. Version: 2.1.4 → 2.1.5
5. Normal backup created
6. Optional: Upload .bin to GitHub manually

### Scenario 2: Feature Release
1. Add new feature
2. Run GUI: Select "Minor (x.N.0)"
3. Click "Create Backup"
4. Version: 2.1.5 → 2.2.0
5. Normal backup created
6. Optional: Upload .bin to GitHub manually

### Scenario 3: Major Release
1. Breaking changes / major milestone
2. Run GUI: Select "MAJOR (N.0.0) + GIT PUSH"
3. Confirm major release dialog
4. Version: 2.2.0 → 3.0.0
5. **Automatic**:
   - Full device backup created
   - All source committed: `git add . && git commit -m "Major release v3.0.0 - Full device backup"`
   - Pushed to remote: `git push`
6. Upload `versions/bronco_v3.0.0.bin` to GitHub
7. Users can now OTA update

### Scenario 4: OTA Update (User Side)
1. **Web**: Open device web interface
   - Settings tab → "Check Updates"
   - Select v3.0.0 → "Install"
2. **Display**: Tap settings icon
   - Updates → "Check for Updates"
   - Select v3.0.0 → Confirm
3. **Process**: Download (~2 min) → Install → Reboot
4. **Verification**: Check version in Settings

## Troubleshooting

### Backup Issues

**Problem**: Backup fails with timeout
- **Solution**: Increase baud rate in GUI or use `--baud 921600`

**Problem**: Git push fails
- **Solution**: Ensure git credentials configured, repository accessible

**Problem**: Version not incrementing
- **Solution**: Delete `.version_state.json` and run backup again to reset

### OTA Issues

**Problem**: "No versions found" on GitHub check
- **Solution**: Ensure .bin files uploaded to `versions/` folder with correct naming

**Problem**: OTA download fails
- **Solution**: 
  - Check WiFi connection
  - Verify GitHub token (private repos)
  - Check file exists at expected URL

**Problem**: OTA installation hangs
- **Solution**: Watchdog timeout - check firmware size, increase timeout

**Problem**: Device doesn't boot after OTA
- **Solution**: ESP32 should auto-rollback. If not, restore from USB backup

### Version Issues

**Problem**: Version shows as "1.3.78" instead of current
- **Solution**: `.version_state.json` outdated, delete and recreate

**Problem**: GUI shows wrong next version
- **Solution**: Click radio buttons to refresh preview

## Advanced Usage

### Custom Version Increment (Python)
```python
from backup_restore_manager import BackupRestoreManager

manager = BackupRestoreManager()

# Major release
backup_folder, version = manager.backup_device(increment_type='major')
manager.git_push_all(version)

# Minor release
backup_folder, version = manager.backup_device(increment_type='minor')

# Build increment
backup_folder, version = manager.backup_device(increment_type='build')
```

### Manual Firmware Upload
```powershell
# Build firmware
pio run -e waveshare_7in

# Backup binary is at:
.pio/build/waveshare_7in/firmware.bin

# Rename and upload to GitHub:
cp .pio/build/waveshare_7in/firmware.bin versions/bronco_v{VERSION}.bin
git add versions/bronco_v{VERSION}.bin
git commit -m "Add OTA firmware v{VERSION}"
git push
```

### Restore from Backup
```powershell
# Method 1: GUI
python backup_restore_gui.py
# → Restore tab → Select backup → Restore

# Method 2: Batch Script
cd backups/bronco_v2.1.4_20260203_123456/
RESTORE.bat

# Method 3: Python
python backup_restore_manager.py restore --backup "backups/bronco_v2.1.4_20260203_123456"
```

## Best Practices

1. **Always backup before major changes**
2. **Use major releases for breaking changes only**
3. **Test OTA updates on development device first**
4. **Keep at least 3 backup versions**
5. **Document changes in commit messages**
6. **Verify firmware size before OTA (<4MB recommended)**
7. **Use GitHub releases for version tags**
8. **Test restore process periodically**

## Future Enhancements

- [ ] Automatic firmware.bin upload to GitHub on major release
- [ ] Release notes display in OTA UI
- [ ] Delta updates (smaller download size)
- [ ] A/B partition status display
- [ ] Backup compression
- [ ] Cloud backup storage (AWS S3, Google Drive)
- [ ] Multi-device fleet management
- [ ] Rollback to previous version from UI

## Support

For issues or questions:
1. Check this documentation
2. Review `backup_restore_gui.py` console output
3. Check Serial Monitor during OTA (115200 baud)
4. Review backup metadata JSON files
5. Test with full backup/restore cycle

---

**Version**: 1.0  
**Last Updated**: February 3, 2026  
**Current Firmware**: v2.1.4
