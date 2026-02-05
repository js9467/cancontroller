#!/usr/bin/env python3
"""
ESP32-S3 Bronco Controls Backup & Restore Manager
Handles full device backup, version management, and GitHub integration
"""

import os
import sys
import json
import subprocess
import time
import argparse
from pathlib import Path
from datetime import datetime
import requests
import base64
import re
import zipfile
import shutil
import tempfile

class BackupRestoreManager:
    """Manages full device backup and restore operations for ESP32-S3"""
    
    # ESP32-S3 Flash Layout (16MB configuration)
    FLASH_SIZE = 16 * 1024 * 1024  # 16MB
    FLASH_REGIONS = {
        'bootloader': {'offset': 0x0, 'size': 0x8000},      # 32KB
        'partitions': {'offset': 0x8000, 'size': 0x1000},   # 4KB
        'nvs': {'offset': 0x9000, 'size': 0x5000},          # 20KB (non-volatile storage)
        'otadata': {'offset': 0xE000, 'size': 0x2000},      # 8KB (OTA data)
        'app0': {'offset': 0x10000, 'size': 0x3F0000},      # ~4MB (main app)
        'app1': {'offset': 0x400000, 'size': 0x3F0000},     # ~4MB (OTA app)
        'spiffs': {'offset': 0x7F0000, 'size': 0x10000},    # 64KB (file system)
    }
    
    def __init__(self, port='COM5', baud=460800):
        self.port = port
        self.baud = baud
        self.project_dir = Path(__file__).parent.resolve()
        self.backups_dir = self.project_dir / 'backups'
        self.backups_dir.mkdir(exist_ok=True)
        
        self.version_state_file = self.project_dir / '.version_state.json'
        self.version_header = self.project_dir / 'src' / 'version_auto.h'
        
        # GitHub configuration
        self.github_repo = 'js9467/cancontroller'
        self.github_branch = 'master'
        self.github_folder = 'versions'
        self.github_token = (
            os.getenv('BRONCO_GITHUB_TOKEN')
            or os.getenv('GITHUB_TOKEN')
            or os.getenv('GITHUB_PAT')
        )

    def _github_headers(self, accept='application/vnd.github.v3+json', require_token=False):
        """Helper to build GitHub API headers with optional auth"""
        headers = {'Accept': accept}
        if self.github_token:
            headers['Authorization'] = f'token {self.github_token}'
        elif require_token:
            print("[GitHub] ⚠ No GitHub token found. Set BRONCO_GITHUB_TOKEN or GITHUB_TOKEN.")
            return None
        return headers

    def _upload_release_asset(self, upload_url, file_path, content_type):
        """Helper to upload a file as a GitHub release asset"""
        file_path = Path(file_path)
        if not file_path.exists():
            print(f"[GitHub] ⚠ Asset not found: {file_path}")
            return False

        headers = self._github_headers(accept='application/vnd.github.v3+json', require_token=True)
        if headers is None:
            return False

        upload_headers = headers.copy()
        upload_headers['Content-Type'] = content_type

        with open(file_path, 'rb') as f:
            response = requests.post(
                f'{upload_url}?name={file_path.name}',
                headers=upload_headers,
                data=f
            )

        if response.status_code == 201:
            print(f"[GitHub] ✓ Uploaded {file_path.name}")
            return True

        print(f"[GitHub] ✗ Upload failed for {file_path.name}: {response.status_code}")
        print(f"[GitHub] {response.text}")
        return False
        
    def get_current_version(self):
        """Read current version from version state file"""
        if self.version_state_file.exists():
            try:
                with open(self.version_state_file, 'r', encoding='utf-8-sig') as f:
                    state = json.load(f)
                    return f"{state['major']}.{state['minor']}.{state['build']}"
            except Exception as e:
                print(f"[Warning] Could not read version state: {e}")
        
        # Fallback: try to parse from version_auto.h
        if self.version_header.exists():
            content = self.version_header.read_text()
            match = re.search(r'APP_VERSION = "(\d+\.\d+\.\d+)"', content)
            if match:
                return match.group(1)
        
        return "1.3.78"  # Default fallback
    
    def increment_version(self, increment_type='build'):
        """Increment version number (major, minor, or build) and update all version files
        
        Args:
            increment_type: 'major', 'minor', or 'build' (default)
        """
        # Read current state
        version_state = {"major": 2, "minor": 1, "build": 4}
        if self.version_state_file.exists():
            try:
                with open(self.version_state_file, 'r', encoding='utf-8-sig') as f:
                    loaded_state = json.load(f)
                    version_state.update(loaded_state)
            except Exception as e:
                print(f"[Warning] Could not read version state file: {e}")
                pass
        
        # Increment version based on type
        if increment_type == 'major':
            version_state['major'] += 1
            version_state['minor'] = 0
            version_state['build'] = 0
            print(f"[Version] MAJOR version increment")
        elif increment_type == 'minor':
            version_state['minor'] += 1
            version_state['build'] = 0
            print(f"[Version] MINOR version increment")
        else:  # build
            version_state['build'] += 1
            print(f"[Version] Build increment")
        
        new_version = f"{version_state['major']}.{version_state['minor']}.{version_state['build']}"
        
        # Save updated state
        version_state['last_increment'] = increment_type
        version_state['last_update'] = datetime.now().isoformat()
        with open(self.version_state_file, 'w', encoding='utf-8') as f:
            json.dump(version_state, f, indent=2)
        
        # Update version header
        timestamp = datetime.utcnow().isoformat()
        header_content = (
            f"#pragma once\n"
            f"// Auto-generated on {timestamp}Z\n"
            f'constexpr const char* APP_VERSION = "{new_version}";\n'
        )
        self.version_header.write_text(header_content, encoding='utf-8')
        
        print(f"[Version] Incremented to {new_version}")
        return new_version, increment_type
    
    def backup_device(self, increment_type='build', version=None, upload_to_device=True):
        """Create a backup based on increment type:
        - Build/Minor: Increment version, build firmware, copy to versions/, push to git
        - Major: Full USB flash backup + all of the above
        
        Args:
            increment_type: 'major', 'minor', or 'build' (default)
            version: Optional version override
            upload_to_device: Upload firmware to device via USB (default True)
        """
        if version is None:
            version, increment_type = self.increment_version(increment_type)
        
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        is_major = increment_type == 'major'
        
        # Build/Minor: Just build firmware and push to git (no USB backup)
        if not is_major:
            print(f"\n{'='*60}")
            print(f"[Release] {increment_type.upper()} version increment")
            print(f"[Release] Version: {version}")
            print(f"[Release] Building firmware...")
            print(f"{'='*60}\n")
            
            # Build firmware with PlatformIO
            if not self.build_firmware():
                print("[Release] ✗ Build failed")
                return None
            
            # Upload to device if requested
            if upload_to_device:
                print(f"\n[Release] Uploading firmware to device...")
                if not self.upload_firmware():
                    print("[Release] ⚠ Upload failed (continuing anyway)")
                else:
                    print(f"[Release] ✓ Device updated with v{version}")
            
            # Copy firmware to versions/ folder
            print(f"\n[Release] Preparing OTA firmware...")
            if not self.copy_firmware_for_ota(version):
                print("[Release] ✗ Firmware copy failed")
                return None
            
            # Push to git
            print(f"\n[Git] Pushing to repository...")
            if not self.git_push_all(version, False):
                print("[Git] ✗ Git push failed")
                return None
            
            print(f"\n{'='*60}")
            print(f"[Release] ✓ Complete!")
            print(f"[Release] Version {version} built and pushed to Git")
            print(f"[Release] Firmware: versions/bronco_v{version}.bin")
            print(f"{'='*60}\n")
            
            return None, version
        
        # MAJOR: Full USB flash backup
        backup_name = f"bronco_v{version}_{timestamp}_FULL"
        backup_folder = self.backups_dir / backup_name
        backup_folder.mkdir(exist_ok=True)
        
        print(f"\n{'='*60}")
        print(f"[Backup] MAJOR RELEASE - Full USB flash backup")
        print(f"[Backup] Version: {version}")
        print(f"[Backup] Location: {backup_folder}")
        print(f"{'='*60}\n")
        
        # Metadata
        metadata = {
            'version': version,
            'timestamp': timestamp,
            'device': 'ESP32-S3-Touch-LCD-7',
            'chip': 'esp32s3',
            'flash_size': '16MB',
            'backup_date': datetime.now().isoformat(),
            'regions': {}
        }
        
        # Backup each flash region from USB
        total_regions = len(self.FLASH_REGIONS)
        for idx, (region_name, region_info) in enumerate(self.FLASH_REGIONS.items(), 1):
            offset = region_info['offset']
            size = region_info['size']
            output_file = backup_folder / f"{region_name}.bin"
            
            print(f"[{idx}/{total_regions}] Backing up {region_name} (offset: 0x{offset:X}, size: {size} bytes)...")
            
            try:
                cmd = [
                    'python', '-m', 'esptool',
                    '--chip', 'esp32s3',
                    '--port', self.port,
                    '--baud', str(self.baud),
                    'read_flash',
                    f'0x{offset:X}',
                    str(size),
                    str(output_file)
                ]
                
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
                
                if result.returncode == 0 and output_file.exists():
                    file_size = output_file.stat().st_size
                    print(f"    ✓ Success ({file_size} bytes)")
                    metadata['regions'][region_name] = {
                        'offset': f'0x{offset:X}',
                        'size': size,
                        'file': output_file.name,
                        'actual_size': file_size
                    }
                else:
                    print(f"    ✗ Failed: {result.stderr}")
                    return None
                    
            except subprocess.TimeoutExpired:
                print(f"    ✗ Timeout during backup")
                return None
            except Exception as e:
                print(f"    ✗ Error: {e}")
                return None
        
        # Create full flash dump (16MB complete image)
        print(f"\n[Backup] Creating full 16MB flash dump...")
        full_dump = backup_folder / 'full_flash_16MB.bin'
        try:
            cmd = [
                'python', '-m', 'esptool',
                '--chip', 'esp32s3',
                '--port', self.port,
                '--baud', str(self.baud),
                'read_flash',
                '0x0',
                str(self.FLASH_SIZE),
                str(full_dump)
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
            if result.returncode == 0:
                print(f"    ✓ Full dump created ({full_dump.stat().st_size} bytes)")
                metadata['full_dump'] = full_dump.name
            else:
                print(f"    ⚠ Full dump failed (not critical)")
        except Exception as e:
            print(f"    ⚠ Full dump error: {e} (not critical)")
        
        # Save metadata
        metadata_file = backup_folder / 'backup_metadata.json'
        with open(metadata_file, 'w', encoding='utf-8') as f:
            json.dump(metadata, f, indent=2)
        
        # Create restore script
        self.create_restore_script(backup_folder, metadata)
        
        # Build firmware
        print(f"\n[Backup] Building firmware...")
        if not self.build_firmware():
            print("[Backup] ⚠ Build failed (continuing anyway)")
        
        # Copy firmware.bin for OTA distribution
        print(f"\n[Backup] Preparing OTA firmware...")
        self.copy_firmware_for_ota(version)
        
        # Push ALL versions to git automatically and create GitHub release with backup
        print(f"\n[Git] Pushing to repository...")
        self.git_push_all(version, True, backup_folder)
        
        print(f"\n{'='*60}")
        print(f"[Backup] ✓ Complete! Backup saved to:")
        print(f"         {backup_folder}")
        print(f"[Backup] FULL DEVICE BACKUP (Major Release)")
        print(f"[Backup] Source code + firmware pushed to Git")
        print(f"{'='*60}\n")
        
        return backup_folder, version
    
    def create_restore_script(self, backup_folder, metadata):
        """Create a standalone restore script in the backup folder"""
        restore_script = backup_folder / 'RESTORE.bat'
        
        script_content = f'''@echo off
REM Bronco Controls - Device Restore Script
REM Version: {metadata['version']}
REM Created: {metadata['backup_date']}

echo ============================================================
echo  Bronco Controls - Device Restore
echo  Version: {metadata['version']}
echo ============================================================
echo.
echo This will COMPLETELY ERASE and restore your ESP32 device.
echo.
echo WARNING: This will delete ALL data on the device!
echo.
pause

echo.
echo [1/2] Erasing flash...
python -m esptool --chip esp32s3 --port COM5 erase_flash
if errorlevel 1 (
    echo ERROR: Flash erase failed!
    pause
    exit /b 1
)

echo.
echo [2/2] Restoring backup...
'''
        
        # Add write commands for each region
        for region_name, region_info in metadata['regions'].items():
            offset = region_info['offset']
            file_name = region_info['file']
            script_content += f'python -m esptool --chip esp32s3 --port COM5 --baud 460800 write_flash {offset} {file_name}\n'
        
        script_content += '''
echo.
echo ============================================================
echo  Restore Complete!
echo ============================================================
echo.
echo Device will reboot automatically.
echo Web interface should be available shortly.
echo.
pause
'''
        
        restore_script.write_text(script_content, encoding='utf-8')
        print(f"[Backup] Created restore script: {restore_script.name}")
    
    def restore_device(self, backup_source):
        """Restore device from a backup folder or GitHub URL"""
        # Check if it's a GitHub URL
        if isinstance(backup_source, str) and backup_source.startswith('http'):
            return self.restore_from_github(backup_source)
        
        # Local backup restore
        backup_path = Path(backup_source)
        if not backup_path.exists():
            print(f"[Error] Backup folder not found: {backup_source}")
            return False
        
        metadata_file = backup_path / 'backup_metadata.json'
        if not metadata_file.exists():
            print(f"[Error] Invalid backup: metadata file not found")
            return False
        
        with open(metadata_file, 'r', encoding='utf-8') as f:
            metadata = json.load(f)
        
        print(f"\n{'='*60}")
        print(f"[Restore] Restoring from backup")
        print(f"[Restore] Version: {metadata['version']}")
        print(f"[Restore] Date: {metadata['backup_date']}")
        print(f"{'='*60}\n")
        
        # Step 1: Erase flash
        print("[1/2] Erasing device flash...")
        try:
            cmd = [
                'python', '-m', 'esptool',
                '--chip', 'esp32s3',
                '--port', self.port,
                'erase_flash'
            ]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            if result.returncode != 0:
                print(f"    ✗ Erase failed: {result.stderr}")
                return False
            print("    ✓ Flash erased")
        except Exception as e:
            print(f"    ✗ Error: {e}")
            return False
        
        time.sleep(2)
        
        # Step 2: Restore each region
        print("\n[2/2] Restoring flash regions...")
        total_regions = len(metadata['regions'])
        for idx, (region_name, region_info) in enumerate(metadata['regions'].items(), 1):
            offset = region_info['offset']
            file_name = region_info['file']
            file_path = backup_path / file_name
            
            if not file_path.exists():
                print(f"    [{idx}/{total_regions}] ✗ Missing file: {file_name}")
                continue
            
            print(f"    [{idx}/{total_regions}] Writing {region_name} to {offset}...")
            
            try:
                cmd = [
                    'python', '-m', 'esptool',
                    '--chip', 'esp32s3',
                    '--port', self.port,
                    '--baud', str(self.baud),
                    'write_flash',
                    offset,
                    str(file_path)
                ]
                
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
                if result.returncode == 0:
                    print(f"         ✓ Success")
                else:
                    print(f"         ✗ Failed: {result.stderr}")
                    return False
                    
            except Exception as e:
                print(f"         ✗ Error: {e}")
                return False
        
        print(f"\n{'='*60}")
        print(f"[Restore] ✓ Device restored successfully!")
        print(f"[Restore] Device will reboot automatically")
        print(f"{'='*60}\n")
        
        return True
    
    def restore_from_github(self, download_url):
        """Download and restore backup from GitHub"""
        try:
            print(f"\n{'='*60}")
            print(f"[GitHub] Downloading backup from GitHub...")
            print(f"{'='*60}\n")
            
            # Download zip file
            headers = self._github_headers(accept='application/octet-stream', require_token=False)
            response = requests.get(download_url, headers=headers, stream=True)
            
            if response.status_code != 200:
                print(f"[Error] Download failed: {response.status_code}")
                return False
            
            # Save to temp file
            with tempfile.NamedTemporaryFile(delete=False, suffix='.zip') as temp_zip:
                total_size = int(response.headers.get('content-length', 0))
                downloaded = 0
                
                for chunk in response.iter_content(chunk_size=8192):
                    temp_zip.write(chunk)
                    downloaded += len(chunk)
                    if total_size > 0:
                        percent = (downloaded / total_size) * 100
                        print(f"\r[Download] {percent:.1f}% ({downloaded / (1024*1024):.1f} MB)", end='')
                
                temp_zip_path = temp_zip.name
            
            print(f"\n[Download] ✓ Download complete")
            
            # Extract to temp directory
            temp_extract_dir = tempfile.mkdtemp()
            print(f"[Extract] Extracting backup...")
            
            with zipfile.ZipFile(temp_zip_path, 'r') as zip_ref:
                zip_ref.extractall(temp_extract_dir)
            
            print(f"[Extract] ✓ Extraction complete")
            
            # Find the backup folder (should be only one)
            extracted_folders = [f for f in Path(temp_extract_dir).iterdir() if f.is_dir()]
            if not extracted_folders:
                print(f"[Error] No backup folder found in archive")
                return False
            
            backup_folder = extracted_folders[0]
            
            # Restore from extracted folder
            result = self.restore_device(str(backup_folder))
            
            # Clean up
            os.unlink(temp_zip_path)
            shutil.rmtree(temp_extract_dir)
            
            return result
            
        except Exception as e:
            print(f"[Error] Failed to restore from GitHub: {e}")
            return False
    
    def build_firmware(self):
        """Build firmware using PlatformIO"""
        try:
            print("[Build] Running PlatformIO build...")
            result = subprocess.run(
                ['pio', 'run', '-e', 'waveshare_7in'],
                cwd=self.project_dir,
                capture_output=True,
                text=True,
                timeout=300
            )
            
            if result.returncode == 0:
                print("[Build] ✓ Build successful")
                return True
            else:
                print(f"[Build] ✗ Build failed: {result.stderr}")
                return False
                
        except subprocess.TimeoutExpired:
            print("[Build] ✗ Build timeout")
            return False
        except Exception as e:
            print(f"[Build] ✗ Build error: {e}")
            return False
    
    def upload_firmware(self):
        """Upload firmware to device using PlatformIO"""
        try:
            print("[Upload] Uploading to ESP32-S3...")
            result = subprocess.run(
                ['pio', 'run', '-e', 'waveshare_7in', '-t', 'upload'],
                cwd=self.project_dir,
                capture_output=True,
                text=True,
                timeout=120
            )
            
            if result.returncode == 0:
                print("[Upload] ✓ Upload successful")
                return True
            else:
                print(f"[Upload] ✗ Upload failed: {result.stderr}")
                return False
                
        except subprocess.TimeoutExpired:
            print("[Upload] ✗ Upload timeout")
            return False
        except Exception as e:
            print(f"[Upload] ✗ Upload error: {e}")
            return False
    
    def copy_firmware_for_ota(self, version):
        """Copy firmware.bin to versions folder for OTA distribution
        
        This copies the built firmware from PlatformIO to versions/ folder
        with the correct naming for OTA updates.
        """
        # PlatformIO build output location
        firmware_source = self.project_dir / '.pio' / 'build' / 'waveshare_7in' / 'firmware.bin'
        
        if not firmware_source.exists():
            print(f"\n[OTA] ⚠ Firmware not found: {firmware_source}")
            print(f"[OTA] Run 'pio run -e waveshare_7in' first to build firmware")
            return False
        
        # Versions folder for OTA binaries
        versions_dir = self.project_dir / 'versions'
        versions_dir.mkdir(exist_ok=True)
        
        # OTA firmware naming: bronco_v{VERSION}.bin
        ota_firmware = versions_dir / f"bronco_v{version}.bin"
        
        print(f"\n[OTA] Copying firmware for OTA distribution...")
        print(f"[OTA] Source: {firmware_source}")
        print(f"[OTA] Destination: {ota_firmware}")
        
        try:
            import shutil
            shutil.copy2(firmware_source, ota_firmware)
            size_mb = ota_firmware.stat().st_size / (1024 * 1024)
            print(f"[OTA] ✓ Firmware copied ({size_mb:.2f} MB)")
            print(f"[OTA] Ready for OTA distribution")
            print(f"\n[Next Steps]")
            print(f"  1. Test the firmware on a device first")
            print(f"  2. Upload to GitHub: git add {ota_firmware}")
            print(f"  3. Commit: git commit -m 'Add OTA firmware v{version}'")
            print(f"  4. Push: git push")
            print(f"  5. Users can now OTA update to v{version}")
            return True
        except Exception as e:
            print(f"[OTA] ✗ Copy failed: {e}")
            return False
    
    def git_push_all(self, version, is_major=False, backup_folder=None):
        """Commit all source code and push to git repository"""
        try:
            # Ensure we're on a branch (not detached HEAD)
            print(f"[Git] Checking branch status...")
            branch_check = subprocess.run(
                ['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                cwd=self.project_dir,
                capture_output=True,
                text=True
            )
            current_branch = branch_check.stdout.strip()
            
            if current_branch == 'HEAD':
                # We're in detached HEAD state - switch to master
                print(f"[Git] Detached HEAD detected, switching to master branch...")
                subprocess.run(['git', 'checkout', 'master'], cwd=self.project_dir, check=True)
                current_branch = 'master'
            
            print(f"[Git] Current branch: {current_branch}")
            print(f"[Git] Adding all source files...")
            
            # Add all source files
            subprocess.run(['git', 'add', '.'], cwd=self.project_dir, check=True)
            
            # Commit with version
            if is_major:
                commit_msg = f"Major release v{version} - Full device backup"
            else:
                commit_msg = f"Release v{version}"
            print(f"[Git] Committing: {commit_msg}")
            subprocess.run(['git', 'commit', '-m', commit_msg], cwd=self.project_dir, check=True)
            
            # Push to remote
            print(f"[Git] Pushing to remote repository...")
            result = subprocess.run(['git', 'push', 'origin', current_branch], cwd=self.project_dir, capture_output=True, text=True)
            
            if result.returncode == 0:
                print(f"[Git] ✓ Successfully pushed to repository")
                
                # If major release and backup folder provided, create GitHub release with backup
                if is_major and backup_folder:
                    self.create_github_release(version, backup_folder)
                
                return True
            else:
                print(f"[Git] ✗ Push failed: {result.stderr}")
                return False
                
        except subprocess.CalledProcessError as e:
            print(f"[Git] ✗ Error: {e}")
            return False
        except Exception as e:
            print(f"[Git] ✗ Unexpected error: {e}")
            return False
    
    def create_github_release(self, version, backup_folder):
        """Create GitHub release with backup zip file"""
        try:
            print(f"[GitHub] Creating release v{version} with full backup...")
            
            # Create zip file from backup folder
            backup_path = Path(backup_folder)
            zip_filename = f"bronco_v{version}_FULL_BACKUP.zip"
            zip_path = self.project_dir / zip_filename
            
            print(f"[GitHub] Creating backup archive...")
            with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
                for file in backup_path.rglob('*'):
                    if file.is_file():
                        arcname = file.relative_to(backup_path.parent)
                        zipf.write(file, arcname)
            
            zip_size_mb = zip_path.stat().st_size / (1024 * 1024)
            print(f"[GitHub] Created {zip_filename} ({zip_size_mb:.1f} MB)")
            
            # Create GitHub release
            headers = self._github_headers(accept='application/vnd.github.v3+json', require_token=True)
            if headers is None:
                print("[GitHub] ✗ Release creation requires a GitHub token")
                return
            
            release_data = {
                'tag_name': f'v{version}',
                'name': f'Bronco Controls v{version} - Full Backup',
                'body': f'Full device backup for version {version}\n\nIncludes complete flash dump with all partitions.',
                'draft': False,
                'prerelease': False
            }
            
            print(f"[GitHub] Creating release tag v{version}...")
            response = requests.post(
                f'https://api.github.com/repos/{self.github_repo}/releases',
                headers=headers,
                json=release_data
            )
            
            if response.status_code == 201:
                release_id = response.json()['id']
                upload_url = response.json()['upload_url'].split('{')[0]
                print(f"[GitHub] ✓ Release created (ID: {release_id})")
                
                # Upload zip file as release asset
                print(f"[GitHub] Uploading backup archive...")
                self._upload_release_asset(upload_url, zip_path, 'application/zip')

                # Upload updater batch file for online updater
                updater_bat = self.project_dir / 'BackupRestore.bat'
                print(f"[GitHub] Uploading updater batch file...")
                self._upload_release_asset(upload_url, updater_bat, 'application/octet-stream')

                print(f"[GitHub] Release URL: https://github.com/{self.github_repo}/releases/tag/v{version}")
                
                # Clean up local zip file
                zip_path.unlink()
                
            elif response.status_code == 422:
                print(f"[GitHub] ℹ Release v{version} already exists")
            else:
                print(f"[GitHub] ✗ Failed to create release: {response.status_code}")
                print(f"[GitHub] {response.text}")
                
        except Exception as e:
            print(f"[GitHub] ✗ Error creating release: {e}")
    
    def fetch_github_releases(self):
        """Fetch available releases from GitHub"""
        try:
            headers = self._github_headers(accept='application/vnd.github.v3+json', require_token=False)
            response = requests.get(
                f'https://api.github.com/repos/{self.github_repo}/releases',
                headers=headers
            )

            if response.status_code in (401, 403) and self.github_token:
                print(f"[GitHub] Auth failed ({response.status_code}). Retrying without token...")
                response = requests.get(
                    f'https://api.github.com/repos/{self.github_repo}/releases',
                    headers={'Accept': 'application/vnd.github.v3+json'}
                )
            
            if response.status_code == 200:
                releases = response.json()
                github_backups = []
                
                for release in releases:
                    # Look for full backup assets (zip files)
                    for asset in release.get('assets', []):
                        if 'FULL_BACKUP.zip' in asset['name']:
                            github_backups.append({
                                'version': release['tag_name'].lstrip('v'),
                                'date': release['published_at'],
                                'size': asset['size'],
                                'download_url': asset['browser_download_url'],
                                'name': asset['name'],
                                'source': 'github'
                            })
                
                return github_backups
            else:
                print(f"[GitHub] Could not fetch releases: {response.status_code}")
                return []
                
        except Exception as e:
            print(f"[GitHub] Error fetching releases: {e}")
            return []
    
    def list_backups(self, include_github=True):
        """List all available backups (local and GitHub)"""
        # Get local backups
        local_backups = sorted(self.backups_dir.glob('bronco_v*'), reverse=True)
        
        backup_info = []
        
        # Process local backups
        for backup in local_backups:
            metadata_file = backup / 'backup_metadata.json'
            if metadata_file.exists():
                with open(metadata_file, 'r') as f:
                    metadata = json.load(f)
                    version = metadata.get('version', 'unknown')
                    date = metadata.get('backup_date', 'unknown')
                    size = sum(f.stat().st_size for f in backup.rglob('*') if f.is_file())
                    
                    backup_info.append({
                        'path': str(backup),
                        'version': version,
                        'date': date,
                        'size': size,
                        'source': 'local'
                    })
        
        # Get GitHub backups
        if include_github:
            github_backups = self.fetch_github_releases()
            backup_info.extend(github_backups)
        
        # Sort by version (descending)
        backup_info.sort(key=lambda x: x['version'], reverse=True)
        
        if not backup_info:
            print("[Backups] No backups found")
            return []
        
        # Display all backups
        print(f"\n{'='*70}")
        print(f"Available Backups ({len(backup_info)} found):")
        print(f"{'='*70}")
        
        for idx, backup in enumerate(backup_info, 1):
            version = backup['version']
            date = backup['date'][:19] if len(backup['date']) > 19 else backup['date']
            size_mb = backup['size'] / (1024 * 1024)
            source = backup['source'].upper()
            
            print(f"{idx}. v{version} - {date} ({size_mb:.1f} MB) [{source}]")
            if backup['source'] == 'local':
                print(f"   {backup['path']}")
            else:
                print(f"   {backup['download_url']}")
        
        print(f"{'='*70}\n")
        return backup_info


def main():
    parser = argparse.ArgumentParser(
        description='ESP32-S3 Bronco Controls Backup & Restore Manager',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  # Create backup with auto-incremented version
  python backup_restore_manager.py backup
  
  # Create backup and upload to GitHub
  python backup_restore_manager.py backup --upload
  
  # List all backups
  python backup_restore_manager.py list
  
  # Restore from latest backup
  python backup_restore_manager.py restore
  
  # Restore from specific backup
  python backup_restore_manager.py restore --backup "backups/bronco_v1.3.79_20260130_123456"
  
  # Full test cycle (backup, erase, restore)
  python backup_restore_manager.py test
        '''
    )
    
    parser.add_argument('action', choices=['backup', 'restore', 'list', 'test'],
                       help='Action to perform')
    parser.add_argument('--port', default='COM5',
                       help='Serial port (default: COM5)')
    parser.add_argument('--baud', type=int, default=460800,
                       help='Baud rate (default: 460800)')
    parser.add_argument('--backup', type=str,
                       help='Backup folder to restore from')
    parser.add_argument('--upload', action='store_true',
                       help='Upload backup to GitHub')
    
    args = parser.parse_args()
    
    manager = BackupRestoreManager(port=args.port, baud=args.baud)
    
    if args.action == 'backup':
        backup_folder, version = manager.backup_device()
        if backup_folder and args.upload:
            manager.upload_to_github(backup_folder, version)
    
    elif args.action == 'restore':
        if args.backup:
            manager.restore_device(args.backup)
        else:
            # Restore from latest backup
            backups = manager.list_backups()
            if backups:
                latest = backups[0]['path']
                print(f"[Restore] Using latest backup: {latest}")
                manager.restore_device(latest)
            else:
                print("[Error] No backups available")
    
    elif args.action == 'list':
        manager.list_backups()
    
    elif args.action == 'test':
        print(f"\n{'='*60}")
        print(f"FULL TEST CYCLE: Backup → Erase → Restore")
        print(f"{'='*60}\n")
        input("Press Enter to start backup...")
        
        # Step 1: Backup
        backup_folder, version = manager.backup_device()
        if not backup_folder:
            print("[Test] ✗ Backup failed, aborting test")
            return
        
        input("\nBackup complete. Press Enter to ERASE device...")
        
        # Step 2: Erase
        print("\n[Test] Erasing device...")
        cmd = ['python', '-m', 'esptool', '--chip', 'esp32s3', '--port', args.port, 'erase_flash']
        result = subprocess.run(cmd)
        if result.returncode != 0:
            print("[Test] ✗ Erase failed")
            return
        
        input("\nDevice erased. Press Enter to RESTORE...")
        
        # Step 3: Restore
        if manager.restore_device(backup_folder):
            print("\n[Test] ✓ Full test cycle completed successfully!")
        else:
            print("\n[Test] ✗ Restore failed")


if __name__ == '__main__':
    main()
