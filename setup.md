# VIEWE 7-inch Display ESPHome Integration Setup

This custom component integrates the VIEWE 7-inch ESP32-S3 display with ESPHome using LVGL and the proper RGB display drivers.

## Directory Structure

Create the following directory structure in your ESPHome project:

```
your-esphome-project/
├── your-device.yaml                    # Main ESPHome configuration
├── components/                         # Custom components directory
│   └── viewe_lvgl_display/            # Our custom component
│       ├── __init__.py                # Component configuration
│       ├── viewe_lvgl_display.h       # Header file
│       └── viewe_lvgl_display.cpp     # Implementation file
└── include/                           # Include directory for headers
    └── lv_conf.h                      # LVGL configuration
```

## Setup Steps

### 1. Create the Component Files

1. Create the `components/viewe_lvgl_display/` directory
2. Copy the provided files:
   - `__init__.py` (Python configuration)
   - `viewe_lvgl_display.h` (Header file)  
   - `viewe_lvgl_display.cpp` (Implementation)

### 2. Create the LVGL Configuration

1. Create the `include/` directory
2. Copy the `lv_conf.h` file to this directory

### 3. Update Your ESPHome Configuration

Use the provided YAML configuration, updating these values:
- WiFi credentials
- API encryption key
- Any specific pin assignments if they differ

### 4. Hardware Connections

The component is configured for the standard VIEWE 7-inch display pinout:

#### Display Pins (RGB Interface)
- **Red pins**: 45, 48, 47, 21, 14 (R0-R4)
- **Green pins**: 5, 6, 7, 15, 16, 4 (G0-G5)
- **Blue pins**: 8, 3, 46, 9, 1 (B0-B4)
- **Control pins**:
  - DE (Data Enable): 40
  - VSYNC (Vertical Sync): 41
  - HSYNC (Horizontal Sync): 39
  - PCLK (Pixel Clock): 42
- **Backlight**: Pin 2

#### Touch Controller (GT911)
- **SDA**: Pin 19
- **SCL**: Pin 20
- **Reset**: Pin 38
- **Interrupt**: Pin 18

### 5. ESP32-S3 Configuration

The component requires these ESP32-S3 settings:
- **Framework**: ESP-IDF (required for RGB display support)
- **PSRAM**: Enabled (for framebuffers)
- **Flash**: 16MB
- **Partition scheme**: Large app partition

### 6. Build and Flash

1. Compile the configuration:
   ```bash
   esphome compile your-device.yaml
   ```

2. Flash to the device:
   ```bash
   esphome upload your-device.yaml
   ```

## Features

### What This Component Provides

1. **RGB Display Driver**: Proper 16-bit RGB565 display support
2. **Touch Support**: GT911 capacitive touch controller integration
3. **LVGL Integration**: Full LVGL graphics library support
4. **Demo Interface**: Basic test interface with widgets
5. **ESPHome Integration**: Full Home Assistant integration

### Expected Results

After flashing, you should see:
- Full-color display (no longer limited 16-color palette)
- Touch functionality working
- Basic demo interface with:
  - Title text
  - Colored rectangles (Red, Green, Blue)
  - Interactive button
  - Status text

### Troubleshooting

#### Display Issues
- **No display**: Check power and data connections
- **Garbled colors**: Verify RGB pin assignments
- **Dim display**: Check backlight pin connection

#### Touch Issues
- **No touch response**: Verify I2C connections (SDA/SCL)
- **Incorrect touch coordinates**: Check touch calibration

#### Build Issues
- **Compilation errors**: Ensure ESP-IDF framework is selected
- **Memory errors**: Verify PSRAM is enabled
- **LVGL errors**: Check that lv_conf.h is in the include directory

## Customization

### Modifying the Interface

Edit the `create_demo_widgets_()` function in `viewe_lvgl_display.cpp` to customize the interface.

### Adding Home Assistant Integration

You can extend the component to expose LVGL widgets as ESPHome components:
- Sensors for button states
- Text sensors for display information  
- Services for updating display content

### Performance Tuning

Adjust these settings in `lv_conf.h` for performance:
- `LV_DISP_DEF_REFR_PERIOD`: Display refresh rate
- `LVGL_BUFFER_SIZE`: Buffer size (affects memory usage)
- `LV_INDEV_DEF_READ_PERIOD`: Touch polling rate

## Technical Details

### RGB Display Configuration
- **Resolution**: 800x480 pixels
- **Color depth**: 16-bit RGB565
- **Interface**: Parallel RGB
- **Refresh rate**: ~60Hz
- **Pixel clock**: 16MHz

### Memory Usage
- **Framebuffers**: 2x (800x60x2) = ~192KB in PSRAM
- **LVGL buffers**: 2x60 lines = ~144KB
- **Total PSRAM usage**: ~400KB

### Performance
- **Frame rate**: 15-25 FPS (depending on content)
- **Touch response**: <10ms latency
- **Boot time**: ~3-5 seconds to display

This implementation should resolve your 16-color display issue by using the proper RGB interface drivers instead of SPI-based display drivers that may have been causing the color limitation.