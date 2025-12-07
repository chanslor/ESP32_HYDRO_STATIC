# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Working Directory

**Always use this directory for all commands:**
```
/home/mdchansl/IOT/ESP32_HYDRO_STATIC/
```

When running bash commands, use absolute paths or `cd` to this directory first.


## Project Overview

ESP32-based hydrostatic water level sensor system using a Heltec WiFi LoRa 32 board with built-in OLED display. Features:
- Reads 4-20 mA industrial current loop signals from a hydrostatic pressure sensor via INA219 (optional)
- LM393 soil moisture sensor for rain detection and soil monitoring
- Converts to water depth measurements displayed in inches/feet
- INA219 is optional - system continues in moisture-only mode if not connected
- **LoRa wireless network** for remote monitoring (river → ridge relay → home)

## Project Structure

```
ESP32_HYDRO_STATIC/
├── ESP32_HYDRO_STATIC.ino    # Original standalone sketch (no LoRa)
├── lora_config.h             # Shared LoRa settings for all units
├── river_unit/               # LoRa-enabled sensor + transmitter
├── ridge_relay/              # Battery-powered LoRa repeater
├── home_unit/                # LoRa receiver + display
└── *.md                      # Documentation files
```

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

**Note:** Always use `--build-path ./build` for compile and `--input-dir ./build` for upload to use a consistent build directory.

## Required Libraries

```bash
arduino-cli lib install "Adafruit INA219"
arduino-cli lib install "Adafruit SSD1306"
arduino-cli lib install "RadioLib"
```

## LoRa Network Build Commands

```bash
# River Unit (sensor + LoRa TX)
cd river_unit
arduino-cli compile --build-path ./build --fqbn esp32:esp32:heltec_wifi_lora_32_V3 .
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 --input-dir ./build .

# Ridge Relay (battery-powered repeater)
cd ridge_relay
arduino-cli compile --build-path ./build --fqbn esp32:esp32:heltec_wifi_lora_32_V3 .
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 --input-dir ./build .

# Home Unit (receiver + display)
cd home_unit
arduino-cli compile --build-path ./build --fqbn esp32:esp32:heltec_wifi_lora_32_V3 .
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 --input-dir ./build .
```

## Architecture

Single-file Arduino sketch (`ESP32_HYDRO_STATIC.ino`) with conditional compilation for V2/V3 board variants.

### Key Configuration (lines 38-105)

- **Board selection**: `#define HELTEC_V2` or `#define HELTEC_V3` (line 40-41)
- **Tank depth**: `MAX_DEPTH_CM` constant - set to actual tank depth in centimeters
- **Sensor calibration**: `MIN_CURRENT_MA` (4.0) and `MAX_CURRENT_MA` (20.0) for standard 4-20mA loop
- **Moisture calibration**: `MOISTURE_DRY_VALUE` (4095) and `MOISTURE_WET_VALUE` (1500) for ADC mapping

### I2C Bus Configuration

**V3 (default)**: Two separate I2C buses
- OLED: GPIO 17/18 (internal, Wire)
- INA219: GPIO 1/2 (external, Wire1 via `I2C_INA219`)
- Moisture sensor: GPIO 4 (ADC input)

**V2**: Single shared I2C bus
- Both I2C devices on GPIO 4/15 (Wire)
- Moisture sensor: GPIO 36 (ADC input)

### Main Functions

- `getAverageCurrent()`: Reads 10 samples from INA219, returns averaged mA value
- `calculateDepth(current_mA)`: Linear interpolation from mA to cm depth
- `calculatePercentage(current_mA)`: Linear interpolation from mA to percentage
- `getAverageMoisture()`: Reads 5 ADC samples from LM393, returns averaged raw value
- `calculateMoisturePercent(rawValue)`: Converts ADC value to 0-100% (inverted scale)
- `updateOLEDDisplay()`: Renders current, depth (ft/in format), percentage, and moisture to built-in OLED
  - Has two display modes: full (with INA219) and moisture-only (without INA219)

### Global State

- `ina219Available`: Boolean flag indicating if INA219 was successfully initialized

### Hardware Addresses

- INA219: 0x40 (I2C)
- OLED: 0x3C (I2C)
- Moisture sensor: GPIO 4 (V3) or GPIO 36 (V2) - analog input
- Vext power control: GPIO 36 (must be LOW to enable OLED on some boards)

### LoRa Configuration (lora_config.h)

- Frequency: 915.0 MHz (US ISM band)
- Spreading Factor: 9
- Bandwidth: 125 kHz
- TX Power: 14 dBm
- Sync Word: 0x12 (private network)

### Ridge Relay Test Mode

The ridge relay has `#define TEST_MODE true/false` at the top:
- `true`: Display stays on, no deep sleep (for testing)
- `false`: Deep sleep enabled (for battery deployment)
