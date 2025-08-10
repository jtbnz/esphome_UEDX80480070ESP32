# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is an ESPHome custom component project for the VIEWE 7-inch ESP32-S3 touch display. It provides full RGB565 color support (solving the 16-color limitation) using ESP-IDF's RGB LCD peripheral and integrates LVGL with GT911 capacitive touch.

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

### Component Structure
The custom `viewe_display` component implements:
- **ESP32-S3 RGB LCD Driver** → Direct parallel RGB interface using `esp_lcd_new_rgb_panel()`
- **LVGL Integration** → Full graphics library with widget support and touch input
- **GT911 Touch Controller** → I2C capacitive touch with coordinate mapping
- **ESPHome Display Platform** → Standard ESPHome display integration

### Key Technical Implementation
- **RGB Interface**: 16 parallel data pins (RGB565) + 4 control signals (HSYNC, VSYNC, DE, PCLK)
- **PSRAM Utilization**: Double-buffered framebuffers in external PSRAM
- **Touch I2C**: Direct GT911 register access for touch coordinate reading
- **LVGL Threading**: FreeRTOS task integration with mutex protection

### Component Files
- `components/viewe_display/__init__.py`: ESPHome platform definition and code generation
- `components/viewe_display/viewe_display.h`: Class definition with ESP-IDF RGB panel interface
- `components/viewe_display/viewe_display.cpp`: Complete RGB LCD + LVGL + Touch implementation
- `include/lv_conf.h`: LVGL 8.4.0 configuration optimized for ESP32-S3
- `your_device.yaml`: Clean ESPHome configuration using the custom display platform

### Critical Requirements
- **ESP-IDF Framework**: Required for `esp_lcd_rgb_panel` support (Arduino framework cannot drive RGB displays)
- **PSRAM Enabled**: Essential for framebuffer allocation in external memory
- **Proper Pin Mapping**: Must match VIEWE hardware exactly (see component for pin definitions)
- **I2C Bus**: GT911 touch controller requires dedicated I2C initialization

### Display Specifications
- Resolution: 800x480 pixels
- Color Depth: 16-bit RGB565 (65,536 colors)
- Interface: Parallel RGB (not SPI)
- Touch: GT911 capacitive with interrupt support
- Backlight: GPIO2 controlled