<#
.SYNOPSIS
Downloads the latest firmware from the OTA server and updates BroncoFlasher package.

.DESCRIPTION
This script fetches the latest firmware version from the OTA manifest server,
downloads the firmware binary, and updates the local BroncoFlasher deployment package.
It can also rebuild the BroncoFlasher.zip with the latest firmware included.

.PARAMETER OtaServer
The OTA server URL. Defaults to https://image-optimizer-still-flower-1282.fly.dev

.PARAMETER OutputPath
Where to save the updated firmware. Defaults to the current script directory.

.PARAMETER RebuildZip
If specified, rebuilds BroncoFlasher.zip with the updated firmware.

.EXAMPLE
.\update_firmware.ps1
Downloads latest firmware to the current directory

.EXAMPLE
.\update_firmware.ps1 -RebuildZip
Downloads latest firmware and rebuilds BroncoFlasher.zip
#>

[CmdletBinding()]
param(
    [string]$OtaServer = "https://image-optimizer-still-flower-1282.fly.dev",
    [string]$OutputPath = $PSScriptRoot,
    [switch]$RebuildZip,
    [string]$Channel = "stable"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Info {
    param([string]$Message)
    Write-Host "[update] $Message" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host "[update] $Message" -ForegroundColor Green
}

function Write-Error-Custom {
    param([string]$Message)
    Write-Host "[update] $Message" -ForegroundColor Red
}

# Fetch latest firmware manifest
Write-Info "Fetching latest firmware manifest from $OtaServer..."
try {
    $manifestUrl = "$OtaServer/ota/manifest"
    if ($Channel -and $Channel -ne "stable") {
        $manifestUrl += "?channel=$Channel"
    }
    
    $manifest = Invoke-RestMethod -Uri $manifestUrl -Method Get
    Write-Success "Found firmware version: $($manifest.version)"
    Write-Info "  Size: $($manifest.firmware.size) bytes"
    Write-Info "  MD5: $($manifest.firmware.md5)"
} catch {
    Write-Error-Custom "Failed to fetch manifest: $_"
    exit 1
}

# Download firmware binary
$firmwareUrl = $manifest.firmware.url
$firmwarePath = Join-Path $OutputPath "firmware.bin"

Write-Info "Downloading firmware from $firmwareUrl..."
try {
    Invoke-WebRequest -Uri $firmwareUrl -OutFile $firmwarePath -UseBasicParsing
    Write-Success "Firmware downloaded to: $firmwarePath"
} catch {
    Write-Error-Custom "Failed to download firmware: $_"
    exit 1
}

# Verify MD5 hash
Write-Info "Verifying firmware integrity..."
$downloadedHash = (Get-FileHash -Path $firmwarePath -Algorithm MD5).Hash.ToLower()
$expectedHash = $manifest.firmware.md5.ToLower()
if ($downloadedHash -ne $expectedHash) {
    Write-Error-Custom "MD5 mismatch!"
    Write-Error-Custom "  Expected: $expectedHash"
    Write-Error-Custom "  Got:      $downloadedHash"
    exit 1
}
Write-Success "Firmware integrity verified (MD5: $downloadedHash)"

# Save version info
$versionFile = Join-Path $OutputPath "firmware_version.txt"
$versionInfo = @"
Firmware Version: $($manifest.version)
Downloaded: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Source: $OtaServer
MD5: $($manifest.firmware.md5)
Size: $($manifest.firmware.size) bytes
"@
Set-Content -Path $versionFile -Value $versionInfo
Write-Success "Version info saved to: $versionFile"

# Rebuild BroncoFlasher.zip if requested
if ($RebuildZip) {
    Write-Info "Rebuilding BroncoFlasher.zip..."
    
    $zipPath = Join-Path $OutputPath "BroncoFlasher.zip"
    $tempExtract = Join-Path $OutputPath "temp_extract"
    
    # Remove old temp directory if it exists
    if (Test-Path $tempExtract) {
        Remove-Item $tempExtract -Recurse -Force
    }
    
    # Create temp directory
    New-Item -ItemType Directory -Path $tempExtract | Out-Null
    
    # Copy all required files
    $filesToInclude = @(
        "bootloader.bin",
        "partitions.bin", 
        "boot_app0.bin",
        "firmware.bin",
        "flash_device.ps1",
        "flash_device.cmd",
        "flash_device.sh",
        "firmware_version.txt",
        "BroncoFlasher.ps1",
        "BroncoFlasher.cmd",
        "Install.bat",
        "Install-Drivers.bat",
        "esp32-usb-driver.zip",
        "README.md"
    )
    
    foreach ($file in $filesToInclude) {
        $srcPath = Join-Path $OutputPath $file
        if (Test-Path $srcPath) {
            Copy-Item $srcPath -Destination $tempExtract
            Write-Info "  Added: $file"
        } else {
            Write-Warning "  Missing: $file (skipped)"
        }
    }
    
    # Remove old zip if exists
    if (Test-Path $zipPath) {
        Remove-Item $zipPath -Force
    }
    
    # Create new zip
    Compress-Archive -Path "$tempExtract\*" -DestinationPath $zipPath -Force
    Write-Success "BroncoFlasher.zip created: $zipPath"
    
    # Cleanup
    Remove-Item $tempExtract -Recurse -Force
    Write-Info "Cleaned up temporary files"
}

Write-Success "Firmware update complete!"
Write-Info ""
Write-Info "Next steps:"
Write-Info "  1. Test the firmware with: .\flash_device.cmd"
if ($RebuildZip) {
    Write-Info "  2. Upload BroncoFlasher.zip to GitHub releases"
    Write-Info "  3. Users can download from: https://github.com/js9467/autotouchscreen/raw/main/tools/deploy/BroncoFlasher.zip"
}
