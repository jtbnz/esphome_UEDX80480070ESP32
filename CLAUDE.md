# CLAUDE.md - VIEWE 7-inch RGB565 Display Project

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 🎯 PROJECT STATUS (Current Session)

### GOAL
Create a custom ESPHome component for the VIEWE 7-inch ESP32-S3 touch display to achieve full RGB565 (65,536 colors) instead of the limited 16-color mode from the standard `rpi_dpi_rgb` platform.

### HARDWARE
- **Display**: VIEWE 7-inch 800x480 capacitive touch display
- **MCU**: ESP32-S3 with 8MB flash, 8MB PSRAM
- **Touch**: GT911 capacitive touch controller (I2C)
- **Interface**: 16-bit parallel RGB565 (5+6+5 pins)
- **Reference**: https://github.com/VIEWESMART/UEDX80480070ESP32-7inch-Touch-Display

## 🔧 CURRENT STATUS

### ✅ COMPLETED
1. **Custom ESPHome Component Structure**: Created proper display platform
   - `components/viewe_display/__init__.py`
   - `components/viewe_display/display.py` 
   - `components/viewe_display/viewe_display.h`
   - `components/viewe_display/viewe_display.cpp`

2. **Pin Mapping**: Correctly mapped RGB pins to match working config
   - **Red pins**: [14, 21, 47, 48, 45] (5 pins)
   - **Green pins**: [4, 16, 15, 7, 6, 5] (6 pins)
   - **Blue pins**: [1, 9, 46, 3, 8] (5 pins)
   - **Control**: DE=40, PCLK=42, HSYNC=39, VSYNC=41, Backlight=2

3. **ESP-IDF RGB LCD Integration**: Using `esp_lcd_new_rgb_panel()` API
4. **Compilation**: Successfully compiles without errors
5. **Hardware Validation**: Display receives data (white screen visible)

### 🚨 CURRENT ISSUE: Sync Timing
**Symptom**: White background with black horizontal pixels and occasional vertical stripes  
**Cause**: ESP-IDF RGB LCD timing parameters don't directly match RPI DPI requirements  
**Progress**: Latest timing adjustments show improvement - fewer artifacts

## 📋 TESTED CONFIGURATIONS

| Config | Pixel Clock | PCLK Inv | HSYNC (W/B/F) | VSYNC (W/B/F) | Polarity | Result |
|--------|-------------|----------|---------------|---------------|----------|---------|
| Original | 16MHz | true | 48/40/40 | 1/31/13 | Default | White + black flashing |
| All Inverted | 16MHz | true | 48/40/40 | 1/31/13 | All inverted | Blank screen |
| Mixed | 16MHz | true | 48/40/40 | 1/31/13 | H-inv only | White + flashing |
| Conservative | 14MHz | false | 4/8/8 | 4/8/8 | All default | White + black pixels |
| **CURRENT** | **15MHz** | **false** | **10/20/20** | **2/15/15** | **All default** | **IMPROVED: Less artifacts** |

## 📁 KEY FILES

### Main Configuration (`your_device.yaml`) - CURRENT STATE
```yaml
display:
  - platform: viewe_display
    id: main_display
    width: 800
    height: 480
    # RGB pins matching working config
    red_pins: [14, 21, 47, 48, 45]
    green_pins: [4, 16, 15, 7, 6, 5]  
    blue_pins: [1, 9, 46, 3, 8]
    # Control pins
    de_pin: 40
    pclk_pin: 42
    hsync_pin: 39
    vsync_pin: 41
    backlight_pin: 2
    # CURRENT TIMING - Shows improvement
    pixel_clock_frequency: 15MHz
    pclk_inverted: false
    hsync_idle_low: false
    vsync_idle_low: false
    de_idle_high: false
    hsync_pulse_width: 10
    hsync_back_porch: 20
    hsync_front_porch: 20
    vsync_pulse_width: 2
    vsync_back_porch: 15
    vsync_front_porch: 15
    update_interval: 3s
    lambda: |-
      // Color cycling test - RED/GREEN/BLUE/YELLOW/MAGENTA/CYAN/WHITE/BLACK
      static int color_phase = 0;
      switch (color_phase % 8) {
        case 0: it.fill(Color(255, 0, 0)); break;    // Red
        case 1: it.fill(Color(0, 255, 0)); break;    // Green  
        case 2: it.fill(Color(0, 0, 255)); break;    // Blue
        case 3: it.fill(Color(255, 255, 0)); break;  // Yellow
        case 4: it.fill(Color(255, 0, 255)); break;  // Magenta
        case 5: it.fill(Color(0, 255, 255)); break;  // Cyan
        case 6: it.fill(Color(255, 255, 255)); break; // White
        case 7: it.fill(Color(0, 0, 0)); break;      // Black
      }
      color_phase++;
```

### Component Architecture
- **`components/viewe_display/__init__.py`**: Component registration  
- **`components/viewe_display/display.py`**: Display platform with RGB pin arrays and sync polarity controls
- **`components/viewe_display/viewe_display.h`**: C++ class with ESP-IDF RGB LCD integration
- **`components/viewe_display/viewe_display.cpp`**: Implementation using `esp_lcd_new_rgb_panel()`

## 🎯 NEXT SESSION ACTIONS

### 1. TEST CURRENT BUILD FIRST
The latest firmware shows improvement - test it immediately to see current behavior.

### 2. IF STILL ISSUES, TRY THESE TIMING VARIATIONS:
```yaml
# Option A: Higher pixel clock
pixel_clock_frequency: 16MHz

# Option B: Adjust sync pulse widths  
hsync_pulse_width: 8
vsync_pulse_width: 4

# Option C: Fine-tune porches
hsync_back_porch: 25
hsync_front_porch: 25

# Option D: Try slight polarity changes
pclk_inverted: true  # Only if others fail
```

### 3. ALTERNATIVE APPROACH (If timing tuning fails)
Implement direct ESP-IDF approach:
- Copy exact configuration from VIEWE GitHub reference
- Bypass ESPHome display abstraction  
- Use VIEWE's proven LCD initialization sequence

### 4. REFERENCE WORKING CONFIG (16-color limit)
```yaml
display:
  - platform: rpi_dpi_rgb
    data_pins:
      red: [14, 21, 47, 48, 45]
      green: [4, 16, 15, 7, 6, 5]
      blue: [1, 9, 46, 3, 8]
    de_pin: 40
    pclk_pin: 42  
    hsync_pin: 39
    vsync_pin: 41
    hsync_front_porch: 40
    hsync_pulse_width: 48 
    hsync_back_porch: 40
    vsync_front_porch: 13 
    vsync_pulse_width: 1 
    vsync_back_porch: 31
    pclk_inverted: true
    color_order: RGB
```

## 🔍 DIAGNOSTIC GUIDE

### Display Behavior Patterns
- **White + black flashing**: Basic sync working, timing issues
- **Vertical stripes**: Line sync problems (HSYNC timing)  
- **Horizontal artifacts**: Pixel clock or data timing issues
- **Blank screen**: Sync polarity completely wrong
- **Solid colors**: Good sync, ready for RGB565 validation

### Color Cycling Test
The lambda cycles through 8 colors every 3 seconds. This diagnoses:
- **Sync stability**: Colors should be solid, not flickering
- **Pin mapping**: Colors should match what logs report
- **RGB565 functionality**: Should see all colors properly

## ✅ SUCCESS CRITERIA
- [ ] Display shows stable, solid colors without artifacts
- [ ] Color cycling test shows all 8 colors correctly  
- [ ] Touch functionality works with GT911 component
- [ ] Full RGB565 color depth proven (65,536 colors vs 16)

## 🛠 BUILD COMMANDS
```bash
# Compile current configuration
esphome compile your_device.yaml

# Upload to device  
esphome upload your_device.yaml

# View logs
esphome logs your_device.yaml
```

## 🎨 PROJECT GOAL REMINDER
Transform this display from **16-color limitation** → **Full RGB565 (65,536 colors)** while maintaining ESPHome compatibility and standard touchscreen integration.

**Current Status**: Hardware working, sync timing nearly there, very close to success! 🎯