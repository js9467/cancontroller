# Standardized Deployment Process - READ ME FIRST

## The Problem We're Solving

Version management was breaking because different AI agents were:
- Manually editing `src/version_auto.h`
- Using PowerShell `ConvertTo-Json` which adds bad formatting
- Creating inconsistent manifest.json files
- Skipping proper git commits
- Not following a standardized process

## The Solution

**Single source of truth**: `deploy.ps1` script that handles ALL deployments

## How to Deploy (For Humans and AI Agents)

### Standard Deployment
```powershell
.\deploy.ps1 -Changelog "Description of what changed"
```

That's it! The script will:
1. ✅ Bump patch version (1.3.60 → 1.3.61)
2. ✅ Update src/version_auto.h with proper formatting
3. ✅ Build firmware
4. ✅ Create OTA release directory
5. ✅ Copy firmware.bin
6. ✅ Generate properly formatted manifest.json (NO PowerShell artifacts!)
7. ✅ Commit to git with descriptive message
8. ✅ Push to GitHub
9. ✅ Deploy to Fly.io OTA server
10. ✅ Provide clear summary

### Custom Version Bumps
```powershell
# Minor version (1.3.60 → 1.4.0)
.\deploy.ps1 -VersionBump minor -Changelog "New feature added"

# Major version (1.3.60 → 2.0.0)  
.\deploy.ps1 -VersionBump major -Changelog "Breaking changes"
```

### Local Testing Only
```powershell
# Build and update version, but don't deploy OTA or commit to git
.\deploy.ps1 -LocalOnly -Changelog "Testing changes"
```

### Skip Build (firmware already built)
```powershell
.\deploy.ps1 -SkipBuild -Changelog "Just deploying existing build"
```

## After Deployment

### Trigger OTA Update on Device
```powershell
# Trigger the update
Invoke-RestMethod -Uri "http://192.168.7.116/api/ota/update" -Method POST

# Wait 60 seconds for download and install
Start-Sleep -Seconds 60

# Verify new version
Invoke-RestMethod -Uri "http://192.168.7.116/api/status" | Select firmware_version
```

### USB Upload (If OTA fails)
```powershell
# Check available COM ports
[System.IO.Ports.SerialPort]::GetPortNames()

# Upload
pio run -e waveshare_7in --target upload --upload-port COM6
```

## Rules for AI Agents

**CRITICAL**: These files are MANAGED by `deploy.ps1` - NEVER edit them directly:
- ❌ `src/version_auto.h`
- ❌ `ota_functions/manifest.json`  
- ❌ `ota_functions/releases/*/manifest.json`

**ALWAYS** use `deploy.ps1` for any code changes that need deployment.

**NEVER** use PowerShell `ConvertTo-Json` for manifest files - it creates invalid formatting.

## Why This Matters

### Before (Broken Process)
```powershell
# AI Agent A does this:
$version = "1.3.61"
Set-Content "src/version_auto.h" "const char* APP_VERSION = `"$version`";"

# AI Agent B does this:
$manifest = @{version="1.3.61"} | ConvertTo-Json  # WRONG! Bad formatting
Set-Content "manifest.json" $manifest

# Result: Version mismatch, broken OTA, deployment fails
```

### After (Correct Process)
```powershell
# ALL AI Agents do this:
.\deploy.ps1 -Changelog "What I changed"

# Result: Consistent, working deployments every time
```

## Common Scenarios

### Scenario 1: Fixed a Bug
```powershell
# Make code changes, then:
.\deploy.ps1 -Changelog "Fix CAN frame timeout issue"
```

### Scenario 2: Added New Feature
```powershell
.\deploy.ps1 -VersionBump minor -Changelog "Add InfinityBox diagnostics endpoint"
```

### Scenario 3: Testing Locally
```powershell
# Build and test without deploying
.\deploy.ps1 -LocalOnly -Changelog "Testing new UI layout"
```

### Scenario 4: Emergency Hotfix
```powershell
# Fix code, then immediate deploy:
.\deploy.ps1 -Changelog "CRITICAL: Fix memory leak in web server"

# Then immediately trigger OTA:
Invoke-RestMethod -Uri "http://192.168.7.116/api/ota/update" -Method POST
```

## Troubleshooting

### "Build failed"
- Check PlatformIO is installed
- Run `pio run -e waveshare_7in` to see full error
- Fix code issues, then run deploy.ps1 again

### "Git push failed"
- Check network connection
- Verify git credentials
- Run `git status` to check state

### "Fly deploy failed"
- Check Fly.io status
- Verify flyctl is logged in: `flyctl auth whoami`
- Check logs: `flyctl logs -a image-optimizer-still-flower-1282`

### "OTA not updating device"
- Verify manifest deployed: `Invoke-RestMethod 'https://image-optimizer-still-flower-1282.fly.dev/ota/manifest'`
- Check device can reach server: `Invoke-RestMethod 'http://192.168.7.116/api/status'`
- Manually trigger: `Invoke-RestMethod -Uri "http://192.168.7.116/api/ota/update" -Method POST`
- Last resort: USB upload

## Files Reference

- `deploy.ps1` - **The only way to deploy** (use this!)
- `.ai-instructions.md` - Detailed AI agent instructions
- `.cursorrules` - Cursor IDE specific rules
- `INFINITYBOX_TEST_INSTRUCTIONS.md` - InfinityBox CAN testing guide
- `test_infinitybox.ps1` - J1939 frame test script

## Summary

**Golden Rule**: If you're changing code that needs deployment, run `.\deploy.ps1`

That's it. Follow this, and versioning stays consistent.
