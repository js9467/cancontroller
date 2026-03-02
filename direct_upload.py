#!/usr/bin/env python3
"""
Direct firmware upload via serial/esptool
Attempts to upload firmware directly without going through OTA system
"""

import subprocess
import sys
import time

# Firmware path - PlatformIO build output
FW_PATH = ".pio/build/waveshare_7in/firmware.bin"
COM_PORT = "COM5"

def check_bootloader_mode():
    """Check if device is in bootloader mode"""
    print("[*] Checking for bootloader mode...")
    result = subprocess.run(['esptool.py', '--port', COM_PORT, 'read_mac'], 
                          capture_output=True, text=True, timeout=5)
    if result.returncode == 0:
        print(f"✓ Device in bootloader mode: {result.stdout.strip()}")
        return True
    else:
        print(f"✗ Device NOT in bootloader mode")
        print(f"  Error: {result.stderr}")
        return False

def enter_bootloader():
    """Attempt to put device in bootloader mode"""
    print("\n[*] Attempting to enter bootloader mode...")
    print("  1. Connect USB to device")
    print("  2. Hold BOOT button")
    print("  3. Press RESET button while holding BOOT")
    print("  4. Release BOOT button")
    print("  Press ENTER when device is in bootloader mode...")
    input()
    
    return check_bootloader_mode()

def upload_firmware(fw_path):
    """Upload firmware via esptool"""
    print(f"\n[*] Uploading firmware: {fw_path}")
    
    cmd = [
        'esptool.py',
        '--chip', 'esp32s3',
        '--port', COM_PORT,
        '--baud', '115200',
        'write_flash',
        '0x0', fw_path
    ]
    
    print(f"  Command: {' '.join(cmd)}")
    
    try:
        result = subprocess.run(cmd, timeout=120)
        if result.returncode == 0:
            print("✓ Upload successful!")
            return True
        else:
            print(f"✗ Upload failed with code {result.returncode}")
            return False
    except subprocess.TimeoutExpired:
        print("✗ Upload timed out")
        return False
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

def main():
    print("=" * 60)
    print("ESP32-S3 Direct Firmware Upload via esptool")
    print("=" * 60)
    
    # Check if esptool is available
    try:
        subprocess.run(['esptool.py', '--version'], capture_output=True, timeout=5)
    except FileNotFoundError:
        print("ERROR: esptool.py not found. Install with: pip install esptool")
        sys.exit(1)
    
    # Try to enter bootloader mode
    if not enter_bootloader():
        print("\nERROR: Could not detect device in bootloader mode")
        print("Try holding BOOT button while pressing RESET, then immediately release BOOT")
        sys.exit(1)
    
    # Upload firmware
    if not upload_firmware(FW_PATH):
        print("\nERROR: Firmware upload failed")
        sys.exit(1)
    
    print("\n[*] Upload complete! Device should reboot automatically")
    print("[*] Waiting 5 seconds for device to boot...")
    time.sleep(5)
    
    print("Done!")

if __name__ == "__main__":
    main()
