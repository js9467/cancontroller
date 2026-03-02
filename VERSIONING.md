# Versioning System

## Overview

The versioning system is designed to prevent loops and ensure consistent version tracking across builds and OTA updates.

## How It Works

### Single Source of Truth: `.version_state.json`

```json
{
  "major": 1,
  "minor": 3,
  "build": 66
}
```

This file contains the current version and is the ONLY file you should manually edit for version changes.

### Build Process

1. **PlatformIO Build** triggers `tools/versioning.py` (configured in `platformio.ini`)
2. **versioning.py** reads `.version_state.json`
3. **Generates** `src/version_auto.h` with the version string
4. **Firmware** is compiled with this version embedded

## Deployment Process

See [OTA_PUBLISHING.md](OTA_PUBLISHING.md) for the complete step-by-step process.

In short:
1. Increment `.version_state.json`
2. `pio run -e waveshare_7in`
3. Copy `firmware.bin` to `..\master-wt\versions\bronco_vX.Y.Z.bin`
4. Commit and push both repos

Devices will pick up the new version the next time they check GitHub.


## OTA Update Flow

1. Device queries GitHub API for `bronco_vX.Y.Z.bin` files in the `versions/` folder
2. Finds the highest available version and compares it to its own `APP_VERSION`
3. If GitHub has a newer version the device downloads and installs it directly from GitHub raw
4. After update, device reboots with new firmware


## Why This Prevents Loops

**The Problem (Before Fix):**
- `.version_state.json` stuck at 66
- Builds kept using 66 even when trying to update to 67
- Device downloaded "new" firmware but it was still 66
- Loop: check → download → reboot → still 66 → check again...

**The Solution (After Fix):**
- `.version_state.json` is the single source of truth
- `deploy.ps1` increments it BEFORE building
- Firmware gets new version baked in
- OTA comparison works correctly
- No more loops!

## Manual Version Changes

If you need to manually set a version:

1. Edit `.version_state.json`:
   ```json
   {
     "major": 1,
     "minor": 5,
     "build": 0
   }
   ```

2. Run a build to regenerate `version_auto.h`:
   ```powershell
   pio run -e waveshare_7in
   ```

3. Deploy if needed:
   ```powershell
   .\deploy.ps1 -VersionBump patch
   ```

## Files in the System

| File | Purpose | Edit? |
|------|---------|-------|
| `.version_state.json` | Single source of truth for version | ✅ Manually before each release |
| `src/version_auto.h` | Generated header file | ❌ Auto-generated |
| `tools/versioning.py` | Reads state, generates header | ❌ Don't modify |
| `OTA_PUBLISHING.md` | Complete release process | ✅ Reference only |

## Troubleshooting

### "Version stuck, not incrementing"

1. Check `.version_state.json` - is it the version you expect?
2. Use `deploy.ps1` to bump version (don't edit files directly)
3. Verify build output shows new version

### "OTA update loop"

1. Check device version: Settings → About
2. Check that GitHub has a newer binary: `Invoke-RestMethod "https://api.github.com/repos/js9467/cancontroller/contents/versions" | Select -ExpandProperty name`
3. Ensure listed version > device version
4. If stuck, publish a new version following OTA_PUBLISHING.md

### "Git conflicts on version files"

Always pull before deploying. If conflicts occur:
1. Accept incoming `.version_state.json`
2. Discard changes to `src/version_auto.h` (it's auto-generated)
3. Run build to regenerate header

## Verification & Safety

Before deploying, always run the verification script:

```powershell
.\verify_ota_package.ps1 -Version "1.3.68"
```

This checks:
1. ✓ `.version_state.json` has correct version
2. ✓ `version_auto.h` matches state file
3. ✓ Firmware binary exists
4. ✓ OTA package directory is complete
5. ✓ All manifests are consistent with correct MD5

The `deploy.ps1` script now runs this automatically before deploying to Fly.io.

## Best Practices

✅ **DO:**
- Increment `.version_state.json` before every release build
- Commit `.version_state.json` to git
- Let the system auto-generate `version_auto.h`
- Copy the built `firmware.bin` to `master-wt/versions/bronco_vX.Y.Z.bin` and push
- See [OTA_PUBLISHING.md](OTA_PUBLISHING.md) for the full checklist

❌ **DON'T:**
- Manually edit `src/version_auto.h`
- Modify `tools/versioning.py` without understanding the system
- Deploy without incrementing version
- Skip git commits when deploying

## Summary

**Single source of truth:** `.version_state.json`  
**OTA source:** `master-wt/versions/bronco_vX.Y.Z.bin` (GitHub raw)  
**Full process:** [OTA_PUBLISHING.md](OTA_PUBLISHING.md)
