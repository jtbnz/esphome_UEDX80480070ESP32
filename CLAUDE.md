# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is an ESPHome custom component project for integrating the VIEWE 7-inch ESP32-S3 touch display. It implements RGB display drivers with LVGL (Light and Versatile Graphics Library) support and GT911 capacitive touch controller integration.

## Key Commands

### Building and Flashing
```bash
# Compile the ESPHome configuration
esphome compile your_device.yaml

# Upload to the device
esphome upload your_device.yaml

# Combined compile and upload
esphome run your_device.yaml

# View logs from the device
esphome logs your_device.yaml
```

### Development Commands
```bash
# Validate configuration without building
esphome config your_device.yaml

# Clean build files
esphome clean your_device.yaml

# Generate C++ code without compiling (useful for debugging)
esphome compile your_device.yaml --only-generate
```

## Architecture

### Component Structure
The project implements a custom ESPHome component (`viewe_lvgl_display`) that bridges:
- **ESP32-S3 RGB LCD peripheral** → 16-bit parallel RGB interface for the 800x480 display
- **LVGL graphics library** → Provides UI rendering and widget management
- **GT911 touch controller** → I2C-based capacitive touch input via custom driver
- **ESPHome framework** → Integration with Home Assistant and ESPHome ecosystem

### Key Technical Details
- **Display Interface**: Parallel RGB565 (not SPI) - requires ESP-IDF framework for proper RGB LCD support
- **Memory Architecture**: Uses PSRAM for dual framebuffers (~400KB total)
- **Touch Pipeline**: GT911 → I2C → LVGL input device → Widget event handling
- **Pin Mapping**: Complex RGB pin configuration (5 red, 6 green, 5 blue pins) plus control signals

### Component Files
- `components/viewe_lvgl_display/__init__.py`: ESPHome component configuration and code generation
- `components/viewe_lvgl_display/viewe_lvgl_display.cpp`: Main implementation with RGB driver and LVGL integration
- `components/viewe_lvgl_display/viewe_lvgl_display.h`: Component header with class definition
- `include/lv_conf.h`: LVGL configuration (buffer sizes, color depth, features)
- `your_device.yaml`: ESPHome device configuration with pin mappings and component setup

### Critical Configuration Requirements
- Must use ESP-IDF framework (not Arduino) for RGB LCD support
- Requires PSRAM enabled for framebuffers
- Needs large app partition (16MB flash recommended)
- Color depth fixed at 16-bit RGB565

### Touch Configuration Options
Two approaches for touch support:
1. **Integrated touch** (in custom component): Basic initialization only, no event handling
2. **Standard GT911 component** (recommended): Full touch events, zones, multi-touch support
   - See `your_device_with_standard_touch.yaml` for standard component example
   - Provides `on_touch`, `on_release` callbacks and binary sensor zones