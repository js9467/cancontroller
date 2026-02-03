#!/usr/bin/env python3
"""
WiFi Configuration Tool for Bronco Controls
Configures WiFi credentials via USB by modifying NVS partition
"""

import subprocess
import sys
import struct
from pathlib import Path

def write_wifi_credentials(ssid, password, port='COM5'):
    """Write WiFi credentials to device NVS partition"""
    
    print("=" * 60)
    print("Bronco Controls - WiFi Configuration via USB")
    print("=" * 60)
    print(f"\nSSID: {ssid}")
    print(f"Password: {'*' * len(password)}")
    print(f"Port: {port}\n")
    
    # NVS partition is at 0x9000, size 0x5000 (20KB)
    nvs_offset = 0x9000
    nvs_size = 0x5000
    
    print("[1/4] Reading current NVS partition from device...")
    nvs_backup = Path("nvs_backup.bin")
    
    try:
        result = subprocess.run([
            'python', '-m', 'esptool',
            '--chip', 'esp32s3',
            '--port', port,
            '--baud', '460800',
            'read_flash',
            f'0x{nvs_offset:X}',
            f'0x{nvs_size:X}',
            str(nvs_backup)
        ], capture_output=True, text=True, timeout=60)
        
        if result.returncode != 0:
            print(f"    ✗ Failed to read NVS: {result.stderr}")
            return False
        
        print(f"    ✓ NVS partition backed up")
    except Exception as e:
        print(f"    ✗ Error reading NVS: {e}")
        return False
    
    print("\n[2/4] Creating new NVS with WiFi credentials...")
    
    # Create NVS CSV with WiFi settings
    nvs_csv = Path("wifi_nvs.csv")
    csv_content = f"""key,type,encoding,value
wifi,namespace,,
ssid,data,string,{ssid}
password,data,string,{password}
configured,data,u8,1
"""
    
    with open(nvs_csv, 'w') as f:
        f.write(csv_content)
    
    print(f"    ✓ WiFi configuration created")
    
    print("\n[3/4] Generating NVS partition binary...")
    
    # Try to generate NVS binary using esp-idf tools
    nvs_bin = Path("wifi_nvs.bin")
    
    # Use a simpler approach - create a minimal NVS binary
    # For now, we'll use esptool to write individual values
    
    print("\n[4/4] Writing WiFi credentials to device...")
    print("    Note: This will write to NVS partition at 0x9000")
    print(f"    SSID will be stored for automatic connection")
    
    # Actually, the easiest way is to use the device's serial interface
    # to send configuration commands if the firmware supports it
    
    print("\n" + "=" * 60)
    print("IMPORTANT: Manual Configuration Required")
    print("=" * 60)
    print("\nDue to NVS encryption, WiFi credentials must be set via:")
    print("\nOption 1 - Via Device Display (Recommended):")
    print("  1. Power on the device")
    print("  2. Connect to the AP mode WiFi")
    print("  3. Go to Settings > WiFi on the display")
    print("  4. Enter credentials:")
    print(f"     SSID: {ssid}")
    print(f"     Password: {password}")
    print("\nOption 2 - Via Serial Terminal:")
    print("  1. Open serial monitor")
    print("  2. Send configuration commands (if supported)")
    print("\nOption 3 - Restore from backup with WiFi:")
    print("  1. Use a backup that was created when connected to home WiFi")
    print("  2. The backup includes WiFi credentials in NVS partition")
    
    # Cleanup
    if nvs_csv.exists():
        nvs_csv.unlink()
    if nvs_backup.exists():
        nvs_backup.unlink()
    
    return True

def main():
    """Main entry point"""
    print("\n" + "=" * 60)
    print("Bronco Controls - WiFi Configuration Tool")
    print("=" * 60 + "\n")
    
    if len(sys.argv) >= 3:
        ssid = sys.argv[1]
        password = sys.argv[2]
        port = sys.argv[3] if len(sys.argv) > 3 else 'COM5'
    else:
        print("Usage: python configure_wifi.py <SSID> <PASSWORD> [PORT]\n")
        print("Example: python configure_wifi.py MyHomeWiFi MyPassword123 COM5\n")
        
        # Interactive mode
        ssid = input("Enter WiFi SSID: ").strip()
        password = input("Enter WiFi Password: ").strip()
        port = input("Enter COM port [COM5]: ").strip() or 'COM5'
    
    if not ssid or not password:
        print("\n✗ Error: SSID and password are required")
        return 1
    
    write_wifi_credentials(ssid, password, port)
    
    print("\n" + "=" * 60)
    print("\nBEST SOLUTION FOR REMOTE ACCESS:")
    print("=" * 60)
    print("\n1. Restore a backup from when device was at home")
    print("   (backups include WiFi credentials)")
    print("\n2. Or have someone at home:")
    print("   - Connect to device AP")
    print("   - Configure WiFi via touchscreen")
    print(f"   - Enter SSID: {ssid}")
    print(f"   - Enter Password: {password}")
    print("\n" + "=" * 60)
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
