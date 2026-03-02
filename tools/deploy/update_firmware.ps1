<#
.SYNOPSIS
Downloads the latest firmware from GitHub and optionally rebuilds BroncoFlasher package.

.DESCRIPTION
Fetches the latest bronco_vX.Y.Z.bin from the GitHub repository (js9467/cancontroller)
and saves it locally. Optionally rebuilds BroncoFlasher.zip with updated firmware.

.PARAMETER OutputPath
Where to save the downloaded firmware. Defaults to the current script directory.

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
    [string]$OutputPath = $PSScriptRoot,
    [switch]$RebuildZip
)

$GitHubApiUrl  = "https://api.github.com/repos/js9467/cancontroller/contents/versions"
$GitHubRawBase = "https://raw.githubusercontent.com/js9467/cancontroller/main/versions"

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

# Fetch version list from GitHub
Write-Info "Fetching firmware list from GitHub..."
try {
    $headers = @{ "User-Agent" = "BroncoControls-Updater" }
    $items = Invoke-RestMethod -Uri $GitHubApiUrl -Headers $headers -Method Get
} catch {
    Write-Error-Custom "Failed to fetch version list from GitHub: $_"
    exit 1
}

# Find latest bronco_vX.Y.Z.bin
$bins = $items | Where-Object { $_.name -match '^bronco_v[\d.]+\.bin$' }
if (-not $bins) {
    Write-Error-Custom "No bronco_vX.Y.Z.bin files found in GitHub versions folder."
    exit 1
}

$latest = $bins | Sort-Object {
    [Version](($_.name -replace 'bronco_v','') -replace '\.bin$','')
} | Select-Object -Last 1
$latestVersion = ($latest.name -replace 'bronco_v','') -replace '\.bin$',''
Write-Success "Latest version on GitHub: $latestVersion"

# Download the binary
$firmwareUrl = "$GitHubRawBase/$($latest.name)"
$firmwarePath = Join-Path $OutputPath "firmware.bin"

Write-Info "Downloading $($latest.name) from GitHub..."
try {
    Invoke-WebRequest -Uri $firmwareUrl -OutFile $firmwarePath -UseBasicParsing
    Write-Success "Firmware downloaded to: $firmwarePath"
} catch {
    Write-Error-Custom "Failed to download firmware: $_"
    exit 1
}

# Save version info
$versionFile = Join-Path $OutputPath "firmware_version.txt"
$versionInfo = @"
Firmware Version: $latestVersion
Downloaded: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Source: $firmwareUrl
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
    Write-Info "  3. Users can download from: https://github.com/js9467/cancontroller/raw/main/tools/deploy/BroncoFlasher.zip"
}
