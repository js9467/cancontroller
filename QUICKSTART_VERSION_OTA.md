# Quick Start: Version Management & OTA Updates

## Easiest Way: Double-Click BackupRestore.bat

**Windows Users:**
1. Double-click `BackupRestore.bat` in the Bronco-Controls folder
2. GUI launches automatically
3. Select version type (Build/Minor/MAJOR)
4. Click "Create Backup"
5. Done!

First time? The batch file auto-installs Python dependencies.

---

## For Developers: Creating a New Release

### Normal Update (Build/Minor)
1. Make your code changes
2. Build firmware: `pio run -e waveshare_7in`
3. **Double-click `BackupRestore.bat`** (or run `python backup_restore_gui.py`)
4. Select version type:
   - **Build**: Bug fixes (2.1.4 → 2.1.5)
   - **Minor**: New features (2.1.4 → 2.2.0)
5. Click "Create Backup"
6. System automatically copies firmware.bin to `versions/` folder
7. Done! Backup saved to `backups/` folder

### Major Release (Automatic Git Push)
1. Make significant changes
2. Build firmware: `pio run -e waveshare_7in`
3. **Double-click `BackupRestore.bat`**
4. Select "**MAJOR (N.0.0) + GIT PUSH**"
5. Click "Create Backup"
6. Confirm the dialog
7. **Automatic**:
   - Version: 2.1.4 → 3.0.0
   - Full backup created
   - Firmware.bin copied to versions/
   - All source code → Git
   - Changes pushed to repository

### Publishing OTA Updates
After backup completes, the console shows:
```
[OTA] ✓ Firmware copied (1.23 MB)
[OTA] Ready for OTA distribution

[Next Steps]
  1. Test the firmware on a device first
  2. Upload to GitHub: git add versions/bronco_v2.1.5.bin
  3. Commit: git commit -m 'Add OTA firmware v2.1.5'
  4. Push: git push
  5. Users can now OTA update to v2.1.5
```

Just follow those steps:
```powershell
git add versions/bronco_v2.1.5.bin
git commit -m "Add OTA firmware v2.1.5"
git push
```

Now users can OTA update!

## For Users: Updating Your Device

### Via Web Interface (Recommended)
1. Connect to device WiFi or web interface
2. Navigate to **Settings** tab
3. Click **"Check Updates"**
4. Select desired version from dropdown
5. Click **"Install Selected Version"**
6. Wait 2-3 minutes - device will reboot automatically
7. Done! New version installed

### Via Device Display
1. Tap **Settings icon** (top-right corner)
2. Scroll to **"Updates"** section
3. Tap **"Check for Updates"**
4. Select version (if multiple available)
5. Tap **Install** and confirm
6. Wait for download and installation
7. Device reboots with new firmware

## Restoring from Backup

### GUI Method
1. Run: `python backup_restore_gui.py`
2. Go to **Restore** tab
3. Select backup from list
4. Click **"Restore Selected"**
5. Confirm warning dialog
6. Wait 5-10 minutes
7. Device fully restored!

### Batch Script Method
1. Navigate to backup folder: `backups/bronco_v2.1.4_20260203_123456/`
2. Run: `RESTORE.bat`
3. Follow prompts
4. Device fully restored!

## Troubleshooting

### OTA Update Failed
- **Check WiFi**: Ensure device connected to internet
- **Try Again**: Click "Check Updates" and retry
- **USB Restore**: Use backup restore if OTA fails repeatedly

### Backup Failed
- **Check USB Connection**: Ensure device connected to COM5 (or update port)
- **Try Different Port**: Change port in GUI if COM5 not working
- **Increase Timeout**: Add `--baud 460800` for faster transfer

### Version Not Updating
- **Delete State**: Remove `.version_state.json` and create new backup
- **Check Header**: Verify `src/version_auto.h` shows correct version
- **Rebuild**: Run `pio run -e waveshare_7in` to rebuild with new version

## Key Files

- **`BackupRestore.bat`** - **DOUBLE-CLICK THIS to launch GUI**
- `backup_restore_gui.py` - GUI application (auto-launched by .bat)  
- `backup_restore_manager.py` - Core backup logic  
- `.version_state.json` - Version tracking (auto-generated)
- `src/version_auto.h` - Version header (auto-generated)
- `backups/` - All device backups (USB restore)
- `versions/` - OTA firmware .bin files (push to GitHub)

## Quick Commands

```powershell
# EASIEST: Just double-click
BackupRestore.bat

# Or run Python directly
python backup_restore_gui.py

# Command line backup
python backup_restore_manager.py backup

# List backups
python backup_restore_manager.py list

# Restore latest
python backup_restore_manager.py restore

# Full test cycle
python backup_restore_manager.py test

# Build firmware
pio run -e waveshare_7in -t upload
```

## What Happens During Major Release?

1. ✅ Version incremented (e.g., 2.1.4 → 3.0.0)
2. ✅ Full device backup created (all flash regions + metadata)
3. ✅ Source code committed to Git
4. ✅ Changes pushed to remote repository
5. ✅ Restore script generated
6. ✅ Ready for OTA distribution

## What Users See During OTA Update?

1. 📡 "Checking for updates..."
2. ✅ "Found X versions available"
3. 🎯 Select version from dropdown
4. ⬇️ "Downloading firmware..."
5. 📊 Progress bar (0-100%)
6. ⚙️ "Installing firmware..."
7. 🔄 "Restarting device..."
8. ✨ New version running!

---

For detailed documentation, see **VERSION_OTA_SYSTEM.md**
