# Bronco Controls - Customer Installation Guide

## One-Click Installation

We've made installation as simple as possible. Just follow these steps:

### Step 1: Download the Installer

Click this link to download and run the installer:

**[Download Bronco Controls Installer](https://raw.githubusercontent.com/js9467/autotouchscreen/main/tools/deploy/Install.bat)**

- Right-click the link and choose "Save link as..."
- Save the `Install.bat` file to your Downloads folder
- **Important**: Do NOT just click the link in the browser, as it will show the file contents instead of downloading it

### Step 2: Connect Your Device

1. Connect your ESP32-S3 device to your computer using a USB-C cable
2. Make sure you're using a **data cable** (not a charge-only cable)
3. The device should power on (screen lights up)

### Step 3: Run the Installer

1. Find the `Install.bat` file you downloaded
2. **Double-click** to run it
3. Click "Yes" if Windows asks for permission

The installer will:
- ✅ Download the latest firmware automatically
- ✅ Install USB drivers if needed (with simple instructions)
- ✅ Flash your device
- ✅ Open the web interface automatically

## What to Expect

### First Time Installation (Drivers Needed)

If this is your first time, the installer will detect that your ESP32 needs drivers and will:

1. **Download Zadig** - A trusted USB driver installer
2. **Open Zadig** with clear instructions
3. You'll follow simple on-screen steps (shown in the command window):
   - Select "Options" → "List All Devices"
   - Choose "USB JTAG/serial debug unit" from the dropdown
   - Click "Install Driver"
4. **Unplug and replug** your device when prompted
5. Continue with automatic flashing

**Total time:** ~3-5 minutes (first time with drivers)

### Subsequent Installations

After drivers are installed once, future updates take only:
- ✅ 30-60 seconds to flash
- ✅ No driver steps needed
- ✅ Automatic firmware download

## Troubleshooting

### "No COM port found"

**Solution 1: Use a different cable**
- Many USB-C cables are charge-only and don't transfer data
- Try a cable that came with a phone or external hard drive

**Solution 2: Try a different USB port**
- USB 2.0 ports (black) often work better than USB 3.0 (blue)
- Avoid USB hubs - connect directly to your computer

**Solution 3: Replug the device**
- Unplug the ESP32
- Wait 5 seconds
- Plug it back in
- Run Install.bat again

### Driver Installation Issues

If Zadig fails or you see errors:

1. Download Zadig manually: https://zadig.akeo.ie/
2. Run it as Administrator
3. Follow the same steps (Options → List All Devices → Select device → Install)

### "Permission Denied" or "Access Denied"

- Close any programs that might be using the COM port:
  - Arduino IDE
  - PuTTY
  - Any serial monitor tools
- Unplug and replug the device
- Try again

## Advanced Options

### Manual Port Selection

If auto-detection doesn't work, you can specify a COM port:

```powershell
powershell -ExecutionPolicy Bypass -File BroncoFlasher.ps1 -Port COM3
```

Replace `COM3` with your actual port number.

### Check Available Ports

To see all available serial ports:

```powershell
powershell -ExecutionPolicy Bypass -File BroncoFlasher.ps1 -ListPorts
```

## What Happens After Flashing?

1. ✅ Device reboots automatically
2. ✅ Connects to WiFi (if configured)
3. ✅ Web interface opens in your browser
4. ✅ You can configure settings via the web interface

If WiFi isn't configured yet:
- The device creates a WiFi access point
- Connect to "Bronco-Controls-XXXXXX"
- Visit http://192.168.4.1
- Configure your WiFi network

## Support

If you encounter issues:

1. Check this guide's troubleshooting section
2. Make sure you're using a data cable
3. Try a different USB port
4. Contact support with:
   - What error message you see
   - What happens when you plug in the device
   - Whether Device Manager shows any devices with yellow warnings

## File Locations

The installer saves temporary files to:
- `%LOCALAPPDATA%\BroncoControls\`

You can safely delete this folder at any time to clear the cache.

## Shipping to Customers

**For distributors/resellers:**

Just send your customer this link:
```
https://raw.githubusercontent.com/js9467/autotouchscreen/main/tools/deploy/Install.bat
```

Or provide these simple instructions:
1. Download: [bit.ly/bronco-install] (create a short URL)
2. Connect device via USB
3. Run the downloaded file
4. Follow on-screen prompts

That's it! No technical knowledge required.
