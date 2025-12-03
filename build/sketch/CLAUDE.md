#line 1 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/CLAUDE.md"
# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Working Directory

**Always use this directory for all commands:**
```
/home/mdchansl/IOT/ESP32_HYDRO_STATIC/
```

When running bash commands, use absolute paths or `cd` to this directory first.


## Project Overview

ESP32-based hydrostatic water level sensor system using a Heltec WiFi LoRa 32 board with built-in OLED display. Reads 4-20 mA industrial current loop signals from a hydrostatic pressure sensor via INA219, converts to water depth measurements displayed in inches/feet.

## Build Commands

```bash
# Compile for Heltec V3 (ESP32-S3)
arduino-cli compile --build-path ./build --fqbn esp32:esp32:heltec_wifi_lora_32_V3 .

# Upload to board
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 --input-dir ./build .

# Monitor serial output
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200

# For V2 boards, replace heltec_wifi_lora_32_V3 with heltec_wifi_lora_32_V2
```

## Required Libraries

```bash
arduino-cli lib install "Adafruit INA219"
arduino-cli lib install "Adafruit SSD1306"
```

## Architecture

Single-file Arduino sketch (`ESP32_HYDRO_STATIC.ino`) with conditional compilation for V2/V3 board variants.

### Key Configuration (lines 38-78)

- **Board selection**: `#define HELTEC_V2` or `#define HELTEC_V3` (line 40-41)
- **Tank depth**: `MAX_DEPTH_CM` constant - set to actual tank depth in centimeters
- **Sensor calibration**: `MIN_CURRENT_MA` (4.0) and `MAX_CURRENT_MA` (20.0) for standard 4-20mA loop

### I2C Bus Configuration

**V3 (default)**: Two separate I2C buses
- OLED: GPIO 17/18 (internal, Wire)
- INA219: GPIO 1/2 (external, Wire1 via `I2C_INA219`)

**V2**: Single shared I2C bus
- Both devices on GPIO 4/15 (Wire)

### Main Functions

- `getAverageCurrent()`: Reads 10 samples from INA219, returns averaged mA value
- `calculateDepth(current_mA)`: Linear interpolation from mA to cm depth
- `calculatePercentage(current_mA)`: Linear interpolation from mA to percentage
- `updateOLEDDisplay()`: Renders current, depth (ft/in format), and percentage to built-in OLED

### Hardware Addresses

- INA219: 0x40
- OLED: 0x3C
