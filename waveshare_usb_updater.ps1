param(
    [string]$Port
)

# Simple Waveshare/Bronco firmware updater using PowerShell + esptool
# Requirements on the PC running this script:
#   - Python installed and on PATH
#   - esptool installed:    pip install esptool
#   - Internet access to GitHub repo js9467/cancontroller (public)

$ErrorActionPreference = 'Stop'

$repo         = 'js9467/cancontroller'
$branch       = 'master'
$versionsPath = 'versions'

function Get-LatestFirmwareInfo {
    # Work-around for corporate networks that block api.github.com:
    # read version components from .version_state.json via raw.githubusercontent.com
    $versionMetaUrl = "https://raw.githubusercontent.com/$repo/$branch/.version_state.json"
    Write-Host "Reading latest version metadata from $versionMetaUrl..." -ForegroundColor Cyan

    $metaResponse = Invoke-WebRequest -Uri $versionMetaUrl -UseBasicParsing
    $meta = $metaResponse.Content | ConvertFrom-Json

    if (-not $meta) {
        throw "Failed to read .version_state.json from $versionMetaUrl"
    }

    $major = [int]$meta.major
    $minor = [int]$meta.minor
    $patch = [int]$meta.build
    $name  = "bronco_v$major.$minor.$patch.bin"

    $downloadUrl = "https://raw.githubusercontent.com/$repo/$branch/$versionsPath/$name"

    return [PSCustomObject]@{
        Name  = $name
        Url   = $downloadUrl
        Major = $major
        Minor = $minor
        Patch = $patch
    }
}

function Get-FirmwareFile([object]$latest) {
    $tempRoot = [IO.Path]::Combine([IO.Path]::GetTempPath(), 'bronco_waveshare_fw')
    if (-not (Test-Path $tempRoot)) {
        New-Item -ItemType Directory -Path $tempRoot | Out-Null
    }

    $firmwarePath = Join-Path $tempRoot $latest.Name
    Write-Host "Downloading $($latest.Name) from GitHub..." -ForegroundColor Cyan
    Invoke-WebRequest -Uri $latest.Url -OutFile $firmwarePath -UseBasicParsing

    Write-Host "Saved firmware to $firmwarePath" -ForegroundColor Green
    return $firmwarePath
}

function Select-SerialPort {
    param([string]$PreferredPort)

    if ($PreferredPort) {
        Write-Host "Using specified port: $PreferredPort" -ForegroundColor Yellow
        return $PreferredPort
    }

    Write-Host "Detecting serial (COM) ports..." -ForegroundColor Cyan
    $ports = Get-CimInstance Win32_SerialPort | Sort-Object -Property DeviceID

    if (-not $ports) {
        Write-Warning 'No serial ports found. Please type the COM port manually (e.g. COM3).'
        $manual = Read-Host 'Enter COM port'
        if (-not $manual) {
            throw 'No COM port provided.'
        }
        return $manual
    }

    Write-Host "Available ports:" -ForegroundColor Yellow
    for ($i = 0; $i -lt $ports.Count; $i++) {
        $p = $ports[$i]
        Write-Host "  [$i] $($p.DeviceID) - $($p.Name)" 
    }

    $selection = Read-Host 'Select port index (or type COMx manually)'

    if ($selection -match '^COM\d+$') {
        return $selection
    }

    if ($selection -notmatch '^\d+$') {
        throw "Invalid selection: $selection"
    }

    $index = [int]$selection
    if ($index -lt 0 -or $index -ge $ports.Count) {
        throw "Index out of range: $index"
    }

    return $ports[$index].DeviceID
}

function Ensure-Esptool {
    $python = 'python'

    Write-Host 'Checking esptool installation (python -m esptool --help)...' -ForegroundColor Cyan
    & $python -m esptool --help *> $null
    if ($LASTEXITCODE -ne 0) {
        throw "esptool is not available. Please install it with: pip install esptool"
    }

    return $python
}

try {
    $latest = Get-LatestFirmwareInfo
    Write-Host "Latest firmware: $($latest.Name)" -ForegroundColor Green

    $firmwarePath = Get-FirmwareFile -latest $latest
    $portName     = Select-SerialPort -PreferredPort $Port
    $python       = Ensure-Esptool

    $args = @(
        '-m', 'esptool',
        '--chip', 'esp32s3',
        '--port', $portName,
        '--baud', '115200',
        'write_flash',
        '0x0',
        $firmwarePath
    )

    Write-Host "Running: $python $($args -join ' ')" -ForegroundColor Yellow
    & $python @args

    if ($LASTEXITCODE -eq 0) {
        Write-Host 'Firmware update completed successfully.' -ForegroundColor Green
    }
    else {
        throw "esptool exited with code $LASTEXITCODE."
    }
}
catch {
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
