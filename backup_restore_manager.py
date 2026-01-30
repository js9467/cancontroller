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
        """Increment version and update all version files
        
        Args:
            increment_type: 'major' for full backup (1.x → 2.0.0), 
                          'build' for version update (1.3.x → 1.3.x+1)
        """
        # Read current state
        version_state = {"major": 1, "minor": 3, "build": 78}
        if self.version_state_file.exists():
            try:
                with open(self.version_state_file, 'r', encoding='utf-8-sig') as f:
                    loaded_state = json.load(f)
                    version_state.update(loaded_state)
            except Exception as e:
                print(f"[Warning] Could not read version state file: {e}")
                pass
        
        # Increment based on type
        if increment_type == 'major':
            version_state['major'] += 1
            version_state['minor'] = 0
            version_state['build'] = 0
            print(f"[Version] MAJOR version increment (full backup)")
        else:  # build
            version_state['build'] += 1
            print(f"[Version] Build increment (version update)")
        
        new_version = f"{version_state['major']}.{version_state['minor']}.{version_state['build']}"
        
        # Save updated state
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
        
        print(f"[Version] New version: {new_version}")
        return new_version
    
    def version_update(self):
        """Create a version snapshot from current project state (no device communication)"""
        # Get current version to determine if it's a major version change
        old_version = self.get_current_version()
        old_parts = list(map(int, old_version.split('.')))
        
        version = self.increment_version(increment_type='build')
        new_parts = list(map(int, version.split('.')))
        
        # Check if this is a major version change (e.g., v2 to v3)
        is_major_version = (new_parts[0] > old_parts[0])
        
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        version_name = f"bronco_v{version}_{timestamp}"
        version_folder = self.backups_dir / version_name
        version_folder.mkdir(exist_ok=True)
        
        print(f"\n{'='*60}")
        print(f"[Version Update] Creating project snapshot")
        print(f"[Version] {version}")
        if is_major_version:
            print(f"[Type] MAJOR VERSION CHANGE - USB installation required")
        else:
            print(f"[Type] Minor/Build update - OTA capable")
        print(f"[Location] {version_folder}")
        print(f"{'='*60}\n")
        
        # Metadata
        metadata = {
            'version': version,
            'timestamp': timestamp,
            'type': 'version_update',
            'device': 'ESP32-S3-Touch-LCD-7',
            'chip': 'esp32s3',
            'created_date': datetime.now().isoformat(),
            'description': 'Project state snapshot - incremental version update',
            'is_major_version': is_major_version,
            'requires_usb': is_major_version
        }
        
        # Build the firmware
        print("[1/3] Building firmware...")
        try:
            process = subprocess.Popen(
                ['pio', 'run', '-e', 'waveshare_7in'],
                cwd=self.project_dir,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1
            )
            
            # Stream output line by line
            for line in process.stdout:
                print(line.rstrip())
            
            process.wait(timeout=300)
            
            if process.returncode == 0:
                print("    ✓ Build successful")
                
                # Copy firmware binary
                firmware_src = self.project_dir / '.pio' / 'build' / 'waveshare_7in' / 'firmware.bin'
                if firmware_src.exists():
                    firmware_dst = version_folder / 'firmware.bin'
                    import shutil
                    shutil.copy2(firmware_src, firmware_dst)
                    print(f"    ✓ Firmware saved ({firmware_dst.stat().st_size} bytes)")
                    metadata['firmware'] = 'firmware.bin'
                    metadata['firmware_size'] = firmware_dst.stat().st_size
                else:
                    print("    ⚠ Firmware binary not found")
            else:
                print(f"    ✗ Build failed with exit code {process.returncode}")
                return None, None
        except Exception as e:
            print(f"    ✗ Build error: {e}")
            return None, None
        
        # Save metadata
        print("\n[2/3] Saving metadata...")
        metadata_file = version_folder / 'version_metadata.json'
        with open(metadata_file, 'w', encoding='utf-8') as f:
            json.dump(metadata, f, indent=2)
        print("    ✓ Metadata saved")
        
        # Commit to git
        print("\n[3/3] Committing to git...")
        try:
            # Add ALL changes (complete project snapshot)
            subprocess.run(['git', 'add', '-A'], 
                         cwd=self.project_dir, check=True)
            
            # Commit
            commit_msg = f"Version {version} - Incremental update"
            subprocess.run(['git', 'commit', '-m', commit_msg], 
                         cwd=self.project_dir, check=True)
            print(f"    ✓ Committed: {commit_msg}")
            print(f"    ✓ Full project state captured")
        except subprocess.CalledProcessError as e:
            print(f"    ⚠ Git commit skipped (possibly no changes)")
        except Exception as e:
            print(f"    ⚠ Git error: {e}")
        
        print(f"\n{'='*60}")
        print(f"[Version Update] ✓ Complete!")
        print(f"{'='*60}\n")
        
        # Upload to GitHub with appropriate type
        self.upload_to_github(version_folder, version, upload_type='version', is_major_version=is_major_version)
        
        return version_folder, version
    
    def full_backup(self):
        """Create a FULL backup from device hardware (increments MAJOR version)"""
        version = self.increment_version(increment_type='major')
        
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        backup_name = f"bronco_v{version}_{timestamp}_FULL"
        backup_folder = self.backups_dir / backup_name
        backup_folder.mkdir(exist_ok=True)
        
        print(f"\n{'='*60}")
        print(f"[FULL BACKUP] Complete device backup with version increment")
        print(f"[Version] {version} (MAJOR version bump)")
        print(f"[Location] {backup_folder}")
        print(f"{'='*60}\n")
        
        # Metadata
        metadata = {
            'version': version,
            'timestamp': timestamp,
            'type': 'full_backup',
            'device': 'ESP32-S3-Touch-LCD-7',
            'chip': 'esp32s3',
            'flash_size': '16MB',
            'backup_date': datetime.now().isoformat(),
            'description': 'Complete hardware + software backup from device',
            'regions': {}
        }
        
        # Backup each flash region
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
        
        # Create full flash dump (optional, for complete safety)
        print(f"\n[Backup] Creating full flash dump...")
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
        
        print(f"\n{'='*60}")
        print(f"[FULL BACKUP] ✓ Complete! Backup saved to:")
        print(f"              {backup_folder}")
        print(f"{'='*60}\n")
        
        # Always upload to GitHub (full backups always include ZIP for USB installation)
        self.upload_to_github(backup_folder, version, upload_type='full', is_major_version=True)
        
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
    
    def restore_device(self, backup_folder):
        """Restore device from a backup folder"""
        backup_path = Path(backup_folder)
        if not backup_path.exists():
            print(f"[Error] Backup folder not found: {backup_folder}")
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
    
    def upload_to_github(self, backup_folder, version, upload_type='version', is_major_version=False):
        """Upload backup/version to GitHub repository
        
        Args:
            upload_type: 'full' for full backup, 'version' for version update
            is_major_version: True if this is a major version change requiring USB installation
        """
        type_label = "FULL BACKUP" if upload_type == 'full' else "VERSION UPDATE"
        print(f"\n[GitHub] Uploading {type_label} to repository...")
        print(f"[GitHub] Target: {self.github_repo}/tree/{self.github_branch}/{self.github_folder}")
        
        # Try to get token from environment first
        github_token = os.environ.get('GITHUB_TOKEN')
        
        # If not in environment, try Windows Credential Manager
        if not github_token:
            try:
                import subprocess
                result = subprocess.run(
                    ['git', 'credential', 'fill'],
                    input=b'protocol=https\nhost=github.com\n\n',
                    capture_output=True,
                    timeout=5
                )
                if result.returncode == 0:
                    output = result.stdout.decode('utf-8')
                    for line in output.split('\n'):
                        if line.startswith('password='):
                            github_token = line.split('=', 1)[1].strip()
                            print("[GitHub] Retrieved token from Windows Credential Manager")
                            break
            except Exception as e:
                print(f"[GitHub] Could not retrieve token from credential manager: {e}")
        
        if not github_token:
            print("[GitHub] ✗ No GitHub token found")
            print("[GitHub] Set GITHUB_TOKEN environment variable or ensure git credentials are configured")
            return False
        
        backup_path = Path(backup_folder)
        
        # For OTA updates (minor/build versions), upload .bin file directly
        # For major versions or full backups, also upload .zip archive
        
        headers = {
            'Authorization': f'token {github_token}',
            'Accept': 'application/vnd.github.v3+json'
        }
        
        upload_success = True
        
        # ALWAYS upload the .bin file for OTA capability
        firmware_bin = backup_path / 'firmware.bin'
        if firmware_bin.exists():
            bin_name = f"bronco_v{version}.bin"
            print(f"[GitHub] Uploading OTA binary: {bin_name}")
            
            with open(firmware_bin, 'rb') as f:
                content = base64.b64encode(f.read()).decode('utf-8')
            
            bin_api_url = f"https://api.github.com/repos/{self.github_repo}/contents/{self.github_folder}/{bin_name}"
            
            data = {
                'message': f'Add OTA firmware v{version}',
                'content': content,
                'branch': self.github_branch
            }
            
            try:
                print(f"    Uploading {firmware_bin.stat().st_size / 1024 / 1024:.1f} MB...")
                response = requests.put(bin_api_url, headers=headers, json=data, timeout=300)
                
                if response.status_code in [200, 201]:
                    print(f"    ✓ BIN upload successful!")
                else:
                    print(f"    ✗ BIN upload failed: {response.status_code}")
                    print(f"    Response: {response.text}")
                    upload_success = False
            except Exception as e:
                print(f"    ✗ BIN upload error: {e}")
                upload_success = False
        else:
            print(f"[GitHub] ⚠ firmware.bin not found, skipping BIN upload")
        
        # Upload ZIP archive for full backups or major versions (USB installation)
        if upload_type == 'full' or is_major_version:
            import zipfile
            suffix = "_FULL" if upload_type == 'full' else ""
            zip_name = f"bronco_v{version}{suffix}.zip"
            zip_path = backup_path.parent / zip_name
            
            print(f"[GitHub] Creating archive for USB installation: {zip_name}")
            with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
                for file in backup_path.rglob('*'):
                    if file.is_file():
                        arcname = file.relative_to(backup_path.parent)
                        zipf.write(file, arcname)
            
            print(f"    ✓ Archive created ({zip_path.stat().st_size / 1024 / 1024:.1f} MB)")
            
            # Upload ZIP to GitHub
            zip_api_url = f"https://api.github.com/repos/{self.github_repo}/contents/{self.github_folder}/{zip_name}"
            # Upload ZIP to GitHub
            zip_api_url = f"https://api.github.com/repos/{self.github_repo}/contents/{self.github_folder}/{zip_name}"
            
            with open(zip_path, 'rb') as f:
                content = base64.b64encode(f.read()).decode('utf-8')
            
            message = f'Add {type_label.lower()} version {version}'
            data = {
                'message': message,
                'content': content,
                'branch': self.github_branch
            }
            
            try:
                print(f"    Uploading ZIP archive...")
                response = requests.put(zip_api_url, headers=headers, json=data, timeout=300)
                
                if response.status_code in [200, 201]:
                    print(f"    ✓ ZIP upload successful!")
                else:
                    print(f"    ✗ ZIP upload failed: {response.status_code}")
                    print(f"    Response: {response.text}")
                    upload_success = False
            except Exception as e:
                print(f"    ✗ ZIP upload error: {e}")
                upload_success = False
            finally:
                # Clean up zip file
                if zip_path.exists():
                    zip_path.unlink()
                    print(f"    Cleaned up temporary archive")
        
        if upload_success:
            print(f"[GitHub] ✓ Upload complete!")
            print(f"[GitHub] URL: https://github.com/{self.github_repo}/tree/{self.github_branch}/{self.github_folder}")
        else:
            print(f"[GitHub] ⚠ Upload completed with errors")
        
        return upload_success
    
    def list_github_versions(self):
        """List available versions in GitHub repository"""
        print(f"\n[GitHub] Checking for available versions...")
        print(f"[GitHub] Repository: {self.github_repo}/{self.github_folder}")
        
        api_url = f"https://api.github.com/repos/{self.github_repo}/contents/{self.github_folder}"
        
        try:
            response = requests.get(api_url, timeout=10)
            if response.status_code == 200:
                files = response.json()
                versions = []
                
                for file in files:
                    if file['name'].endswith('.zip') and file['name'].startswith('bronco_v'):
                        # Extract version from filename
                        match = re.search(r'bronco_v([\d.]+)', file['name'])
                        if match:
                            version_str = match.group(1)
                            is_full = '_FULL' in file['name']
                            versions.append({
                                'version': version_str,
                                'filename': file['name'],
                                'size': file['size'],
                                'download_url': file['download_url'],
                                'type': 'Full Backup' if is_full else 'Version Update'
                            })
                
                # Sort by version (newest first)
                versions.sort(key=lambda x: list(map(int, x['version'].split('.'))), reverse=True)
                
                if versions:
                    print(f"\n{'='*70}")
                    print(f"Available Versions ({len(versions)} found):")
                    print(f"{'='*70}")
                    for idx, v in enumerate(versions, 1):
                        size_mb = v['size'] / (1024 * 1024)
                        print(f"{idx}. v{v['version']} - {v['type']} ({size_mb:.1f} MB)")
                        print(f"   {v['filename']}")
                    print(f"{'='*70}\n")
                else:
                    print("[GitHub] No versions found in repository")
                
                return versions
            else:
                print(f"[GitHub] ✗ Failed to fetch versions: {response.status_code}")
                return []
        except Exception as e:
            print(f"[GitHub] ✗ Error: {e}")
            return []
    
    def list_backups(self):
        """List all available backups"""
        backups = sorted(self.backups_dir.glob('bronco_v*'), reverse=True)
        
        if not backups:
            print("[Backups] No backups found")
            return []
        
        print(f"\n{'='*60}")
        print(f"Available Backups ({len(backups)} found):")
        print(f"{'='*60}")
        
        backup_info = []
        for idx, backup in enumerate(backups, 1):
            metadata_file = backup / 'backup_metadata.json'
            if metadata_file.exists():
                with open(metadata_file, 'r') as f:
                    metadata = json.load(f)
                    version = metadata.get('version', 'unknown')
                    date = metadata.get('backup_date', 'unknown')
                    size = sum(f.stat().st_size for f in backup.rglob('*') if f.is_file())
                    size_mb = size / (1024 * 1024)
                    
                    print(f"{idx}. v{version} - {date[:19]} ({size_mb:.1f} MB)")
                    print(f"   {backup}")
                    backup_info.append({
                        'path': backup,
                        'version': version,
                        'date': date,
                        'size': size
                    })
            else:
                print(f"{idx}. {backup.name} (metadata missing)")
        
        print(f"{'='*60}\n")
        return backup_info


def main():
    parser = argparse.ArgumentParser(
        description='ESP32-S3 Bronco Controls Version & Backup Manager',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  # Create version update from current project (auto-upload to GitHub)
  python backup_restore_manager.py version
  
  # Create FULL backup from device (increments MAJOR version, auto-upload)
  python backup_restore_manager.py full-backup
  
  # List local backups
  python backup_restore_manager.py list
  
  # List available versions on GitHub
  python backup_restore_manager.py list-github
  
  # Restore from latest backup
  python backup_restore_manager.py restore
  
  # Restore from specific backup
  python backup_restore_manager.py restore --backup "backups/bronco_v2.0.0_20260130_123456_FULL"
        '''
    )
    
    parser.add_argument('action', choices=['version', 'full-backup', 'restore', 'list', 'list-github'],
                       help='Action to perform')
    parser.add_argument('--port', default='COM5',
                       help='Serial port (default: COM5)')
    parser.add_argument('--baud', type=int, default=460800,
                       help='Baud rate (default: 460800)')
    parser.add_argument('--backup', type=str,
                       help='Backup folder to restore from')
    
    args = parser.parse_args()
    
    manager = BackupRestoreManager(port=args.port, baud=args.baud)
    
    if args.action == 'version':
        manager.version_update()
    
    elif args.action == 'full-backup':
        manager.full_backup()
    
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
    
    elif args.action == 'list-github':
        manager.list_github_versions()
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
