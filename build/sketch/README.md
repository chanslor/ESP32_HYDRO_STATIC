#line 1 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/README.md"
# ESP32 Hydrostatic Water Level Sensor with OLED Display

A complete water depth measurement system using an industrial 4-20 mA hydrostatic pressure sensor, INA219 current sensor module, 0.96" OLED display, and ESP32 microcontroller. This system accurately measures water depth in tanks and displays real-time readings on both an OLED screen and via serial output.

## Overview

This project reads industrial 4-20 mA current loop signals from a hydrostatic pressure sensor and converts them to water depth measurements. The system uses an INA219 current sensor to measure the loop current, which is then processed by an ESP32 to calculate and display water depth in both centimeters and percentage full. A compact 0.96" OLED display provides real-time visual feedback, making the system ideal for standalone field deployment.

### Key Features

- **0.96" OLED Display** - Real-time visual readout without needing a computer
- **4-20 mA current loop interface** for industrial sensor compatibility
- **High-precision INA219** current measurement (±0.8 mA resolution)
- **Noise-resistant averaging** of 10 samples per reading
- **Dual output format**: Depth in cm and percentage full
- **Automatic fault detection** for sensor/wiring issues
- **Serial output** at 115200 baud for monitoring and data logging
- **Easy calibration** via single constant (tank depth)
- **Compact design** - All components share I2C bus
- **Low memory footprint**: 325KB program, 22KB RAM

## Hardware Requirements

### Components

- **ALS-MPM-2F Hydrostatic Sensor** (4-20 mA output)
- **ESP32 Development Board** (DevKit or similar)
- **INA219 Current Sensor Module** (I2C interface)
- **0.96" OLED Display Module** (SSD1306, 128x64, I2C)
- **19.5V Dell Power Supply** (barrel connector power brick)
- Connecting wires

### Why These Components?

- **Hydrostatic Sensor**: Measures water pressure which correlates directly to depth
- **4-20 mA Current Loop**: Industry standard for long-distance, noise-immune sensing
- **INA219**: Precision current sensor that measures the loop current
- **OLED Display**: Low-power, high-contrast display for real-time readings in field use
- **ESP32**: WiFi-capable microcontroller (future expansion for remote monitoring)
- **Dell PSU**: Provides required voltage for 4-20 mA loop operation

## Wiring Diagram

```
=====================================================================
HYDROSTATIC SENSOR + INA219 + ESP32 + DELL 19.5 V SUPPLY (4–20 mA loop)
=====================================================================
CURRENT FLOW  (left to right)
  +19.5 V  ─────────────────────────────────────────────────────▶

   DELL BRICK                    INA219 SHUNT                 HYDROSTATIC SENSOR
   -----------                -------------------             -------------------
                              |                 |
   White (+19.5 V) ---------->|  VIN+     VIN-  |------------> RED  (+V)
                              |                 |

   Black (GND) -------------------------------------------> BLACK (4–20 mA return)

   Blue (ID)  --> not used


          INA219 MODULE                      ESP32 DEV BOARD
          ==============                =========================
          VCC   o--------------------->  3V3
          GND   o---------------------.> GND
                                       |
          SDA   o--------------------->| GPIO 21  (SDA)
          SCL   o--------------------->| GPIO 22  (SCL)
                                       |
   Dell BLACK (GND) -------------------'   (all grounds common)
```

### Connection Summary

**INA219 to ESP32 (I2C + Power):**
- VCC → ESP32 3V3
- GND → ESP32 GND
- SDA → ESP32 GPIO 21
- SCL → ESP32 GPIO 22

**OLED Display to ESP32 (shares I2C bus with INA219):**
- VCC (or VDD) → ESP32 3V3
- GND → ESP32 GND
- SDA → ESP32 GPIO 21 (same as INA219)
- SCL → ESP32 GPIO 22 (same as INA219)

**I2C Addresses:**
- INA219: 0x40 (default)
- OLED SSD1306: 0x3C (default)
- No address conflicts - both devices coexist on same bus

**Current Loop (Dell PSU → INA219 → Sensor):**
- Dell White (+19.5V) → INA219 VIN+
- INA219 VIN- → Sensor RED wire (+V)
- Sensor BLACK wire → Dell Black (GND)
- All grounds must be common

## How It Works

### 4-20 mA Current Loop

Industrial sensors use current loops because:
- Current is immune to voltage drops over long cable runs
- Easy to detect broken wires (current drops to 0 mA)
- Industry standard for process control

The hydrostatic sensor outputs:
- **4 mA** when tank is empty (0% depth)
- **20 mA** when tank is full (100% depth)
- Linear scale in between (e.g., 12 mA = 50% full)

### Measurement Process

1. **Pressure Sensing**: Water depth creates pressure on the sensor
2. **Current Output**: Sensor converts pressure to 4-20 mA current
3. **Current Measurement**: INA219 measures the loop current
4. **I2C Communication**: ESP32 reads current value from INA219
5. **Averaging**: 10 samples averaged over 1 second for stability
6. **Conversion**: Linear interpolation converts mA to depth
7. **Display**: Results shown on OLED display and via serial output every 2 seconds

### Calculation Formula

```
Depth (cm) = ((Current - 4) / (20 - 4)) × MAX_DEPTH_CM
Percentage = ((Current - 4) / (20 - 4)) × 100
```

## Quick Start

### 1. Install Arduino Libraries

**Required Libraries:**
- Adafruit INA219 (for current sensor)
- Adafruit SSD1306 (for OLED display)
- Adafruit GFX Library (dependency, auto-installed)

Using Arduino IDE:
```
Sketch → Include Library → Manage Libraries
Search: "Adafruit INA219" → Install
Search: "Adafruit SSD1306" → Install
```

Or using arduino-cli:
```bash
arduino-cli lib install "Adafruit INA219"
arduino-cli lib install "Adafruit SSD1306"
```

### 2. Configure Tank Depth

Edit `ESP32_HYDRO_STATIC.ino` line 37:
```cpp
const float MAX_DEPTH_CM = 100.0;  // Change to your tank depth
```

### 3. Upload Code

Using Arduino IDE:
- Board: ESP32 Dev Module
- Port: Select your ESP32 port
- Click Upload

Using arduino-cli:
```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .
```

### 4. Monitor Output

Arduino IDE: Tools → Serial Monitor (115200 baud)

arduino-cli:
```bash
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### Expected Output

**Serial Monitor (115200 baud):**
```
ESP32 Hydrostatic Water Level Sensor
=====================================
INA219 initialized successfully
OLED display initialized successfully

Tank depth range: 0 - 100 cm
Current range: 4 - 20 mA

Starting measurements...
=====================================

--- Water Level Reading ---
Current: 12.05 mA
Depth: 50.3 cm (50.3%)
```

**OLED Display Layout:**
```
┌────────────────────────┐
│ Water Level Reading    │
│────────────────────────│
│ Current: 7.46 mA       │
│                        │
│ Depth:                 │
│ 21.6 cm       ← Large  │
│                        │
│                (21.6%) │
└────────────────────────┘
```

## OLED Display Features

The 0.96" OLED provides clear, real-time visual feedback:

- **Startup Screen**: Shows "Water Level Sensor - Initializing..." on boot
- **Title Bar**: "Water Level Reading" with separator line
- **Current Reading**: Displays loop current in mA (small text)
- **Depth Value**: Large, easy-to-read depth in centimeters
- **Percentage**: Tank fill percentage in bottom-right corner
- **Update Rate**: Refreshes every 2 seconds
- **High Contrast**: White text on black background for outdoor visibility
- **Low Power**: OLED only draws ~20 mA

The display makes the system perfect for field installation where a computer is not available.

## Configuration

All configuration is done via constants at the top of `ESP32_HYDRO_STATIC.ino`:

```cpp
const float MIN_CURRENT_MA = 4.0;      // Current at empty tank
const float MAX_CURRENT_MA = 20.0;     // Current at full tank
const float MAX_DEPTH_CM = 100.0;      // Your tank depth in cm
const int SAMPLE_COUNT = 10;           // Samples to average
const unsigned long SAMPLE_DELAY_MS = 100;  // Delay between samples
```

**Typical Adjustments:**
- `MAX_DEPTH_CM`: Set to your actual tank depth
- `SAMPLE_COUNT`: Increase for smoother readings (slower response)
- Loop `delay(2000)`: Change reading frequency

## Troubleshooting

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| "Failed to find INA219" | I2C wiring issue | Check SDA/SCL connections to GPIO 21/22 |
| "Failed to find SSD1306 OLED" | OLED not connected | Check OLED wiring, verify VCC/GND |
| OLED blank/not working | Wrong I2C address | Try 0x3D if 0x3C fails (rare) |
| Both devices fail | I2C bus problem | Check all SDA/SCL connections |
| Current = 0 mA | Power supply off | Turn on Dell PSU |
| Current < 4 mA | Broken wire | Check all connections |
| Current > 20 mA | Sensor fault | Check sensor specification |
| Noisy readings | Electrical interference | Increase SAMPLE_COUNT |
| Wrong depth values | Wrong MAX_DEPTH_CM | Measure and update constant |
| OLED shows garbled text | Interference | Check for loose I2C connections |

See [SETUP.md](SETUP.md) for detailed troubleshooting steps.

## Technical Specifications

**Sensor Specifications:**
- **Measurement Range**: 4-20 mA (industry standard)
- **Resolution**: ±0.8 mA (INA219 accuracy)
- **Depth Resolution**: ~0.5% of tank depth
- **Update Rate**: 2 seconds per reading
- **Averaging**: 10 samples over 1 second

**Display Specifications:**
- **OLED Type**: SSD1306, 128x64 pixels, monochrome
- **Display Size**: 0.96 inches diagonal
- **Refresh Rate**: 2 seconds
- **Visibility**: High contrast, outdoor readable

**System Specifications:**
- **Serial Baud Rate**: 115200
- **I2C Bus Speed**: 100 kHz (default)
- **I2C Addresses**: INA219 (0x40), OLED (0x3C)
- **Program Size**: 324,563 bytes (24% of ESP32 flash)
- **RAM Usage**: 22,360 bytes (6% of ESP32 RAM)
- **Power Consumption**: ~170 mA (ESP32 + INA219 + OLED)

## Code Structure

```
ESP32_HYDRO_STATIC.ino
├── setup()
│   ├── Initialize Serial (115200)
│   ├── Initialize I2C (GPIO 21/22)
│   ├── Initialize INA219 (0x40)
│   ├── Initialize OLED Display (0x3C)
│   └── Show startup screen on OLED
├── loop()
│   ├── getAverageCurrent() → Read & average 10 samples
│   ├── calculateDepth() → Convert mA to cm
│   ├── calculatePercentage() → Convert mA to %
│   ├── Display to Serial Monitor
│   ├── updateOLEDDisplay() → Render to OLED screen
│   └── Fault detection & warnings
└── updateOLEDDisplay()
    ├── Clear display buffer
    ├── Draw title bar
    ├── Show current (mA)
    ├── Show depth (cm) in large text
    ├── Show percentage
    └── Update physical display
```

## Future Enhancements

Possible additions to this project:
- **WiFi connectivity** for remote monitoring
- **MQTT publishing** to IoT platforms (Home Assistant, Node-RED, etc.)
- **Web server** for browser-based access and configuration
- **SD card data logging** for historical tracking
- **Alarm triggers** for low/high water level warnings
- **Email/SMS notifications** via WiFi
- **Multiple sensor support** for monitoring multiple tanks
- **Battery backup** with sleep modes for power-outage resilience
- **Graphing** water level trends on OLED or web interface

## Safety Notes

- **Voltage Warning**: 19.5V present in current loop - use caution
- **Water Safety**: Ensure all electronics are properly housed/sealed
- **Sensor Depth Rating**: Verify sensor max pressure rating matches tank
- **Electrical Isolation**: Do not submerge non-waterproof components

## License

This project is open source and provided as-is for educational and practical use.

## Acknowledgments

- **Adafruit INA219 Library** - High-precision current sensing
- **Adafruit SSD1306 Library** - OLED display driver
- **Adafruit GFX Library** - Graphics primitives for display
- Designed for **ALS-MPM-2F** hydrostatic sensor
- Compatible with standard **4-20 mA industrial sensors**

