# Quick Start Guide - Backup & Restore

## ✅ First Backup Created!

Your device has been successfully backed up to:
```
backups\bronco_v1.3.79_20260130_110229\
```

Version incremented: **1.3.78 → 1.3.79**

---

## 🎯 Next Steps

### Option 1: Test the Restore (Recommended)

Verify everything works with a full test cycle:

```powershell
python backup_restore_manager.py test
```

This will:
1. ✅ Create a new backup
2. 🗑️ Erase the device completely  
3. ♻️ Restore from backup
4. ✅ Verify device works

**Time:** ~10 minutes  
**Risk:** Low (backed up first)

### Option 2: Try the GUI

Launch the graphical interface:

```powershell
.\BackupRestore.bat
```

or directly:

```powershell
python backup_restore_gui.py
```

Features:
- 📦 Create backups with one click
- ♻️ Restore from any backup
- 🧪 Run test cycles
- 📊 View all backups in one place

### Option 3: Wipe & Restore Test

Manually test the backup:

```powershell
# 1. Erase the device
python -m esptool --chip esp32s3 --port COM5 erase_flash

# 2. Restore from backup
python backup_restore_manager.py restore
```

---

## 📤 Upload to GitHub

To automatically upload backups to GitHub:

1. **Create GitHub token:**
   - Go to: https://github.com/settings/tokens
   - Generate new token (classic)
   - Select: `repo` permission
   - Copy token

2. **Set environment variable (PowerShell):**
   ```powershell
   [Environment]::SetEnvironmentVariable("GITHUB_TOKEN", "your_token_here", "User")
   ```

3. **Create backup with upload:**
   ```powershell
   python backup_restore_manager.py backup --upload
   ```

Backups will be uploaded to:
```
https://github.com/js9467/autotouchscreen/tree/main/versions
```

---

## 🔧 Common Commands

**List all backups:**
```powershell
python backup_restore_manager.py list
```

**Create backup:**
```powershell
python backup_restore_manager.py backup
```

**Restore latest:**
```powershell
python backup_restore_manager.py restore
```

**Restore specific backup:**
```powershell
python backup_restore_manager.py restore --backup "backups\bronco_v1.3.79_20260130_110229"
```

**Use different port:**
```powershell
python backup_restore_manager.py backup --port COM6
```

---

## 📋 What's Backed Up?

✅ **Included in backup:**
- Firmware (application code)
- WiFi credentials & passwords
- CAN bus settings
- Display configuration
- Custom settings
- Uploaded images/assets
- File system data

❌ **Not needed:**
- Hardware-specific chip data (automatic)
- MAC address (chip-specific, unchangeable)

---

## 🆘 Troubleshooting

**"Failed to connect"**
- Check USB cable (must support data)
- Try different COM port
- Close Arduino IDE, PuTTY, or other serial programs

**"Permission denied"**
- Close other programs using the COM port
- Run as Administrator
- Restart computer

**Backup is slow**
- Normal! 16MB takes 5-10 minutes
- Don't interrupt the process

**Version not updating**
- Delete `.version_state.json` and `.pio/` folder
- Rebuild: `pio run -e waveshare_7in`

---

## 📁 Folder Structure

```
Bronco-Controls-4/
├── backups/                     # All your backups
│   └── bronco_v1.3.79_.../      # Each backup folder
│       ├── *.bin                # Flash regions
│       ├── RESTORE.bat          # Standalone restore
│       └── backup_metadata.json # Backup info
├── backup_restore_manager.py    # CLI tool
├── backup_restore_gui.py        # GUI tool
├── BackupRestore.bat            # Easy launcher
└── BACKUP_RESTORE_README.md     # Full documentation
```

---

## 🎉 Ready to Go!

Your backup system is fully configured and tested. You now have:

1. ✅ Full device backup (v1.3.79)
2. ✅ Auto-versioning system active
3. ✅ Restore capability ready
4. ✅ Standalone restore scripts
5. ✅ Command-line and GUI tools

**Recommended:** Run the full test cycle to verify everything:

```powershell
python backup_restore_manager.py test
```

**Questions?** See `BACKUP_RESTORE_README.md` for complete documentation.
