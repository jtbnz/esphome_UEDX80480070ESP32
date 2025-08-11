# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is an ESPHome custom display platform project for the VIEWE 7-inch ESP32-S3 touch display. It provides full RGB565 color support (solving the 16-color limitation) using ESP-IDF's RGB LCD peripheral with standard ESPHome display and touchscreen components.

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
```

## Architecture

### Custom Display Platform Structure
The `viewe_rgb` display platform provides:
- **ESPHome Display Integration** → Standard `display:` platform following ESPHome patterns
- **RGB565 LCD Driver** → ESP-IDF `esp_lcd_new_rgb_panel()` for true 16-bit color
- **Standard Touchscreen Support** → Works with ESPHome's GT911 touchscreen component
- **Proper Component Separation** → Display and touch are independent, following ESPHome conventions

### Key Technical Implementation
- **RGB Interface**: 16 parallel data pins (RGB565) + 4 control signals (DE, PCLK, HSYNC, VSYNC)
- **PSRAM Framebuffer**: Double-buffered 800x480x2 framebuffer in external PSRAM
- **ESPHome Display API**: Full compatibility with ESPHome's drawing primitives and lambda functions
- **Separated Touch**: Uses standard ESPHome GT911 touchscreen component via I2C

### Component Files
- `components/viewe_rgb/__init__.py`: ESPHome display platform configuration and code generation
- `components/viewe_rgb/viewe_rgb.h`: Display class inheriting from ESPHome Display base class
- `components/viewe_rgb/viewe_rgb.cpp`: ESP-IDF RGB LCD implementation with ESPHome integration
- `your_device.yaml`: ESPHome configuration using both custom display and standard touchscreen

### Configuration Structure
```yaml
display:
  - platform: viewe_rgb              # Custom RGB display platform
    width: 800
    height: 480
    data_pins: [8,3,46,9,1,5,6,7,15,16,4,45,48,47,21,14]  # 16 RGB data pins
    de_pin: 40
    pclk_pin: 42
    hsync_pin: 39
    vsync_pin: 41
    backlight_pin: 2
    lambda: |-
      // Standard ESPHome display lambda
      
touchscreen:
  - platform: gt911                  # Standard ESPHome touchscreen
    interrupt_pin: 18
    reset_pin: 38
```

### Critical Requirements
- **ESP-IDF Framework**: Required for `esp_lcd_rgb_panel` support
- **PSRAM Enabled**: Essential for large framebuffer allocation
- **Exact Pin Mapping**: Must match VIEWE hardware RGB565 pin configuration
- **I2C Bus**: Separate I2C configuration for GT911 touchscreen

### Display Specifications
- Resolution: 800x480 pixels
- Color Depth: 16-bit RGB565 (65,536 colors) 
- Interface: Parallel RGB (16 data pins + 4 control pins)
- Touch: GT911 capacitive via I2C (pins 19/20)
- Backlight: PWM controlled via GPIO2

### Test Card Features
The configuration includes a comprehensive test card that proves RGB565 operation:
- RGB gradient bars showing 32/64/32 bit levels
- 18 intermediate colors impossible on 16-color displays
- Touch coordinate logging
- Professional layout proving full color depth