# Bronco Controls - Automotive HMI for Waveshare ESP32-S3 Touch LCD

A polished, rugged automotive HMI (Human-Machine Interface) with a Bronco-themed design for Waveshare ESP32-S3 Touch LCD boards.

> Repository mirror: https://github.com/js9467/autotouchscreen

## ⚠️ IMPORTANT: Deployment Process

**All code changes MUST use the standardized deployment script to prevent version management issues:**

```powershell
.\deploy.ps1 -Changelog "Description of changes"
```

**See [DEPLOYMENT_README.md](DEPLOYMENT_README.md) for complete instructions.**

### For AI Agents
- Read [.ai-instructions.md](.ai-instructions.md) - Contains critical rules
- **NEVER** manually edit: `src/version_auto.h`, `ota_functions/manifest.json`, or manifest files
- **ALWAYS** use `deploy.ps1` for deployments
- Cursor IDE: Rules in [.cursorrules](.cursorrules)

## Hardware Specifications

- **Display**: 800×480 RGB LCD with ST7262 controller (4.3" and 7.0" panels)
- **Touch**: GT911 capacitive I2C touch controller (5-point multi-touch)
- **MCU**: ESP32-S3 dual-core LX7 @ 240MHz
- **Memory**: 16MB Flash + 8MB PSRAM
- **Interface**: USB-C for programming and power

## Features

- **Web Configuration Portal**: Device hosts a WiFi AP (`BroncoControls/bronco123`) and modern web app (AsyncWebServer + LVGL) for editing pages, WiFi, and CAN mappings without reflashing.
- **WiFi Scanner + Status Badges**: Scan for nearby SSIDs directly from the portal, auto-fill the join form, and read both AP and DHCP-assigned LAN IPs right on the touchscreen header.
- **Dynamic Screen Builder**: JSON-driven `UIBuilder` renders any number of custom pages (up to 20) with up to 12 programmable buttons each.
- **CAN/J1939 Output Ready**: `CanManager` uses the ESP32-S3 TWAI peripheral to send 29-bit frames derived from web-configured PGNs, priority, and payload bytes.
- **Design System**: Reusable LVGL theme (`UITheme`) keeps typography, spacing, and motion consistent across generated layouts.
- **Touch-Optimized**: Large targets, press feedback, and LVGL animations tuned for in-vehicle interaction.
- **Performance**: Double-buffered LVGL draw buffers in PSRAM + dedicated FreeRTOS task for responsive UI.

## Project Structure

```
Bronco-Controls-4/
├── platformio.ini            # PlatformIO configuration + library deps
├── lib/                      # Vendor display + LVGL config
│   ├── ESP_Panel_Conf.h
│   ├── lv_conf.h
│   ├── ESP32_Display_Panel/
│   └── ESP32_IO_Expander/
├── src/
│   ├── main.cpp              # Hardware bring-up + system bootstrap
│   ├── app_state.h/.cpp      # Legacy helpers (optional for future vehicle data)
│   ├── can_manager.h/.cpp    # TWAI helper for J1939 frames
│   ├── config_types.h        # JSON schema structs
│   ├── config_manager.h/.cpp # LittleFS-backed configuration store
│   ├── ui_builder.h/.cpp     # Dynamic LVGL layouts (pages/buttons)
│   ├── ui_theme.h/.cpp       # Design system helpers
│   ├── web_server.h/.cpp     # AsyncWebServer + REST endpoints
│   ├── web_interface.h       # Embedded HTML/CSS/JS SPA
│   └── assets/               # LVGL image descriptors
│       └── images.h
└── README.md                 # This file
```

## Build Instructions

### Prerequisites

1. **VS Code** with **PlatformIO** extension installed
2. **USB drivers** for ESP32-S3 (usually auto-installed)
3. **Waveshare ESP32-S3-Touch-LCD** board (4.3" or 7.0" variants)

### Clone and Setup

```bash
# Navigate to project directory
cd Bronco-Controls-4

# The project is ready to build - PlatformIO will auto-download dependencies
```

### Build

```bash
# In VS Code:
# - Open project folder in VS Code
# - PlatformIO should detect platformio.ini automatically
# - Click "Build" in PlatformIO toolbar (checkmark icon)

# Or use PlatformIO CLI:
pio run
```

#### Select panel variant

- Default environment (4.3"): `pio run`
- 7.0" panel: `pio run -e waveshare_7in`

### Flash

1. **Connect the board** via USB-C cable
2. **Enter bootloader mode** (IMPORTANT for first flash):
   - Press and hold the **BOOT** button
   - Press the **RESET** button (while holding BOOT)
   - Release the **RESET** button
   - Release the **BOOT** button
3. **Flash the firmware**:

```bash
# In VS Code: Click "Upload" in PlatformIO toolbar (arrow icon)
# Or use CLI:
pio run --target upload
```

4. **Monitor serial output** (optional):

```bash
# In VS Code: Click "Serial Monitor" in PlatformIO toolbar
# Or use CLI:
pio device monitor --baud 115200
```

### Subsequent Flashes

After the first flash, you usually don't need to enter bootloader mode manually. Just click Upload.

## Usage

### Access the Web Configurator

1. **Power the board** after flashing the firmware; wait ~5 seconds for WiFi to start.
2. **Connect to WiFi**: `BroncoControls` / `bronco123` (default AP credentials stored in `config.json`).
3. **Browse to** `http://192.168.4.250` from any phone/laptop. You should see the single-page app served from the ESP32-S3.
4. (Optional) Use the **Join Home WiFi** panel in the portal to push STA credentials so the unit also connects to your house network while keeping its access point online.

### Build Your UI

- **Pages**: Up to 20 pages, each with a configurable grid (1–4 rows & columns).
- **Buttons**: Up to 12 per page. Set label, grid position, span, accent color, and momentary behavior directly in the browser.
- **CAN Frames**: Toggle CAN on/off per button, then fill in PGN, priority, source/destination addresses, and all 8 data bytes. The firmware computes the 29-bit identifier and transmits over TWAI when the button is pressed.
- **WiFi**: Switch between AP-only or AP+Station mode, update SSIDs/passwords, use the "Nearby Networks" scanner to auto-fill SSIDs, trigger the "Join Home WiFi" workflow, and monitor IP/uptime/heaps on both the web portal and on-screen badges.
- **Export/Import**: Use the buttons at the top of the page to download a JSON backup or import a saved configuration. Saving pushes the JSON to LittleFS and live-refreshes LVGL.

### Advanced Tweaks

- **Default Config**: `src/config_manager.cpp::buildDefaultConfig()` seeds the first boot experience. Edit it to change the starting layout, WiFi credentials, or button palette.
- **Manual Editing**: Mount LittleFS (`pio run --target uploadfs`) and edit `/config.json` if you prefer to version-control a base layout.
- **Theme Adjustments**: Update `src/ui_theme.cpp` for global colors/typography; the dynamic builder consumes these helpers for every generated widget.
- **Touch Axis/Timing**: Still controlled via `lib/ESP_Panel_Conf.h` if your hardware variant needs flipped axes or slower RGB clocks.

## Design System

### Colors

- **Background**: `#1A1A1A` (Dark grey)
- **Surface**: `#2A2A2A` (Medium grey)
- **Accent**: `#FFA500` (Amber/Orange)
- **Text Primary**: `#FFFFFF` (White)
- **Text Secondary**: `#AAAAAA` (Light grey)
- **Success**: `#00FF00` (Green)
- **Error**: `#FF0000` (Red)

### Typography

- **Title**: Montserrat 32px Bold
- **Heading**: Montserrat 24px Semibold
- **Body**: Montserrat 16px Regular
- **Caption**: Montserrat 14px Regular

### Spacing

- **XS**: 4px
- **SM**: 8px
- **MD**: 16px
- **LG**: 24px
- **XL**: 32px

### Components

- **TileButton**: 160×120px, rounded corners, icon + label
- **Toggle**: 60×30px switch, animated
- **Slider**: Full-width, 8px track, 20px handle
- **Card**: Padded container with border
- **TopBar**: Fixed 60px height, title + back button

## Troubleshooting

### Board Not Recognized

- Install CP210x USB drivers (Windows)
- Try different USB cable (data cable, not charging-only)
- Check Device Manager for COM port

### Upload Failed

- Enter bootloader mode manually (BOOT + RESET sequence)
- Close Serial Monitor if open
- Try lower upload speed in `platformio.ini`: `upload_speed = 460800`

### Touch Not Working

- Check I2C pins in `ESP_Panel_Conf.h` (SCL=9, SDA=8)
- Verify GT911 touch controller is detected in serial output
- Try adjusting touch transformation flags

### Display Artifacts

- Reduce RGB clock speed in `ESP_Panel_Conf.h`: `ESP_PANEL_LCD_RGB_CLK_HZ (14 * 1000 * 1000)`
- Enable double buffering (already enabled in `lv_conf.h`)
- Ensure PSRAM is working (check serial output)

### Memory Issues

- PSRAM is mandatory for this project
- Verify `BOARD_HAS_PSRAM` is defined in `platformio.ini`
- Check PSRAM init in serial monitor

## Future Enhancements

- **CAN/J1939 Integration**: AppState architecture is ready for vehicle data
- **Gauges Screen**: Speedometer, tachometer, fuel, temperature
- **Settings Screen**: Brightness, units, calibration
- **Persistent Storage**: Save user preferences to NVS flash
- **Animations**: Screen transitions, value changes
- **Sensors**: Integrate real vehicle sensors

## License

This project is provided as-is for educational and development purposes.

## Acknowledgments

- **Waveshare**: Hardware and base driver libraries
- **LVGL**: Graphics library (v8.3.x)
- **PlatformIO**: Build system
- **istvank**: Reference repository for initial Waveshare setup

## Support

For issues or questions:
1. Check this README's Troubleshooting section
2. Review PlatformIO serial monitor output
3. Consult Waveshare wiki: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3
4. LVGL documentation: https://docs.lvgl.io/8.3/

---

**Built with ⚡ for the Ford Bronco community**
