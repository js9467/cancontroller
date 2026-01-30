[CmdletBinding()]
param(
    [string]$OutputPath,
    [string]$Port,
    [switch]$ListPorts,
    [switch]$EraseAfterBackup,
    [string]$EsptoolVersion = "v4.7.0"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
$DefaultBackupRoot = Join-Path $RepoRoot "dist\backups"

function Write-Info {
    param([string]$Message)
    Write-Host "[backup] $Message"
}

function Get-SerialCandidates {
    try {
        return @(Get-CimInstance Win32_SerialPort |
            Select-Object DeviceID, Description, Manufacturer, PNPDeviceID |
            Sort-Object DeviceID)
    } catch {
        Write-Warning "Unable to enumerate serial ports: $_"
        return @()
    }
}

function Detect-Port {
    param([array]$Candidates)
    $Candidates = @($Candidates)
    if ($Candidates.Count -eq 0) {
        return $null
    }

    $patterns = @('CP210', 'Silicon Labs', 'USB Serial', 'USB JTAG', 'ESP32', 'CH910')
    foreach ($pattern in $patterns) {
        $match = $Candidates | Where-Object { $_.Description -like "*$pattern*" }
        if ($match) {
            return $match[0].DeviceID
        }
    }

    return $Candidates[0].DeviceID
}

function Ensure-Esptool {
    param([string]$Version)
    $root = Join-Path $env:LOCALAPPDATA "BroncoControls\esptool-$Version"
    $exe = Join-Path $root "esptool.exe"

    if (Test-Path $exe) {
        return $exe
    }

    Write-Info "Downloading esptool $Version"
    $downloadUrl = "https://github.com/espressif/esptool/releases/download/$Version/esptool-$Version-win64.zip"
    $tempZip = Join-Path ([System.IO.Path]::GetTempPath()) ("esptool-" + [guid]::NewGuid() + ".zip")
    Invoke-WebRequest -Uri $downloadUrl -OutFile $tempZip

    if (Test-Path $root) {
        Remove-Item $root -Recurse -Force
    }
    New-Item -ItemType Directory -Path $root | Out-Null
    Expand-Archive -LiteralPath $tempZip -DestinationPath $root
    Remove-Item $tempZip -Force

    $tool = Get-ChildItem -Path $root -Filter esptool.exe -Recurse | Select-Object -First 1
    if (-not $tool) {
        throw "esptool.exe not found after extraction"
    }

    return $tool.FullName
}

function Resolve-OutputPath {
    param([string]$Path)

    if (-not $Path) {
        $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
        $Path = Join-Path $DefaultBackupRoot ("device_backup_$timestamp.bin")
    }

    if (-not [System.IO.Path]::IsPathRooted($Path)) {
        $Path = Join-Path $PSScriptRoot $Path
    }

    return [System.IO.Path]::GetFullPath($Path)
}

$candidates = @(Get-SerialCandidates)
if ($ListPorts) {
    if ($candidates.Count -eq 0) {
        Write-Info "No serial ports found"
    } else {
        Write-Info "Available ports:"
        $candidates | Format-Table -AutoSize
    }
    return
}

if (-not $Port) {
    $Port = Detect-Port -Candidates $candidates
}

if (-not $Port) {
    throw "Could not auto-detect the ESP32-S3 serial port. Re-run with -Port COMx"
}

$esptool = Ensure-Esptool -Version $EsptoolVersion
$resolvedOutput = Resolve-OutputPath -Path $OutputPath
$resolvedDir = Split-Path -Parent $resolvedOutput

if (-not (Test-Path $resolvedDir)) {
    New-Item -ItemType Directory -Path $resolvedDir -Force | Out-Null
}

Write-Info "Using port $Port"
Write-Info "Backing up flash to $resolvedOutput"

$backupArgs = @(
    "--chip", "esp32s3",
    "--port", $Port,
    "--baud", "460800",
    "read_flash",
    "0x0", "0x1000000",
    $resolvedOutput
)

& $esptool @backupArgs
if ($LASTEXITCODE -ne 0) {
    throw "esptool read_flash failed with exit code $LASTEXITCODE"
}

$bytes = (Get-Item $resolvedOutput).Length
$sizeMb = [Math]::Round($bytes / 1MB, 2)
Write-Info "Backup complete ($sizeMb MB saved)."

if ($EraseAfterBackup) {
    Write-Info "Erasing entire flash..."
    $eraseArgs = @(
        "--chip", "esp32s3",
        "--port", $Port,
        "erase_flash"
    )
    & $esptool @eraseArgs
    if ($LASTEXITCODE -ne 0) {
        throw "esptool erase_flash failed with exit code $LASTEXITCODE"
    }
    Write-Info "Flash erase complete."
}