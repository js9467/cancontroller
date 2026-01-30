# Bronco Controls - Web Configuration Quick Start

## 🚀 What's New

Your Bronco Controls device has been upgraded with a **complete web-based configuration system**! You can now configure everything through a modern web interface without touching the code.

## 📱 Quick Start (3 Steps)

### Step 1: Build and Upload
```bash
cd d:\Software\Bronco-Controls-4
pio run --target upload
```

### Step 2: Connect to WiFi
- Look for WiFi network: **BroncoControls**
- Password: **bronco123**

### Step 3: Open Web Interface
- Open browser: **http://192.168.4.250**
- Use the **Join Home WiFi** card (optional) to push your home SSID/password so the unit also connects to your LAN.
- Tap **Scan Networks** to discover nearby SSIDs from the ESP32 and auto-fill the join form.
- After it grabs a DHCP lease, read the LAN IP from the new on-screen network badges at the top of the touchscreen.

## ✨ What You Can Do

### Via Web Interface:
- ✅ Add/delete/modify pages
- ✅ Configure button colors, text, and positions
- ✅ Assign J1939 CAN frames to buttons
- ✅ Change WiFi settings or trigger the "Join Home WiFi" workflow
- ✅ Export/import configurations
- ✅ Monitor system status

### Button Configuration:
Each button can be customized with:
- Custom text and colors
- Position and size on screen
- J1939 CAN frame parameters:
  - PGN (Parameter Group Number)
  - Priority, Source/Dest addresses
  - 8 bytes of data

### Example Use Cases:
1. **Window Control**: Button sends CAN frame to raise/lower windows
2. **Door Locks**: Button sends CAN frame to lock/unlock doors
3. **Running Boards**: Button sends CAN frame to deploy/retract boards
4. **Custom Functions**: Any CAN-controlled feature in your vehicle

## 🔧 Hardware Requirements

- ESP32-S3 with display and touch
- CAN transceiver (optional, for J1939 features)
- Connect CAN_TX to GPIO 21 and CAN_RX to GPIO 22 (or modify in code)

## 📊 System Architecture

```
┌─────────────────────┐
│   Web Interface     │ ← Access via WiFi
│  (HTML/CSS/JS)      │
└──────────┬──────────┘
           │
           │ REST API
           ▼
┌─────────────────────┐
│   Web Server        │
│  (AsyncWebServer)   │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Config Manager     │ ◄─► LittleFS Storage
│  (JSON Config)      │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   UI Builder        │ → Builds LVGL UI dynamically
│  (Dynamic LVGL)     │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   Touch Display     │ → User interacts
│   (480x272 LVGL)    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   CAN Manager       │ → Sends J1939 frames
│  (J1939 @ 250kbps)  │
└─────────────────────┘
```

## 🎯 Configuration Workflow

1. **Design on Web**: Use web interface to build pages and buttons
2. **Assign CAN Frames**: Configure what each button does
3. **Test on Device**: Touch buttons to send CAN commands
4. **Export Config**: Save your configuration as JSON backup
5. **Share/Reuse**: Import config to other devices

## 📝 Default Configuration

On first boot, the device creates a default configuration:
- 1 Home page with 4 sample buttons
- WiFi AP mode enabled
- No CAN frames assigned (add them via web interface)

## 🔍 Files Created

New files in your project:
```
src/
├── main.cpp                 - Web-enabled entry point
├── backup/main_original.cpp - Archived factory main (reference only)
├── config_manager.h/cpp     - Configuration storage
├── web_server.h/cpp         - REST API server
├── web_interface.h          - Web UI (HTML/CSS/JS)
├── ui_builder.h/cpp         - Dynamic UI generator
└── can_manager.h/cpp        - J1939 CAN support
```

## 🛠️ Customization

### Change CAN Pins
Edit the call in [main.cpp](src/main.cpp#L150-L165) where `CanManager::instance().begin()` is invoked:
```cpp
// Defaults: TX=21, RX=22 @ 250 kbps
CanManager::instance().begin();

// Example: use GPIO17/18 if you rewire the transceiver
#include <hal/gpio_types.h>
CanManager::instance().begin(static_cast<gpio_num_t>(17), static_cast<gpio_num_t>(18), 250000);
```

### Change Default WiFi Credentials
Edit device settings via web interface or modify [config_manager.cpp](src/config_manager.cpp) default values.

### Change Page Limits
Edit [config_types.h](src/config_types.h#L11-L14):
```cpp
constexpr std::size_t MAX_PAGES = 20;
constexpr std::size_t MAX_BUTTONS_PER_PAGE = 12;
```

## 🆘 Troubleshooting

**Can't see WiFi network?**
- Wait 30 seconds after boot
- Check serial monitor for WiFi status
- Try resetting the device

**Web interface won't load?**
- Verify IP: http://192.168.4.250 (or read the LAN IP from the on-screen badges if you've joined your garage WiFi)
- Try different browser
- Check WiFi connection

**CAN frames not sending?**
- Check CAN hardware connections
- Verify CAN pins in can_manager.h
- Look for CAN errors in serial monitor
- Ensure 120Ω termination resistors on CAN bus

**Device won't boot?**
- Check serial monitor for errors
- May need to erase flash: `pio run --target erase`
- Then re-upload: `pio run --target upload`

## 📖 Documentation

- [WEB_CONFIGURATION_GUIDE.md](WEB_CONFIGURATION_GUIDE.md) - Complete guide
- Serial Monitor - Real-time debug info
- Source code comments - Implementation details

## 🎉 Next Steps

1. Build and upload the firmware
2. Connect to the web interface
3. Create your first custom page
4. Configure some buttons with CAN frames
5. Test on your vehicle
6. Export your configuration as backup

Enjoy your web-configurable Bronco Controls! 🚙✨
