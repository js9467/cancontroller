# GitHub Backup Integration

## Overview

The backup/restore system now integrates with GitHub to automatically upload full device backups and make them available for remote restoration.

## How It Works

### Major Version Releases

When you create a **MAJOR** version release:

1. **Full Device Backup** - All flash regions backed up locally to `backups/` folder
2. **Firmware Build** - Firmware compiled with PlatformIO  
3. **Firmware Copy** - `.bin` file copied to `versions/` folder
4. **Git Push** - All changes pushed to GitHub repository
5. **GitHub Release Creation** - Automated GitHub release created with:
   - Release tag: `v{major}.{minor}.{build}`
   - Title: `Bronco Controls v{version} - Full Backup`
   - Asset: Complete backup as `.zip` file (e.g., `bronco_v2.1.0_FULL_BACKUP.zip`)

### Minor/Build Version Releases

Minor and build releases only:
- Build firmware
- Copy to `versions/` folder
- Push to Git
- **NO** full backup or GitHub release

## Backup Sources

The restore utility now shows backups from **two sources**:

### LOCAL Backups
- Stored in `backups/` folder on your PC
- Fast access
- No download required
- Created during MAJOR releases

### GITHUB Backups  
- Stored as GitHub releases
- Available anywhere with internet
- Automatically downloaded when restoring
- Ideal for remote devices

## Using GitHub Backups

### In the GUI

1. Open the **Restore** tab
2. Click **🔄 Refresh List**
3. Backups will show with source:
   - `[LOCAL]` - On your PC
   - `[GITHUB]` - From GitHub releases

4. Select any backup (local or GitHub)
5. Click **♻️ Restore Selected**
6. For GitHub backups:
   - Download progress shown
   - Extraction handled automatically
   - Then restored to device

### Command Line

```bash
# List all backups (local + GitHub)
python backup_restore_manager.py list

# Restore from local backup
python backup_restore_manager.py restore --backup "backups/bronco_v2.1.0_LOCKED"

# Restore from GitHub (use download URL)
python backup_restore_manager.py restore --backup "https://github.com/js9467/cancontroller/releases/download/v2.1.0/bronco_v2.1.0_FULL_BACKUP.zip"
```

## GitHub Configuration

The system uses:
- **Repository**: `js9467/cancontroller`
- **Branch**: `master`
- **Token**: Configured in `backup_restore_manager.py`

### Updating GitHub Token

If you need to update the token:

1. Generate new token at: https://github.com/settings/tokens
2. Required scopes: `repo` (full control)
3. Update in [backup_restore_manager.py](backup_restore_manager.py):

```python
self.github_token = 'ghp_YOUR_NEW_TOKEN_HERE'
```

## Benefits

### Remote Device Management
- Device at home, you're remote? No problem!
- Download backup from GitHub
- Restore via WiFi/OTA or USB when home

### Backup Redundancy
- Local backups on PC
- Cloud backups on GitHub
- Never lose your device configurations

### Version History
- All major releases preserved
- Easy rollback to any previous version
- Complete flash dumps for each version

## File Structure

### GitHub Release Asset

Each GitHub release includes a `.zip` file containing:
```
bronco_v2.1.0_FULL_BACKUP.zip
└── bronco_v2.1.0_LOCKED/
    ├── backup_metadata.json
    ├── bootloader.bin
    ├── partitions.bin
    ├── nvs.bin
    ├── otadata.bin
    ├── app0.bin
    ├── app1.bin
    ├── spiffs.bin
    └── coredump.bin
```

### Metadata Example

```json
{
  "version": "2.1.0",
  "backup_date": "2026-01-30T13:27:50",
  "device": "ESP32-S3-Touch-LCD-7",
  "flash_size": "16MB",
  "regions": {
    "bootloader": {"offset": "0x0", "size": "0x8000", "file": "bootloader.bin"},
    "nvs": {"offset": "0x9000", "size": "0x5000", "file": "nvs.bin"},
    ...
  }
}
```

## Troubleshooting

### No GitHub Backups Showing

1. Check internet connection
2. Verify GitHub token is valid
3. Check repository has releases with backup assets
4. Look for errors in console output

### Download Failed

1. Check GitHub token permissions
2. Verify release exists: https://github.com/js9467/cancontroller/releases
3. Check available disk space
4. Try again (temporary network issue)

### Upload Failed During Backup

1. Check GitHub token has `repo` scope
2. Verify you have write access to repository
3. Check if release tag already exists
4. Review console error messages

## Example Workflow

### Creating a Major Release

```bash
# Via GUI:
1. Open Backup tab
2. Select "Major" version type
3. Click "Create Backup"
4. Wait for completion (includes GitHub upload)

# Via CLI:
python backup_restore_manager.py backup --major
```

### Restoring from GitHub (Remote Device)

```bash
# Via GUI:
1. Open Restore tab  
2. Refresh list (shows GitHub backups)
3. Select GitHub backup
4. Click Restore
5. Download + restore happens automatically

# Via CLI:
python backup_restore_manager.py restore --backup "https://github.com/js9467/cancontroller/releases/download/v2.1.0/bronco_v2.1.0_FULL_BACKUP.zip"
```

## Security Notes

- GitHub token stored in source code (for automation)
- Token has full repository access
- **Never commit token to public repositories**
- Rotate token periodically for security
- Consider using environment variables for production

## Future Enhancements

Potential improvements:
- [ ] Encrypted backups with password protection
- [ ] Selective restore (choose specific partitions)
- [ ] Backup compression optimization
- [ ] GitHub Actions for automated testing
- [ ] Multi-repository support
- [ ] Backup retention policies
