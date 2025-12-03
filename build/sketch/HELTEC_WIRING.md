#line 1 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/HELTEC_WIRING.md"
# Heltec WiFi LoRa 32 Wiring Guide

## Board Information

The code supports both **Heltec WiFi LoRa 32 V2** and **V3** (ESP32-S3).

**Current Configuration**: **V3** (default in code)

To switch versions, edit line 40-41 in `ESP32_HYDRO_STATIC.ino`:
```cpp
// For V2 (older):
#define HELTEC_V2
// #define HELTEC_V3

// For V3 (newer, ESP32-S3) - CURRENT SETTING:
// #define HELTEC_V2
#define HELTEC_V3
```

## Pin Assignments

### Heltec WiFi LoRa 32 V2
- **Built-in OLED**: SDA=GPIO 4, SCL=GPIO 15, RST=GPIO 16 (shared I2C bus)
- **INA219 Sensor**: SDA=GPIO 4, SCL=GPIO 15 (shares bus with OLED)
- **LoRa Module**: SX1276/SX1278 on separate SPI pins

### Heltec WiFi LoRa 32 V3 (ESP32-S3)
- **Built-in OLED**: SDA=GPIO 17, SCL=GPIO 18, RST=GPIO 21 (internal, not on headers!)
- **INA219 Sensor**: SDA=GPIO 1, SCL=GPIO 2 (separate I2C bus, Header J3 Pins 12/13)
- **LoRa Module**: SX1262 on separate SPI pins
- **Important**: GPIO 17/18 are NOT accessible on pin headers - they're internal to the board

## INA219 Current Sensor Module

![INA219 Module](INA219-power-supply.png)

The **INA219** is a high-precision current sensor module that measures the 4-20 mA signal from the hydrostatic sensor.

**Key Features:**
- **I2C Interface**: Communicates with ESP32 via 2 wires (SDA/SCL)
- **High Precision**: ±0.8 mA accuracy for 4-20 mA measurements
- **Bi-directional**: Measures current in both directions
- **Low Power**: Operates on 3.3V from the Heltec board
- **Compact Size**: 25.5mm × 22.3mm module

**Pin Functions:**
- **VIN+ / VIN-**: Current loop connections (19.5V passes through here)
- **VCC**: 3.3V power from Heltec
- **GND**: Ground connection
- **SDA**: I2C data line to ESP32
- **SCL**: I2C clock line to ESP32

**What It Does:**
The INA219 sits in the 4-20 mA current loop and measures the exact current flowing through it. This current value is proportional to water depth, which the ESP32 then converts to inches/feet for display.

## Wiring Connections

### INA219 Current Sensor → Heltec Board

**For V2 (shared I2C bus):**
```
INA219          Heltec WiFi LoRa 32 V2
------          ----------------------
VCC      --->   3V3
GND      --->   GND
SDA      --->   GPIO 4  (shares with built-in OLED)
SCL      --->   GPIO 15 (shares with built-in OLED)
```

**For V3 (separate I2C bus on Header J3):**
```
INA219          Heltec WiFi LoRa 32 V3 (Header J3 - Left Side)
------          ----------------------------------------------
VCC      --->   3V3 (Pin 2 or 3)
GND      --->   GND (Pin 1)
SDA      --->   GPIO 1  (Pin 12) ← Accessible on header
SCL      --->   GPIO 2  (Pin 13) ← Accessible on header

Note: GPIO 17/18 are used INTERNALLY by the built-in OLED
      and are NOT available on the pin headers!
```

### Current Loop (same for all versions)
```
Dell Power Supply → INA219 → Hydrostatic Sensor

Dell White (+19.5V) → INA219 VIN+
INA219 VIN-         → Sensor RED wire (+V)
Sensor BLACK wire   → Dell Black (GND)
All grounds common
```

## Important Notes

1. **No External OLED Needed**: The Heltec board has a built-in 0.96" OLED display
2. **I2C Bus Configuration**:
   - **V2**: Single I2C bus shared between OLED and INA219
   - **V3**: Two separate I2C buses (OLED on GPIO 17/18, INA219 on GPIO 1/2)
3. **I2C Addresses**:
   - INA219: 0x40
   - Built-in OLED: 0x3C
   - No conflicts on either version!
4. **V3 Critical Note**: GPIO 17/18 are internal-only pins. Use GPIO 1/2 for external devices.

## Upload Instructions

### Connect Heltec Board
1. Connect Heltec to computer via USB-C cable
2. The board should appear as `/dev/ttyUSB0` or similar

### Compile and Upload

**For V2 (default):**
```bash
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V2 .
arduino-cli upload --fqbn esp32:esp32:heltec_wifi_lora_32_V2 --port /dev/ttyUSB0 .
```

**For V3:**
```bash
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 .
arduino-cli upload --fqbn esp32:esp32:heltec_wifi_lora_32_V3 --port /dev/ttyUSB0 .
```

### Monitor Output
```bash
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## Expected Output

### Serial Monitor (V3 Example)
```
Heltec WiFi LoRa 32 - Hydrostatic Water Level Sensor
====================================================
Board: Heltec WiFi LoRa 32 V3
I2C: OLED on GPIO17/18, INA219 on GPIO1/2
INA219 initialized successfully
Built-in OLED display initialized successfully

Tank depth range: 0 - 100 cm
Current range: 4 - 20 mA

Starting measurements...
====================================================

--- Water Level Reading ---
Current: 7.46 mA
Depth: 8.5 in (21.6%)

--- Water Level Reading ---
Current: 12.05 mA
Depth: 1 ft 7.8 in (50.3%)
```

### Built-in OLED Display
The built-in display will show:
- Water Level Reading (title)
- Current in mA
- Depth in inches (or feet + inches if > 12")
- Percentage full

## Troubleshooting

| Issue | Solution |
|-------|----------|
| OLED not working | Check you selected correct version (V2/V3 in code) |
| INA219 not found on V2 | Verify wiring to GPIO 4 (SDA) and GPIO 15 (SCL) |
| INA219 not found on V3 | Verify wiring to GPIO 1 (SDA) and GPIO 2 (SCL) - NOT GPIO 17/18! |
| Upload fails | Press and hold PRG button during upload |
| Board not detected | Install CP2102 USB-to-serial driver if needed |
| "Wrong chip" error on V3 | Use heltec_wifi_lora_32_V3 FQBN (ESP32-S3, not ESP32) |

## Next Steps: LoRa Integration

The Heltec board has a built-in LoRa module ready for future use:
- **V2**: SX1276/SX1278 LoRa chip
- **V3**: SX1262 LoRa chip (newer, more efficient)

Capabilities:
- Long-range wireless data transmission (up to 10km line-of-sight)
- Remote tank monitoring without WiFi infrastructure
- LoRaWAN integration for IoT networks
- Battery-powered remote installations

This will be added in future updates!

## Board Resources

**Heltec WiFi LoRa 32 V3 (ESP32-S3):**
- **MCU**: ESP32-S3 dual-core @ 240MHz
- **Flash Memory**: 4MB (8MB on some variants)
- **RAM**: 327KB SRAM
- **Current Flash Usage**: 331,979 bytes (9% of 4MB)
- **Current RAM Usage**: 21,888 bytes (6% of RAM)
- **Plenty of room** for LoRa, WiFi, and additional features!

**Heltec WiFi LoRa 32 V2:**
- **MCU**: ESP32 dual-core @ 240MHz
- **Flash Memory**: 4MB
- **RAM**: 320KB SRAM
