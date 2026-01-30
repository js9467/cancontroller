# Build script for Backup & Restore Manager
# Creates standalone Windows executable using PyInstaller

param(
    [switch]$Install,
    [switch]$Build,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host "  Bronco Controls - Backup & Restore Builder" -ForegroundColor Cyan
Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host ""

# Check Python
try {
    $pythonVersion = python --version 2>&1
    Write-Host "[✓] Python found: $pythonVersion" -ForegroundColor Green
} catch {
    Write-Host "[✗] Python not found! Install Python 3.8+ first." -ForegroundColor Red
    exit 1
}

# Install dependencies
if ($Install -or -not (Test-Path ".venv")) {
    Write-Host ""
    Write-Host "[1/3] Installing dependencies..." -ForegroundColor Yellow
    
    # Install PyInstaller and other requirements
    $packages = @(
        "pyinstaller",
        "requests",
        "esptool"
    )
    
    foreach ($pkg in $packages) {
        Write-Host "  Installing $pkg..." -ForegroundColor Gray
        python -m pip install --quiet --upgrade $pkg
    }
    
    Write-Host "  [✓] Dependencies installed" -ForegroundColor Green
}

# Build executable
if ($Build -or $Install) {
    Write-Host ""
    Write-Host "[2/3] Building Windows executable..." -ForegroundColor Yellow
    
    # Clean previous builds
    if (Test-Path "build") { Remove-Item -Recurse -Force "build" }
    if (Test-Path "dist\BroncoBackupRestore") { Remove-Item -Recurse -Force "dist\BroncoBackupRestore" }
    
    # Create icon (optional - using emoji for now)
    $iconData = @"
# Simple text-based icon marker
BRONCO BACKUP & RESTORE
"@
    
    # Build with PyInstaller
    $buildCmd = @(
        "--name=BroncoBackupRestore",
        "--onefile",
        "--windowed",
        "--clean",
        "--noconfirm"
    )
    
    # Add both scripts
    $buildCmd += "backup_restore_gui.py"
    
    Write-Host "  Running PyInstaller..." -ForegroundColor Gray
    python -m PyInstaller @buildCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  [✓] Build successful!" -ForegroundColor Green
        
        # Check output
        $exePath = "dist\BroncoBackupRestore.exe"
        if (Test-Path $exePath) {
            $size = (Get-Item $exePath).Length / 1MB
            Write-Host "  Executable: $exePath" -ForegroundColor Cyan
            Write-Host "  Size: $([math]::Round($size, 2)) MB" -ForegroundColor Cyan
        }
    } else {
        Write-Host "  [✗] Build failed!" -ForegroundColor Red
        exit 1
    }
}

# Clean build artifacts
if ($Clean) {
    Write-Host ""
    Write-Host "[3/3] Cleaning build artifacts..." -ForegroundColor Yellow
    
    $cleanPaths = @("build", "dist", "*.spec")
    foreach ($path in $cleanPaths) {
        if (Test-Path $path) {
            Remove-Item -Recurse -Force $path
            Write-Host "  Removed: $path" -ForegroundColor Gray
        }
    }
    
    Write-Host "  [✓] Cleanup complete" -ForegroundColor Green
}

Write-Host ""
Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host "  Build Complete!" -ForegroundColor Green
Write-Host "=====================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "To run the executable:" -ForegroundColor Yellow
Write-Host "  .\dist\BroncoBackupRestore.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "To use command line version:" -ForegroundColor Yellow
Write-Host "  python backup_restore_manager.py --help" -ForegroundColor Cyan
Write-Host ""
