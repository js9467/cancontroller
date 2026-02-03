# Bronco Controls - Version Management & OTA Updates

## Quick Start

### For Users: Just Double-Click!

**Windows Users:**
1. Double-click `BackupRestore.bat`
2. GUI launches automatically
3. Select version type (Build/Minor/MAJOR)
4. Click "Create Backup"
5. Done!

**First time?** The batch file will automatically install Python dependencies.

### What the Batch File Does

```batch
BackupRestore.bat
  ↓
Checks Python installed
  ↓
Installs dependencies (first run only)
  ↓
Launches backup_restore_gui.py
  ↓
GUI application starts
```

---

## OTA Firmware Distribution

### Important: .bin vs .zip Files

**For OTA Updates (Over-The-Air):**
- Use `.bin` files → Fast wireless installation
- Location: `versions/bronco_v{VERSION}.bin`
- Uploaded to GitHub for device access

**For USB Backups:**
- Full backups saved in `backups/` folder
- Contains all flash regions + metadata
- Used for complete device restore via USB

### After Creating a Backup

The system now automatically:
1. ✅ Creates backup in `backups/` folder
2. ✅ Copies `firmware.bin` to `versions/` folder with correct naming
3. ✅ Shows you the git commands to push it to GitHub

**Manual Steps (after backup completes):**

```powershell
# 1. Verify firmware.bin was copied
dir versions\

# 2. Upload to GitHub for OTA distribution
git add versions/bronco_v{VERSION}.bin
git commit -m "Add OTA firmware v{VERSION}"
git push

# 3. Users can now OTA update!
```

**For Major Releases:**
- Source code is automatically committed and pushed
- You still need to manually push the firmware.bin

---

## Version Types

**Build (x.x.N)** - Bug fixes
- Example: 2.1.4 → 2.1.5
- Normal backup created
- Firmware copied to versions/

**Minor (x.N.0)** - New features
- Example: 2.1.5 → 2.2.0
- Normal backup created
- Firmware copied to versions/

**MAJOR (N.0.0) + GIT PUSH** - Breaking changes
- Example: 2.2.0 → 3.0.0
- **FULL device backup** created
- **All source code** committed and pushed to Git
- Firmware copied to versions/
- Requires manual confirmation

---

## Troubleshooting

### "Python not found"
Install Python from https://python.org (3.8 or later)

### "backup_restore_gui.py not found"
Run `BackupRestore.bat` from the Bronco-Controls folder

### "No module named 'requests'"
Delete `.deps_installed` file and run `BackupRestore.bat` again

### GUI won't start
Open PowerShell and run:
```powershell
python backup_restore_gui.py
```
Check error messages

### Firmware not copied to versions/
Build firmware first:
```powershell
pio run -e waveshare_7in
```
Then run backup again

---

## Files Overview

| File | Purpose |
|------|---------|
| `BackupRestore.bat` | **Double-click to launch GUI** |
| `backup_restore_gui.py` | GUI application (auto-launched by .bat) |
| `backup_restore_manager.py` | Core backup/restore logic |
| `.version_state.json` | Current version tracking |
| `src/version_auto.h` | Auto-generated version header |
| `backups/` | All device backups (USB restore) |
| `versions/` | OTA firmware .bin files (for GitHub) |

---

## Complete Workflow

### Developer: Creating a Release

1. **Make code changes**
2. **Build firmware:**
   ```powershell
   pio run -e waveshare_7in
   ```
3. **Create backup:** Double-click `BackupRestore.bat`
4. **Select version type** (Build/Minor/MAJOR)
5. **Click "Create Backup"**
6. **System automatically:**
   - Increments version
   - Creates full device backup
   - Copies firmware.bin to versions/
   - (If MAJOR) Commits and pushes source code to Git
7. **You manually push firmware:**
   ```powershell
   git add versions/bronco_v{VERSION}.bin
   git commit -m "Add OTA firmware v{VERSION}"
   git push
   ```
8. **Users can now OTA update!**

### User: Updating Device

**Via Web Interface:**
1. Open device web UI
2. Settings tab → "Check Updates"
3. Select version → "Install"
4. Wait 2-3 minutes
5. Device reboots with new firmware

**Via Device Display:**
1. Tap Settings icon (top-right)
2. Updates section → "Check for Updates"
3. Select version → Confirm
4. Device downloads and installs
5. Auto-reboot

---

For detailed documentation, see:
- `VERSION_OTA_SYSTEM.md` - Complete system documentation
- `QUICKSTART_VERSION_OTA.md` - Quick reference guide
