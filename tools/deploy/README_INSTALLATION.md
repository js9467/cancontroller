# 🚗 Bronco Controls - Installation Guide

## Quick Start (3 Easy Steps)

### 📥 Step 1: Download

**Right-click** this link and choose **"Save link as..."**:

👉 **[QuickSetup.bat](https://raw.githubusercontent.com/js9467/cancontroller/master/tools/deploy/QuickSetup.bat)**

Save it to your **Downloads** folder.

> ⚠️ **Don't just click** - you need to right-click and save!

---

### 🔌 Step 2: Connect Device

1. Plug your ESP32 device into your computer with a **USB-C cable**
2. Make sure it's a **data cable** (not charge-only)
3. Device screen should light up

---

### ▶️ Step 3: Run Installer

1. Find **QuickSetup.bat** in your Downloads
2. **Double-click** it to run
3. Click **"Yes"** if Windows asks for permission
4. **Follow the on-screen instructions**

---

## 🔧 First Time Setup (Drivers)

If this is your first time, you'll need to install USB drivers (one-time only):

### When Zadig Opens:

1. Click **Options** → **List All Devices**
2. Select from dropdown:
   - **"USB JTAG/serial debug unit"** OR
   - **"USB Composite Device (303A:1001)"**
3. Make sure it shows **"WinUSB"** in the driver box
4. Click **"Install Driver"** or **"Replace Driver"**
5. Wait for "Driver installed successfully"
6. **Close Zadig**
7. **Unplug and replug** your ESP32

The installer will continue automatically!

⏱️ **First time:** ~3-5 minutes  
⏱️ **After drivers installed:** ~30 seconds

---

## ❓ Troubleshooting

### "No COM port found"

**Try these in order:**

1. **Different cable** - Many USB-C cables are charge-only
2. **Different USB port** - Try USB 2.0 (black ports) instead of USB 3.0 (blue)
3. **Unplug and replug** - Wait 5 seconds between
4. **Close other programs** - Arduino IDE, PuTTY, serial monitors
5. **Restart computer** - Sometimes Windows needs a fresh start

### Zadig doesn't show my device

1. In Zadig: **Options** → **List All Devices** ✅
2. Look for these names:
   - "USB JTAG/serial debug unit"
   - "USB Composite Device"
   - Anything with "303A" in the name
3. If you see nothing, unplug/replug and check again

### "Access Denied" / "Permission Denied"

- Run QuickSetup.bat as Administrator
- Close programs using COM port
- Unplug/replug device

---

## 📦 What You're Shipping

### For Customers:

Send them this link:
```
https://raw.githubusercontent.com/js9467/cancontroller/master/tools/deploy/QuickSetup.bat
```

Or create a short link (e.g., bit.ly/bronco-install)

### Simple Instructions to Include:

```
BRONCO CONTROLS - QUICK SETUP

1. Download: [your-short-url-here]
   (Right-click → Save link as...)

2. Connect device via USB

3. Run the downloaded file

4. Follow on-screen instructions

Need help? Contact: [your-support-email]
```

---

## ✅ After Installation

The installer will:
- ✅ Flash the latest firmware
- ✅ Reboot your device
- ✅ Open the web interface automatically
- ✅ Show you the device IP address

### If WiFi is not configured:

1. Device creates WiFi network: **"Bronco-Controls-XXXXXX"**
2. Connect to that network
3. Visit **http://192.168.4.1**
4. Configure your WiFi settings

---

## 🗂️ Files Saved

Temporary files are stored in:
```
C:\Users\[YourName]\AppData\Local\BroncoControls\
```

You can delete this folder anytime to clear the cache.

---

## 🆘 Still Having Issues?

Contact support with:
- ✉️ Error message (take a screenshot)
- 💻 Windows version
- 🔌 Whether Device Manager shows any yellow warning icons
- 📷 Photo of your USB cable ends (to verify it's a data cable)

---

## 📋 Technical Details

For developers and advanced users:

### Manual Installation

Download and run PowerShell script directly:
```powershell
powershell -ExecutionPolicy Bypass -File BroncoFlasher.ps1
```

### Specify COM Port
```powershell
powershell -ExecutionPolicy Bypass -File BroncoFlasher.ps1 -Port COM3
```

### List Available Ports
```powershell
powershell -ExecutionPolicy Bypass -File BroncoFlasher.ps1 -ListPorts
```

### Offline Mode (Use Cached Firmware)
```powershell
powershell -ExecutionPolicy Bypass -File BroncoFlasher.ps1 -OfflineMode
```

---

## 🔄 Updating Firmware

Use the same QuickSetup.bat file!
- It automatically downloads the latest version
- Your settings are preserved
- Takes ~30 seconds

---

**Made with ❤️ for Bronco Controls**
