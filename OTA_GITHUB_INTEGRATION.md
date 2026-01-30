# GitHub OTA Integration - Implementation Summary

## Overview
The OTA (Over-The-Air) update system has been updated to use GitHub as the source for firmware versions instead of the old manifest-based system.

## Changes Made

### Device UI (ESP32 Touch Screen)
**File:** `src/ui_builder.cpp`
- **OTA Button Behavior:** The "Check for Updates" button in the Settings modal now queries GitHub for available firmware versions
- **Check Action:** Calls `checkGitHubVersions()` to get list of versions from GitHub repository
- **Install Action:** Calls `installVersionFromGitHub()` with the selected version
- **Version Comparison:** Compares latest GitHub version with current `APP_VERSION` to determine if update is available
- **Status Updates:** Shows appropriate messages on screen:
  - "checking-github" - while querying GitHub
  - "update-available-{version}" - when newer version found
  - "up-to-date" - when current version is latest
  - "github-check-failed" - if GitHub query fails

**File:** `src/ui_builder.h`
- **New Member Variable:** `latest_github_version_` - stores the version selected for installation

### Web UI (Browser Interface)
**File:** `src/web_interface.h`

#### `checkForUpdates()` Function
**Old Behavior:**
- Called `/api/ota/check` (manifest-based)

**New Behavior:**
- Calls `/api/ota/github/versions` endpoint
- Gets JSON response with list of available versions from GitHub
- Compares `data.versions[0]` (latest) with `data.current`
- Shows update button with version number if update available
- Stores version in button's `data-version` attribute

#### `triggerOTAUpdate()` Function
**Old Behavior:**
- Posted to `/api/ota/update` with no body
- Device would fetch from manifest URL

**New Behavior:**
- Posts to `/api/ota/github/install` with JSON body
- Body includes: `{ "version": "x.x.x" }`
- Device downloads firmware from GitHub and performs OTA update
- Reload timeout increased to 10 seconds (OTA download takes longer)

## API Endpoints Used

### Device → GitHub
The ESP32 device uses these methods (already implemented in `ota_manager.cpp`):
- `checkGitHubVersions(std::vector<std::string>& versions)` - List all versions
- `installVersionFromGitHub(const std::string& version)` - Download and install specific version

### Web UI → Device API
The web interface calls these REST endpoints:
- `GET /api/ota/github/versions` - Get list of available versions from GitHub
  - Response: `{ "status": "ok", "versions": ["1.3.87", "1.3.86"], "current": "1.3.87", "count": 2 }`
- `POST /api/ota/github/install` - Trigger OTA update with specific version
  - Request: `{ "version": "1.3.88" }`
  - Response: `{ "status": "ok", "message": "Installing version 1.3.88" }`

## GitHub Repository Structure
The system expects firmware versions in this format:
```
https://github.com/js9467/cancontroller/tree/master/versions/
├── bronco_v1.3.87/
│   └── firmware.bin
├── bronco_v1.3.88/
│   └── firmware.bin
└── bronco_v1.3.88_FULL/
    └── firmware.bin
```

The OTA system tries both:
1. `bronco_v{version}/firmware.bin` (regular version update)
2. `bronco_v{version}_FULL/firmware.bin` (full backup version)

## User Experience Flow

### On Device (Touch Screen)
1. User taps Settings icon (invisible hotspot in top-right corner)
2. Settings modal opens, shows "Updates" section
3. User sees current version and "Check for Updates" button
4. User taps "Check for Updates"
   - Device queries GitHub API
   - If update available, button changes to "Install Update v{version}"
   - If up to date, status shows "Up to date"
5. User taps "Install Update" button
   - Device downloads firmware from GitHub
   - Progress bar shows download progress
   - Device automatically reboots when complete

### On Web UI (Browser)
1. User navigates to device IP in browser
2. Opens "Device Info & Updates" section
3. User sees current firmware version
4. User clicks "Check Updates" button
   - Browser calls GitHub versions API
   - If update available, shows green "Update to v{version}" button
   - If up to date, shows "Firmware is up to date" banner
5. User clicks "Update to v{version}" button
   - Confirms in dialog
   - Browser triggers GitHub OTA install
   - Device downloads and installs
   - Page auto-reloads after 10 seconds

## Testing Checklist

Once the new firmware is flashed:
- [ ] Connect to device WiFi or web interface
- [ ] Test device UI: Tap Settings → Updates → Check for Updates
- [ ] Verify it shows correct status (up to date or update available)
- [ ] Test web UI: Click "Check Updates" button
- [ ] Verify it queries GitHub and shows correct version info
- [ ] If update available, test "Install" button (creates backup first!)

## Next Steps

After flashing this firmware (v1.3.88):
1. Test OTA functionality on device
2. Verify GitHub version check works
3. Use Version Update GUI tool to create v1.3.89
4. Test OTA update from v1.3.88 → v1.3.89
5. Verify device automatically downloads from GitHub and updates

## Backward Compatibility

The old manifest-based endpoints still exist (`/api/ota/check`, `/api/ota/update`) but are no longer used by the UI. They can be removed in a future version once GitHub OTA is proven stable.
