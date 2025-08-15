# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a VIEWE ESP32-S3 Smart Display development project for the UEDX80480070E-WB-A board, featuring:
- ESP32-S3-N16R8 MCU with 16MB Flash and 8MB PSRAM
- 7-inch 800x480 IPS RGB display with GT911 capacitive touch
- Support for Arduino IDE, PlatformIO, and ESP-IDF frameworks
- LVGL graphics library integration

## Common Development Commands

### Arduino IDE Setup
1. Install ESP32 board package v3.1.0+ via Board Manager
2. Install ESP32_Display_Panel library v1.0.3+ and dependencies
3. Install LVGL v8.4.0 for GUI examples
4. Board selection: ESP32S3 Dev Module
5. Partition scheme: 16M Flash (3MB APP/9.9MB FATFS)
6. PSRAM: OPI PSRAM

### PlatformIO Commands
```bash
# Build the default environment (BOARD_VIEWE_UEDX80480070E_WB_A)
pio run

# Upload to device (may need to hold BOOT button on first upload)
pio run -t upload

# Monitor serial output
pio device monitor

# Clean build files
pio run -t clean
```

### ESP-IDF Commands
```bash
# Configure the project
idf.py menuconfig

# Build the project
idf.py build

# Flash to device (may need to hold BOOT button)
idf.py -p PORT flash

# Monitor output
idf.py -p PORT monitor

# Full build, flash and monitor
idf.py -p PORT flash monitor
```

## Board Configuration

### Critical Macros for Board Setup

When working with Arduino examples:
1. Edit `esp_panel_board_supported_conf.h`:
   - Set `ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED` to `1`
   - Uncomment `#define BOARD_VIEWE_UEDX80480070E_WB_A`

2. For LVGL color configuration in `lv_conf.h`:
   - RGB screens: Set `LV_COLOR_16_SWAP` to `0`
   - SPI/QSPI screens: Set `LV_COLOR_16_SWAP` to `1`

When working with PlatformIO:
- Ensure `default_envs = BOARD_VIEWE_UEDX80480070E_WB_A` in platformio.ini

## Hardware Architecture

### Pin Assignments
- **RGB Display**: DE(IO40), VS(IO41), HS(IO39), PCLK(IO42), R[0-4], G[0-5], B[0-4], Backlight(IO2)
- **Touch (I2C)**: RST(IO38), INT(IO18), SDA(IO19), SCL(IO20)
- **SD Card (SPI)**: CS(IO10), CLK(IO12), MOSI(IO11), MISO(IO13)
- **UART/USB**: TX(IO43), RX(IO44), USB-DP(IO20), USB-DN(IO19)
- **Buttons**: BOOT(IO0), RESET(CHIP-EN)
- **RGB LED**: IO0 (WS2812B)

### Display Driver Stack
```
Application (LVGL/Arduino/ESP-IDF)
    ↓
ESP32_Display_Panel Library
    ↓
ESP LCD Driver (RGB Interface)
    ↓
EK9716BD3 + EK73002AB2 Display Controllers
```

### Touch Driver Stack
```
Application
    ↓
ESP32_Display_Panel Touch Interface
    ↓
I2C Driver
    ↓
GT911 Touch Controller
```

## Key Project Structure

```
├── examples/
│   ├── arduino/          # Arduino framework examples
│   │   ├── board/        # Board configuration examples
│   │   ├── drivers/      # LCD and touch driver examples
│   │   └── gui/lvgl_v8/  # LVGL GUI examples
│   ├── esp_idf/          # ESP-IDF framework examples
│   └── PlatformIO/       # PlatformIO examples with platformio.ini
├── Libraries/            # Required Arduino libraries
│   ├── ESP32_Display_Panel/
│   ├── lvgl-release-v8.4/
│   └── ...
├── firmware/            # Pre-built firmware binaries
├── tools/              # Flash tools and utilities
└── Schematic/          # Hardware schematics
```

## Development Tips

1. **First-time programming**: Always hold the BOOT button when uploading for the first time
2. **UART configuration**: Default uses USB CDC. To use external UART pins, disable USB CDC in Arduino Tools menu or PlatformIO build flags
3. **Display initialization**: The ESP32_Display_Panel library handles most initialization automatically when using supported board configurations
4. **Touch calibration**: GT911 touch controller typically doesn't require calibration but supports gesture recognition
5. **Memory management**: With 8MB PSRAM available, enable PSRAM in menuconfig/board settings for memory-intensive applications

## Common Issues and Solutions

- **Upload fails**: Hold BOOT button during upload, ensure correct port selected
- **Display drift/artifacts**: Ensure RGB LCD timing parameters match display specifications
- **Touch not working**: Verify I2C pull-up resistors, check INT pin connection
- **Serial output missing**: Check if USB CDC is enabled/disabled as needed