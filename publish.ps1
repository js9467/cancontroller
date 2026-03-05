<#
.SYNOPSIS
    Publish a new firmware release. Bumps version, builds, commits source + binary
    to main. Do NOT manually push anything to master — master is a read-only bridge
    shim for devices running firmware older than 6.4.7.

.PARAMETER Message
    Short description of this release (e.g. "Fix window-up timeout").

.PARAMETER Bump
    Which part of the version to increment: build (default), minor, or major.

.EXAMPLE
    .\publish.ps1 -Message "Fix window-up timeout"
    .\publish.ps1 -Message "New output type" -Bump minor
#>
param(
    [Parameter(Mandatory)]
    [string]$Message,
    [ValidateSet('build','minor','major')]
    [string]$Bump = 'build'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot

function Fail($msg) { Write-Host "ERROR: $msg" -ForegroundColor Red; exit 1 }
function Info($msg) { Write-Host $msg -ForegroundColor Cyan }

# ── Guard: must be on main ───────────────────────────────────────────────────
$branch = git -C $root rev-parse --abbrev-ref HEAD
if ($branch -ne 'main') { Fail "You are on '$branch'. Switch to main first: git checkout main" }

# ── Guard: working tree must be clean (except versions/) ────────────────────
$dirty = git -C $root status --porcelain | Where-Object { $_ -notmatch '^\?\?' }
if ($dirty) { Fail "Working tree has uncommitted changes. Commit or stash them first.`n$($dirty -join "`n")" }

# ── 1. Bump version ──────────────────────────────────────────────────────────
$statePath = Join-Path $root '.version_state.json'
$state = Get-Content $statePath | ConvertFrom-Json

switch ($Bump) {
    'major' { $state.major++; $state.minor = 0; $state.build = 0 }
    'minor' { $state.minor++;                   $state.build = 0 }
    'build' {                                   $state.build++   }
}

$version = "$($state.major).$($state.minor).$($state.build)"
Info "Bumping version to $version..."
$state | ConvertTo-Json | Set-Content $statePath -Encoding UTF8

# ── 2. Build ─────────────────────────────────────────────────────────────────
Info "Building firmware $version..."
Push-Location $root
$buildOut = pio run -e waveshare_7in 2>&1
if ($LASTEXITCODE -ne 0) {
    $buildOut | Select-Object -Last 30
    Fail "Build failed."
}
$buildOut | Where-Object { $_ -match 'error:|SUCCESS|FAILED|\[Versioning\]' } | Select-Object -Last 5
Pop-Location

# ── 3. Copy binary into versions/ (on main) ──────────────────────────────────
$binSrc  = Join-Path $root '.pio\build\waveshare_7in\firmware.bin'
$binName = "bronco_v$version.bin"
$binDst  = Join-Path $root "versions\$binName"
if (-not (Test-Path $binSrc)) { Fail "Build output not found: $binSrc" }
Info "Copying binary to versions\$binName..."
New-Item -ItemType Directory -Force (Join-Path $root 'versions') | Out-Null
Copy-Item $binSrc $binDst -Force

# ── 4. Commit everything to main ─────────────────────────────────────────────
Info "Committing to main..."
git -C $root add ".version_state.json" "src/version_auto.h" "versions/$binName"
git -C $root commit -m "v${version}: $Message"
git -C $root push origin main

Info ""
Info "Released v$version to main/versions/$binName"
Info "Verifying GitHub sees the new binary..."
Start-Sleep -Seconds 5

try {
    $files = Invoke-RestMethod "https://api.github.com/repos/js9467/cancontroller/contents/versions" -Headers @{'User-Agent'='publish-script'}
    $found = $files | Where-Object { $_.name -eq $binName }
    if ($found) {
        Info "Confirmed: $binName is live on GitHub."
    } else {
        Write-Host "WARNING: $binName not yet visible via API — GitHub may need a few seconds." -ForegroundColor Yellow
    }
} catch {
    Write-Host "WARNING: Could not verify via GitHub API: $_" -ForegroundColor Yellow
}

Info ""
Info "NOTE: Do NOT push anything to master. master is a read-only bridge shim."
