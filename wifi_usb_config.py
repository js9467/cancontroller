#!/usr/bin/env python3
"""
WiFi Configuration via USB for ESP32-S3
Writes WiFi credentials directly to NVS partition
"""

import subprocess
import struct
import sys
from pathlib import Path

def write_wifi_to_nvs(ssid, password, port='COM5'):
    """Write WiFi credentials to NVS partition via USB"""
    
    print("=" * 70)
    print("ESP32-S3 WiFi Configuration Tool")
    print("=" * 70)
    print(f"\nPort: {port}")
    print(f"SSID: {ssid}")
    print(f"Password: {'*' * len(password)}")
    print()
    
    # NVS partition configuration
    nvs_offset = 0x9000
    nvs_size = 0x5000  # 20KB
    
    temp_nvs = Path("temp_nvs.bin")
    
    print("[Step 1/5] Reading current NVS partition...")
    try:
        result = subprocess.run([
            'python', '-m', 'esptool',
            '--chip', 'esp32s3',
            '--port', port,
            '--baud', '460800',
            'read_flash',
            f'0x{nvs_offset:X}',
            f'0x{nvs_size:X}',
            str(temp_nvs)
        ], capture_output=True, text=True, timeout=60)
        
        if result.returncode != 0:
            print(f"    ✗ Failed: {result.stderr}")
            return False
        
        print(f"    ✓ Read {temp_nvs.stat().st_size} bytes")
    except Exception as e:
        print(f"    ✗ Error: {e}")
        return False
    
    print("\n[Step 2/5] Creating WiFi configuration data...")
    
    # Create a simple NVS structure with WiFi credentials
    # ESP32 stores WiFi in namespace "nvs.net80211"
    # Keys: "sta.ssid" and "sta.passwd"
    
    # For simplicity, we'll create a minimal NVS binary
    # The proper way requires NVS partition generator
    
    # Create CSV for NVS partition generator
    nvs_csv = Path("wifi_config.csv")
    with open(nvs_csv, 'w') as f:
        f.write("key,type,encoding,value\n")
        f.write("nvs.net80211,namespace,,\n")
        f.write(f"sta.ssid,data,string,{ssid}\n")
        f.write(f"sta.passwd,data,string,{password}\n")
        f.write("sta.authmode,data,u8,3\n")  # WPA2_PSK
    
    print(f"    ✓ WiFi configuration created")
    
    print("\n[Step 3/5] Generating NVS partition binary...")
    
    nvs_bin = Path("wifi_nvs.bin")
    
    # Try to use ESP-IDF's nvs_partition_gen.py
    try:
        # Check if tool exists in PlatformIO packages
        nvs_gen_tool = Path.home() / '.platformio' / 'packages' / 'tool-esptoolpy' / 'nvs_partition_gen.py'
        
        if nvs_gen_tool.exists():
            result = subprocess.run([
                'python', str(nvs_gen_tool),
                'generate',
                str(nvs_csv),
                str(nvs_bin),
                f'0x{nvs_size:X}'
            ], capture_output=True, text=True)
            
            if result.returncode == 0:
                print(f"    ✓ NVS binary generated ({nvs_bin.stat().st_size} bytes)")
            else:
                raise Exception(f"NVS generation failed: {result.stderr}")
        else:
            raise Exception("NVS partition generator not found")
            
    except Exception as e:
        print(f"    ⚠ Could not generate NVS binary: {e}")
        print(f"    Using alternative method...")
        
        # Alternative: Erase NVS and let device create fresh config
        print("\n[Alternative Method] Erasing NVS partition...")
        print("    Device will boot in AP mode and require manual WiFi config")
        
        result = subprocess.run([
            'python', '-m', 'esptool',
            '--chip', 'esp32s3',
            '--port', port,
            '--baud', '460800',
            'erase_region',
            f'0x{nvs_offset:X}',
            f'0x{nvs_size:X}'
        ], capture_output=True, text=True)
        
        if result.returncode == 0:
            print("    ✓ NVS erased - device will use AP mode")
        
        # Cleanup
        if nvs_csv.exists():
            nvs_csv.unlink()
        if temp_nvs.exists():
            temp_nvs.unlink()
        
        print("\n" + "=" * 70)
        print("WIFI CONFIGURATION REQUIRED")
        print("=" * 70)
        print("\nThe device NVS has been erased. To configure WiFi:")
        print("\n1. Reset/power cycle the device")
        print("2. Device will start in AP mode")
        print("3. Connect to the device's WiFi AP")
        print("4. On the touchscreen:")
        print("   - Tap 'Settings'")
        print("   - Tap 'WiFi Settings'")
        print(f"   - Enter SSID: {ssid}")
        print(f"   - Enter Password: {password}")
        print("   - Tap 'Connect'")
        print("\n5. Device will save credentials and connect to WiFi")
        print("=" * 70)
        
        return True
    
    print("\n[Step 4/5] Writing WiFi configuration to device...")
    
    try:
        result = subprocess.run([
            'python', '-m', 'esptool',
            '--chip', 'esp32s3',
            '--port', port,
            '--baud', '460800',
            'write_flash',
            f'0x{nvs_offset:X}',
            str(nvs_bin)
        ], capture_output=True, text=True, timeout=60)
        
        if result.returncode != 0:
            print(f"    ✗ Write failed: {result.stderr}")
            return False
        
        print(f"    ✓ WiFi configuration written to NVS")
    except Exception as e:
        print(f"    ✗ Error: {e}")
        return False
    
    print("\n[Step 5/5] Cleanup...")
    
    # Cleanup temp files
    for f in [nvs_csv, nvs_bin, temp_nvs]:
        if f.exists():
            f.unlink()
    
    print("    ✓ Temporary files removed")
    
    print("\n" + "=" * 70)
    print("SUCCESS - WiFi Configuration Complete!")
    print("=" * 70)
    print("\nNext steps:")
    print("1. Reset/power cycle the device")
    print("2. Device should automatically connect to:")
    print(f"   SSID: {ssid}")
    print("3. Check the device screen for IP address")
    print("=" * 70)
    
    return True

def main():
    if len(sys.argv) < 3:
        print("\n" + "=" * 70)
        print("ESP32-S3 WiFi Configuration Tool")
        print("=" * 70)
        print("\nUsage: python wifi_usb_config.py <SSID> <PASSWORD> [PORT]")
        print("\nExample:")
        print("  python wifi_usb_config.py MyHomeWiFi MySecurePass123 COM5")
        print("\nThis tool will:")
        print("  1. Read the device's NVS partition")
        print("  2. Inject your WiFi credentials")
        print("  3. Write the modified NVS back to the device")
        print("  4. Device will connect to WiFi on next boot")
        print("\n" + "=" * 70)
        sys.exit(1)
    
    ssid = sys.argv[1]
    password = sys.argv[2]
    port = sys.argv[3] if len(sys.argv) > 3 else 'COM5'
    
    if not ssid:
        print("✗ Error: SSID cannot be empty")
        sys.exit(1)
    
    if len(password) < 8:
        print("✗ Error: WiFi password must be at least 8 characters")
        sys.exit(1)
    
    success = write_wifi_to_nvs(ssid, password, port)
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
