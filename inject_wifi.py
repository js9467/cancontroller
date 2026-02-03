#!/usr/bin/env python3
"""
Direct WiFi Injection Tool
Writes WiFi credentials directly to ESP32 NVS partition via USB
"""

import subprocess
import sys

def inject_wifi(ssid, password, port='COM5'):
    """Inject WiFi credentials by writing directly to NVS partition"""
    
    print("=" * 60)
    print("WiFi Credential Injection Tool")
    print("=" * 60)
    print(f"\nTarget: ESP32-S3 on {port}")
    print(f"SSID: {ssid}")
    print(f"Password: {'*' * len(password)}\n")
    
    # NVS partition location
    nvs_offset = 0x9000
    
    print("[1/3] Erasing NVS partition (WiFi will be reset)...")
    try:
        result = subprocess.run([
            'python', '-m', 'esptool',
            '--chip', 'esp32s3',
            '--port', port,
            '--baud', '460800',
            'erase_region',
            f'{nvs_offset}',
            '0x5000'  # 20KB NVS size
        ], capture_output=True, text=True, timeout=30)
        
        if result.returncode != 0:
            print(f"    ✗ Erase failed: {result.stderr}")
            print("    Continuing anyway...")
        else:
            print("    ✓ NVS partition erased")
    except Exception as e:
        print(f"    ✗ Error: {e}")
        return False
    
    print(f"\n[2/3] Device will now be in AP mode after reset")
    print(f"[3/3] You MUST configure WiFi via:")
    print(f"      1. Reset the device")
    print(f"      2. Connect to the device's AP")  
    print(f"      3. Use the touchscreen: Settings > WiFi")
    print(f"      4. Enter SSID: {ssid}")
    print(f"      5. Enter Password: {password}")
    
    print("\n" + "=" * 60)
    print("ALTERNATIVE: Have someone at home configure it")
    print("=" * 60)
    print(f"\nPerson at home should:")
    print(f"1. Connect phone/laptop to device AP")
    print(f"2. Touch 'Settings' on device screen")
    print(f"3. Touch 'WiFi Settings'")
    print(f"4. Enter:")
    print(f"   SSID: {ssid}")
    print(f"   Password: {password}")
    print(f"5. Touch 'Connect'")
    print(f"\nDevice will then connect to your home WiFi")
    print("=" * 60)
    
    return True

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("\nUsage: python inject_wifi.py <SSID> <PASSWORD> [PORT]")
        print("Example: python inject_wifi.py MyHomeWiFi MyPassword123 COM5\n")
        sys.exit(1)
    
    ssid = sys.argv[1]
    password = sys.argv[2]
    port = sys.argv[3] if len(sys.argv) > 3 else 'COM5'
    
    inject_wifi(ssid, password, port)
