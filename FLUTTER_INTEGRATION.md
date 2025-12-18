# Flutter Integration Guide

This guide explains how to integrate the VIEWE Display standalone driver with your Flutter application.

## Architecture Overview

```
┌─────────────────┐         Serial/WiFi          ┌──────────────────┐
│                 │◄──────────────────────────────►│                  │
│  Flutter App    │     Command Protocol          │  ESP32-S3        │
│  (Dart/Flutter) │                               │  (C/ESP-IDF)     │
│                 │                               │                  │
└─────────────────┘                               └──────────────────┘
      │                                                    │
      │ Uses Dart Package                                │ Controls
      │ for Communication                                │ Hardware
      ▼                                                    ▼
┌─────────────────┐                               ┌──────────────────┐
│ viewe_display   │                               │ 800x480 LCD      │
│ Dart Package    │                               │ GT911 Touch      │
└─────────────────┘                               └──────────────────┘
```

## Communication Protocol

### Packet Structure

All commands and responses use the following packet format:

```
┌────────┬─────────┬──────────┬──────────┬──────────┬────────┐
│ Header │ Command │ Length_L │ Length_H │   Data   │ Footer │
├────────┼─────────┼──────────┼──────────┼──────────┼────────┤
│  0xAA  │ 1 byte  │  1 byte  │  1 byte  │  N bytes │  0x55  │
└────────┴─────────┴──────────┴──────────┴──────────┴────────┘
```

- **Header**: Fixed value 0xAA
- **Command**: Command code (see below)
- **Length**: 16-bit little-endian data length
- **Data**: Command-specific payload
- **Footer**: Fixed value 0x55

### Command Reference

#### 1. PING (0x01)
Check device connection.

**Request:**
```
AA 01 00 00 55
```

**Response:**
```
AA 00 00 00 55  // RESP_OK
```

#### 2. GET_DISPLAY_INFO (0x02)
Get display dimensions and capabilities.

**Request:**
```
AA 02 00 00 55
```

**Response:**
```
AA 02 06 00 [WIDTH_L] [WIDTH_H] [HEIGHT_L] [HEIGHT_H] [BPP] [RESERVED] 55
```

Example: 800x480, 16bpp
```
AA 02 06 00 20 03 E0 01 10 00 55
```

#### 3. SET_PIXEL (0x03)
Set a single pixel color.

**Request:**
```
AA 03 07 00 [X_L] [X_H] [Y_L] [Y_H] [R] [G] [B] 55
```

Example: Set pixel at (100, 50) to red (255, 0, 0)
```
AA 03 07 00 64 00 32 00 FF 00 00 55
```

#### 4. FILL (0x04)
Fill entire screen with a color.

**Request:**
```
AA 04 03 00 [R] [G] [B] 55
```

Example: Fill screen with blue
```
AA 04 03 00 00 00 FF 55
```

#### 5. FILL_RECT (0x05)
Fill a rectangular area.

**Request:**
```
AA 05 0B 00 [X_L] [X_H] [Y_L] [Y_H] [W_L] [W_H] [H_L] [H_H] [R] [G] [B] 55
```

Example: Fill rect at (50, 50) size 200x100 with red
```
AA 05 0B 00 32 00 32 00 C8 00 64 00 FF 00 00 55
```

#### 6. DRAW_RECT (0x06)
Draw a rectangle outline.

**Request:**
```
AA 06 0B 00 [X_L] [X_H] [Y_L] [Y_H] [W_L] [W_H] [H_L] [H_H] [R] [G] [B] 55
```

#### 7. FLUSH (0x07)
Update the display with framebuffer contents.

**Request:**
```
AA 07 00 00 55
```

**Response:**
```
AA 00 00 00 55  // RESP_OK
```

#### 8. SET_BRIGHTNESS (0x08)
Set backlight brightness.

**Request:**
```
AA 08 01 00 [BRIGHTNESS] 55
```

Example: Set 50% brightness
```
AA 08 01 00 32 55
```

#### 9. SET_ROTATION (0x09)
Set display rotation.

**Request:**
```
AA 09 01 00 [ROTATION] 55
```

Rotation values:
- 0: 0 degrees
- 1: 90 degrees clockwise
- 2: 180 degrees
- 3: 270 degrees clockwise

#### 10. GET_TOUCH (0x0A)
Read current touch state.

**Request:**
```
AA 0A 00 00 55
```

**Response:**
```
AA 0A [LEN_L] [LEN_H] [COUNT] [TOUCH_DATA...] 55
```

Touch data for each point (7 bytes):
- X coordinate (2 bytes, little-endian)
- Y coordinate (2 bytes, little-endian)
- Size (2 bytes, little-endian)
- Track ID (1 byte)

Example: One touch at (400, 240), size 100, id 1
```
AA 0A 08 00 01 90 01 F0 00 64 00 01 55
```

## Dart Package Structure

### Package Layout

```
viewe_display/
├── lib/
│   ├── viewe_display.dart        # Main library export
│   ├── src/
│   │   ├── display_driver.dart   # Core driver class
│   │   ├── protocol.dart         # Protocol implementation
│   │   ├── touch_event.dart      # Touch event types
│   │   └── color.dart            # Color utilities
│   └── viewe_display_platform_interface.dart
├── example/
│   └── main.dart                 # Example application
├── test/
│   └── viewe_display_test.dart
└── pubspec.yaml
```

### Basic Implementation

#### pubspec.yaml
```yaml
name: viewe_display
description: Flutter driver for VIEWE 800x480 LCD display
version: 0.1.0
homepage: https://github.com/jtbnz/esphome_UEDX80480070ESP32

environment:
  sdk: '>=3.0.0 <4.0.0'

dependencies:
  flutter:
    sdk: flutter
  flutter_libserialport: ^0.4.0  # For serial communication

dev_dependencies:
  flutter_test:
    sdk: flutter
  flutter_lints: ^3.0.0
```

#### lib/viewe_display.dart
```dart
library viewe_display;

export 'src/display_driver.dart';
export 'src/touch_event.dart';
export 'src/color.dart';
```

#### lib/src/protocol.dart
```dart
import 'dart:typed_data';

class VieweProtocol {
  static const int header = 0xAA;
  static const int footer = 0x55;
  
  // Commands
  static const int cmdPing = 0x01;
  static const int cmdGetDisplayInfo = 0x02;
  static const int cmdSetPixel = 0x03;
  static const int cmdFill = 0x04;
  static const int cmdFillRect = 0x05;
  static const int cmdDrawRect = 0x06;
  static const int cmdFlush = 0x07;
  static const int cmdSetBrightness = 0x08;
  static const int cmdSetRotation = 0x09;
  static const int cmdGetTouch = 0x0A;
  
  // Responses
  static const int respOk = 0x00;
  static const int respError = 0xFF;
  static const int respDisplayInfo = 0x02;
  static const int respTouchData = 0x0A;
  
  static Uint8List buildPacket(int command, List<int> data) {
    final length = data.length;
    final packet = Uint8List(5 + length);
    
    packet[0] = header;
    packet[1] = command;
    packet[2] = length & 0xFF;
    packet[3] = (length >> 8) & 0xFF;
    
    for (int i = 0; i < length; i++) {
      packet[4 + i] = data[i];
    }
    
    packet[4 + length] = footer;
    
    return packet;
  }
  
  static Map<String, dynamic>? parseResponse(Uint8List data) {
    if (data.length < 5) return null;
    if (data[0] != header || data[data.length - 1] != footer) return null;
    
    final response = data[1];
    final length = data[2] | (data[3] << 8);
    
    if (data.length < 5 + length) return null;
    
    final payload = Uint8List.view(data.buffer, 4, length);
    
    return {
      'response': response,
      'payload': payload,
    };
  }
}
```

#### lib/src/display_driver.dart
```dart
import 'dart:async';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:flutter_libserialport/flutter_libserialport.dart';
import 'protocol.dart';
import 'touch_event.dart';

class VieweDisplay {
  final SerialPort _port;
  final _touchController = StreamController<TouchEvent>.broadcast();
  bool _isConnected = false;
  
  VieweDisplay(String portName) : _port = SerialPort(portName);
  
  Stream<TouchEvent> get touchEvents => _touchController.stream;
  bool get isConnected => _isConnected;
  
  Future<bool> connect() async {
    try {
      _port.config = SerialPortConfig()
        ..baudRate = 115200
        ..bits = 8
        ..stopBits = 1
        ..parity = SerialPortParity.none;
      
      final result = _port.openReadWrite();
      _isConnected = result;
      
      if (_isConnected) {
        _startReading();
      }
      
      return _isConnected;
    } catch (e) {
      print('Failed to connect: $e');
      return false;
    }
  }
  
  void disconnect() {
    _port.close();
    _isConnected = false;
  }
  
  Future<bool> ping() async {
    if (!_isConnected) return false;
    
    final packet = VieweProtocol.buildPacket(VieweProtocol.cmdPing, []);
    _port.write(packet);
    
    // Wait for response (simplified - should have proper async handling)
    await Future.delayed(Duration(milliseconds: 100));
    return true;
  }
  
  Future<void> fill(Color color) async {
    if (!_isConnected) return;
    
    final packet = VieweProtocol.buildPacket(VieweProtocol.cmdFill, [
      color.red,
      color.green,
      color.blue,
    ]);
    
    _port.write(packet);
  }
  
  Future<void> fillRect(int x, int y, int width, int height, Color color) async {
    if (!_isConnected) return;
    
    final packet = VieweProtocol.buildPacket(VieweProtocol.cmdFillRect, [
      x & 0xFF, (x >> 8) & 0xFF,
      y & 0xFF, (y >> 8) & 0xFF,
      width & 0xFF, (width >> 8) & 0xFF,
      height & 0xFF, (height >> 8) & 0xFF,
      color.red,
      color.green,
      color.blue,
    ]);
    
    _port.write(packet);
  }
  
  Future<void> drawRect(int x, int y, int width, int height, Color color) async {
    if (!_isConnected) return;
    
    final packet = VieweProtocol.buildPacket(VieweProtocol.cmdDrawRect, [
      x & 0xFF, (x >> 8) & 0xFF,
      y & 0xFF, (y >> 8) & 0xFF,
      width & 0xFF, (width >> 8) & 0xFF,
      height & 0xFF, (height >> 8) & 0xFF,
      color.red,
      color.green,
      color.blue,
    ]);
    
    _port.write(packet);
  }
  
  Future<void> flush() async {
    if (!_isConnected) return;
    
    final packet = VieweProtocol.buildPacket(VieweProtocol.cmdFlush, []);
    _port.write(packet);
  }
  
  Future<void> setBrightness(int brightness) async {
    if (!_isConnected) return;
    
    final packet = VieweProtocol.buildPacket(
      VieweProtocol.cmdSetBrightness,
      [brightness.clamp(0, 100)],
    );
    
    _port.write(packet);
  }
  
  Future<void> setRotation(int rotation) async {
    if (!_isConnected) return;
    
    final packet = VieweProtocol.buildPacket(
      VieweProtocol.cmdSetRotation,
      [rotation % 4],
    );
    
    _port.write(packet);
  }
  
  void _startReading() {
    final reader = SerialPortReader(_port);
    reader.stream.listen((data) {
      _handleIncomingData(Uint8List.fromList(data));
    });
  }
  
  void _handleIncomingData(Uint8List data) {
    final response = VieweProtocol.parseResponse(data);
    if (response == null) return;
    
    if (response['response'] == VieweProtocol.respTouchData) {
      final payload = response['payload'] as Uint8List;
      if (payload.isEmpty) return;
      
      final count = payload[0];
      for (int i = 0; i < count; i++) {
        final offset = 1 + i * 7;
        if (offset + 7 > payload.length) break;
        
        final x = payload[offset] | (payload[offset + 1] << 8);
        final y = payload[offset + 2] | (payload[offset + 3] << 8);
        final size = payload[offset + 4] | (payload[offset + 5] << 8);
        final trackId = payload[offset + 6];
        
        _touchController.add(TouchEvent(
          x: x,
          y: y,
          size: size,
          trackId: trackId,
        ));
      }
    }
  }
  
  void dispose() {
    disconnect();
    _touchController.close();
  }
}
```

#### lib/src/touch_event.dart
```dart
class TouchEvent {
  final int x;
  final int y;
  final int size;
  final int trackId;
  
  TouchEvent({
    required this.x,
    required this.y,
    required this.size,
    required this.trackId,
  });
  
  @override
  String toString() => 'TouchEvent(x: $x, y: $y, size: $size, id: $trackId)';
}
```

## Example Flutter Application

### example/main.dart

```dart
import 'package:flutter/material.dart';
import 'package:viewe_display/viewe_display.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'VIEWE Display Demo',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
      home: const DisplayControlPage(),
    );
  }
}

class DisplayControlPage extends StatefulWidget {
  const DisplayControlPage({super.key});

  @override
  State<DisplayControlPage> createState() => _DisplayControlPageState();
}

class _DisplayControlPageState extends State<DisplayControlPage> {
  VieweDisplay? _display;
  bool _isConnected = false;
  String _lastTouch = 'No touch';
  
  @override
  void initState() {
    super.initState();
    _connectToDisplay();
  }
  
  Future<void> _connectToDisplay() async {
    // Find the correct serial port for your system
    // On Linux: /dev/ttyUSB0 or /dev/ttyACM0
    // On Windows: COM3, COM4, etc.
    // On macOS: /dev/cu.usbserial-*
    
    _display = VieweDisplay('/dev/ttyUSB0');
    final connected = await _display!.connect();
    
    if (connected) {
      setState(() => _isConnected = true);
      
      // Listen to touch events
      _display!.touchEvents.listen((event) {
        setState(() {
          _lastTouch = 'Touch at (${event.x}, ${event.y})';
        });
      });
      
      // Run startup demo
      await _runDemo();
    }
  }
  
  Future<void> _runDemo() async {
    if (_display == null || !_isConnected) return;
    
    // Fill screen with blue
    await _display!.fill(Colors.blue);
    await _display!.flush();
    await Future.delayed(Duration(seconds: 1));
    
    // Draw colored rectangles
    await _display!.fill(Colors.black);
    await _display!.fillRect(50, 50, 200, 100, Colors.red);
    await _display!.fillRect(300, 50, 200, 100, Colors.green);
    await _display!.fillRect(550, 50, 200, 100, Colors.blue);
    await _display!.flush();
  }
  
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        title: const Text('VIEWE Display Control'),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Text(
              _isConnected ? 'Connected to display' : 'Connecting...',
              style: Theme.of(context).textTheme.headlineMedium,
            ),
            const SizedBox(height: 20),
            Text(_lastTouch),
            const SizedBox(height: 40),
            if (_isConnected) ...[
              ElevatedButton(
                onPressed: () async {
                  await _display?.fill(Colors.red);
                  await _display?.flush();
                },
                child: const Text('Fill Red'),
              ),
              ElevatedButton(
                onPressed: () async {
                  await _display?.fill(Colors.green);
                  await _display?.flush();
                },
                child: const Text('Fill Green'),
              ),
              ElevatedButton(
                onPressed: () async {
                  await _display?.fill(Colors.blue);
                  await _display?.flush();
                },
                child: const Text('Fill Blue'),
              ),
              ElevatedButton(
                onPressed: () async {
                  await _display?.setBrightness(50);
                },
                child: const Text('50% Brightness'),
              ),
              ElevatedButton(
                onPressed: () async {
                  await _display?.setBrightness(100);
                },
                child: const Text('100% Brightness'),
              ),
            ],
          ],
        ),
      ),
    );
  }
  
  @override
  void dispose() {
    _display?.dispose();
    super.dispose();
  }
}
```

## Testing the Integration

### 1. Flash the ESP32 Firmware

```bash
cd path/to/esphome_UEDX80480070ESP32
git checkout flutter-standalone-driver
pio run --target upload
```

### 2. Find the Serial Port

```bash
# Linux
ls /dev/ttyUSB* /dev/ttyACM*

# macOS
ls /dev/cu.usbserial-*

# Windows
# Use Device Manager to find the COM port
```

### 3. Run the Flutter App

```bash
cd viewe_display_example
flutter pub get
flutter run
```

## Advanced Topics

### Custom Drawing

You can create custom drawing functions by sending multiple primitive commands:

```dart
Future<void> drawSmileyFace(VieweDisplay display) async {
  // Draw yellow circle for face
  await display.fillCircle(400, 240, 100, Colors.yellow);
  
  // Draw eyes
  await display.fillCircle(370, 210, 10, Colors.black);
  await display.fillCircle(430, 210, 10, Colors.black);
  
  // Draw smile (using multiple rectangles)
  for (int x = 360; x <= 440; x += 5) {
    int y = 250 + ((x - 400) * (x - 400)) ~/ 200;
    await display.fillRect(x, y, 5, 5, Colors.black);
  }
  
  await display.flush();
}
```

### Performance Optimization

For better performance:

1. **Batch Updates**: Group multiple drawing commands before calling `flush()`
2. **Direct Framebuffer Access**: For complex rendering, consider extending the protocol
3. **Reduce Flush Frequency**: Only flush when display update is needed
4. **Use Fill Operations**: Faster than individual pixels

### WiFi Communication (Future)

The firmware includes stubs for WiFi communication. To enable:

1. Implement WiFi initialization in firmware
2. Create TCP socket server
3. Update Flutter package to support TCP connections
4. Use same protocol over WiFi

## Troubleshooting

### Display Not Responding

1. Check serial connection and baud rate (115200)
2. Verify ESP32 is powered properly
3. Check that firmware is flashed correctly
4. Look at serial monitor for error messages

### Touch Events Not Working

1. Verify GT911 is properly initialized (check serial logs)
2. Check I2C connections
3. Ensure touch controller firmware is loaded
4. Test touch separately with GET_TOUCH command

### Performance Issues

1. Reduce number of flush() calls
2. Use fill operations instead of individual pixels
3. Consider WiFi for higher bandwidth
4. Optimize Flutter app rendering

## Next Steps

1. Publish Dart package to pub.dev
2. Add more drawing primitives (circles, lines, text)
3. Implement bitmap transfer for images
4. Add WiFi communication support
5. Create widget library for common UI elements
6. Add gesture recognition

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Submit a pull request

## License

This driver and example code are provided as-is for use with VIEWE display hardware.
