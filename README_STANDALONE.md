# VIEWE Display Standalone Driver for Flutter

This is a standalone C driver for the VIEWE UEDX80480070E-WB-A 7-inch 800x480 LCD display with GT911 touch controller, designed to work with Flutter applications via serial or WiFi communication.

## Overview

This project refactors the ESPHome-based LCD driver into a standalone driver that can be used with Flutter applications. The driver communicates with Flutter apps through a simple serial protocol, allowing Flutter UI to be rendered on the physical display.

## Features

- **Standalone LCD Driver**: Pure C implementation using ESP-IDF
- **GT911 Touch Support**: Multi-touch capacitive touch controller
- **Flutter Bridge**: Serial communication protocol for Flutter integration
- **Hardware Accelerated**: Uses ESP32-S3 RGB LCD peripheral with DMA
- **Display Features**:
  - 800x480 resolution
  - RGB565 color (65,536 colors)
  - Display rotation (0°, 90°, 180°, 270°)
  - Backlight control
  - PSRAM framebuffer
- **Drawing Primitives**: Pixels, rectangles, fills
- **Touch Events**: Real-time touch data reporting to Flutter

## Hardware Requirements

- **Display Board**: VIEWE UEDX80480070E-WB-A
- **MCU**: ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM)
- **Display**: 7-inch IPS, 800x480 pixels
- **Touch**: GT911 capacitive touch (I2C)
- **Backlight**: GPIO2 (configurable)

## Pin Configuration

All pins are hardcoded to match VIEWE hardware:

### Display Interface
- **VSYNC**: GPIO41
- **HSYNC**: GPIO39
- **DE**: GPIO40
- **PCLK**: GPIO42
- **RGB Data**: GPIO8,3,46,9,1,5,6,7,15,16,4,45,48,47,21,14
- **Backlight**: GPIO2

### Touch Controller (I2C)
- **SDA**: GPIO19
- **SCL**: GPIO20
- **INT**: GPIO18
- **RST**: GPIO38

## Project Structure

```
├── platformio.ini              # PlatformIO configuration
├── sdkconfig.defaults          # ESP-IDF configuration
├── include/                    # Public headers
│   ├── viewe_lcd_driver.h      # LCD driver API
│   ├── gt911_touch.h           # Touch controller API
│   └── flutter_bridge.h        # Flutter communication API
├── lib/                        # Library implementations
│   ├── VieweDisplay/
│   │   └── viewe_lcd_driver.c
│   ├── GT911Touch/
│   │   └── gt911_touch.c
│   └── FlutterBridge/
│       └── flutter_bridge.c
└── src/
    └── main.c                  # Application entry point
```

## Building the Firmware

### Prerequisites

1. Install [PlatformIO](https://platformio.org/)
2. Install ESP-IDF 5.1 or later

### Build Steps

```bash
# Clone the repository
git clone https://github.com/jtbnz/esphome_UEDX80480070ESP32.git
cd esphome_UEDX80480070ESP32

# Checkout the standalone driver branch
git checkout flutter-standalone-driver

# Build the firmware
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

## Communication Protocol

The driver uses a simple packet-based protocol over UART (115200 baud, 8N1).

### Packet Format

```
[Header] [Command] [Length_L] [Length_H] [Data...] [Footer]
  0xAA     1 byte    1 byte     1 byte    N bytes    0x55
```

### Commands

| Command | Code | Description | Data Format |
|---------|------|-------------|-------------|
| PING | 0x01 | Check connection | None |
| GET_DISPLAY_INFO | 0x02 | Get display dimensions | None |
| SET_PIXEL | 0x03 | Set single pixel | x(2), y(2), r, g, b |
| FILL | 0x04 | Fill entire screen | r, g, b |
| FILL_RECT | 0x05 | Fill rectangle | x(2), y(2), w(2), h(2), r, g, b |
| DRAW_RECT | 0x06 | Draw rectangle outline | x(2), y(2), w(2), h(2), r, g, b |
| FLUSH | 0x07 | Update display | None |
| SET_BRIGHTNESS | 0x08 | Set backlight | brightness(1) |
| SET_ROTATION | 0x09 | Set rotation | rotation(1) |
| GET_TOUCH | 0x0A | Read touch data | None |

### Responses

| Response | Code | Description | Data Format |
|----------|------|-------------|-------------|
| OK | 0x00 | Success | None |
| ERROR | 0xFF | Error | None |
| DISPLAY_INFO | 0x02 | Display info | w(2), h(2), bpp(1), reserved(1) |
| TOUCH_DATA | 0x0A | Touch points | count(1), [x(2),y(2),size(2),id(1)]... |

## API Reference

### LCD Driver (`viewe_lcd_driver.h`)

```c
// Initialize LCD
viewe_lcd_driver_t* viewe_lcd_init(const viewe_lcd_config_t* config);

// Set pixel
esp_err_t viewe_lcd_set_pixel(viewe_lcd_driver_t* driver, int x, int y, viewe_color_t color);

// Fill screen
esp_err_t viewe_lcd_fill(viewe_lcd_driver_t* driver, viewe_color_t color);

// Fill rectangle
esp_err_t viewe_lcd_fill_rect(viewe_lcd_driver_t* driver, int x, int y, int width, int height, viewe_color_t color);

// Draw rectangle outline
esp_err_t viewe_lcd_draw_rect(viewe_lcd_driver_t* driver, int x, int y, int width, int height, viewe_color_t color);

// Update display
esp_err_t viewe_lcd_flush(viewe_lcd_driver_t* driver);

// Set brightness (0-100)
esp_err_t viewe_lcd_set_brightness(viewe_lcd_driver_t* driver, uint8_t brightness);

// Set rotation
esp_err_t viewe_lcd_set_rotation(viewe_lcd_driver_t* driver, viewe_lcd_rotation_t rotation);
```

### Touch Controller (`gt911_touch.h`)

```c
// Initialize touch controller
gt911_driver_t* gt911_init(const gt911_config_t* config);

// Read touch data
esp_err_t gt911_read_touch(gt911_driver_t* driver, gt911_touch_data_t* data);

// Register touch callback
esp_err_t gt911_register_callback(gt911_driver_t* driver, gt911_callback_t callback, void* user_data);
```

### Flutter Bridge (`flutter_bridge.h`)

```c
// Initialize bridge
flutter_bridge_t* flutter_bridge_init(const flutter_bridge_config_t* config);

// Process commands (call in main loop)
esp_err_t flutter_bridge_process(flutter_bridge_t* bridge);

// Send touch event
esp_err_t flutter_bridge_send_touch_event(flutter_bridge_t* bridge, gt911_touch_data_t* touch_data);
```

## Flutter Integration

### Example Flutter Package

A Dart package for communicating with the display will be needed. Here's a basic structure:

```dart
class VieweDisplay {
  final SerialPort _port;
  
  VieweDisplay(String portName) : _port = SerialPort(portName);
  
  Future<void> connect() async {
    await _port.open(baudrate: 115200);
  }
  
  Future<void> setPixel(int x, int y, Color color) async {
    final packet = _buildPacket(0x03, [
      x & 0xFF, (x >> 8) & 0xFF,
      y & 0xFF, (y >> 8) & 0xFF,
      color.red, color.green, color.blue,
    ]);
    await _port.write(packet);
  }
  
  Future<void> flush() async {
    final packet = _buildPacket(0x07, []);
    await _port.write(packet);
  }
  
  Stream<TouchEvent> get touchEvents => _touchStream;
}
```

## Example Usage

### C Code (ESP32)

```c
#include "viewe_lcd_driver.h"
#include "gt911_touch.h"
#include "flutter_bridge.h"

void app_main(void) {
    // Initialize LCD
    viewe_lcd_driver_t* lcd = viewe_lcd_init(NULL);
    
    // Initialize touch
    gt911_driver_t* touch = gt911_init(NULL);
    
    // Initialize bridge
    flutter_bridge_config_t config = {
        .lcd_driver = lcd,
        .touch_driver = touch,
        .enable_serial = true,
    };
    flutter_bridge_t* bridge = flutter_bridge_init(&config);
    
    // Main loop
    while (1) {
        flutter_bridge_process(bridge);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

### Flutter Code (Dart)

```dart
import 'package:viewe_display/viewe_display.dart';

void main() async {
  final display = VieweDisplay('/dev/ttyUSB0');
  await display.connect();
  
  // Fill screen with blue
  await display.fill(Colors.blue);
  await display.flush();
  
  // Draw a red rectangle
  await display.fillRect(50, 50, 200, 100, Colors.red);
  await display.flush();
  
  // Listen for touch events
  display.touchEvents.listen((event) {
    print('Touch at (${event.x}, ${event.y})');
  });
}
```

## Testing

The firmware includes a demo that:
1. Displays colored screens on startup
2. Draws test rectangles
3. Waits for Flutter commands
4. Reports touch events

## Troubleshooting

1. **Display not working**
   - Check power supply (5V/1A minimum)
   - Verify ESP-IDF 5.1+ is installed
   - Check PSRAM is enabled in sdkconfig

2. **Touch not responding**
   - Verify I2C connections
   - Check I2C address (0x5D or 0x14)
   - Try resetting the touch controller

3. **Serial communication issues**
   - Verify baud rate is 115200
   - Check USB cable quality
   - Use proper serial adapter

## Performance

- **Frame Rate**: ~31 FPS (hardware limited)
- **Touch Response**: < 10ms
- **Serial Throughput**: ~11KB/s at 115200 baud
- **Memory Usage**: 768KB PSRAM for framebuffer

## Future Enhancements

- [ ] WiFi communication support
- [ ] Bitmap/image transfer
- [ ] PWM brightness control
- [ ] Compressed data transfer
- [ ] Touch gesture recognition
- [ ] Flutter widget library

## License

This driver is provided as-is for use with VIEWE display hardware.

## Credits

Based on the original ESPHome component by @jtbnz
Refactored for standalone Flutter integration

## Support

For issues and questions:
- GitHub Issues: https://github.com/jtbnz/esphome_UEDX80480070ESP32/issues
- ESPHome Discord: https://discord.gg/KhAMKrd
