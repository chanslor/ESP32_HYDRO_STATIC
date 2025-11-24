#line 1 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/HELTEC_WIRING.md"
# Heltec WiFi LoRa 32 Wiring Guide

## Board Information

The code is configured for **Heltec WiFi LoRa 32 V2** (most common version).

If you have **V3**, edit line 40 in the .ino file:
```cpp
// Comment out V2 and uncomment V3:
// #define HELTEC_V2
#define HELTEC_V3
```

## Pin Assignments

### Heltec WiFi LoRa 32 V2
- **Built-in OLED**: SDA=GPIO 4, SCL=GPIO 15, RST=GPIO 16
- **LoRa Module**: Uses separate SPI pins (not used in this project yet)

### Heltec WiFi LoRa 32 V3
- **Built-in OLED**: SDA=GPIO 17, SCL=GPIO 18, RST=GPIO 21
- **LoRa Module**: Uses separate SPI pins (not used in this project yet)

## Wiring Connections

### INA219 Current Sensor → Heltec Board

**For V2:**
```
INA219          Heltec WiFi LoRa 32 V2
------          ----------------------
VCC      --->   3V3
GND      --->   GND
SDA      --->   GPIO 4  (shares with built-in OLED)
SCL      --->   GPIO 15 (shares with built-in OLED)
```

**For V3:**
```
INA219          Heltec WiFi LoRa 32 V3
------          ----------------------
VCC      --->   3V3
GND      --->   GND
SDA      --->   GPIO 17 (shares with built-in OLED)
SCL      --->   GPIO 18 (shares with built-in OLED)
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
2. **I2C Bus Sharing**: The INA219 shares the I2C bus with the built-in OLED
3. **I2C Addresses**:
   - INA219: 0x40
   - Built-in OLED: 0x3C
   - No conflicts!

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

### Serial Monitor
```
Heltec WiFi LoRa 32 - Hydrostatic Water Level Sensor
====================================================
Board: Heltec WiFi LoRa 32 V2
INA219 initialized successfully
Built-in OLED display initialized successfully

Tank depth range: 0 - 100 cm
Current range: 4 - 20 mA

Starting measurements...
====================================================

--- Water Level Reading ---
Current: 7.46 mA
Depth: 8.5 in (21.6%)
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
| OLED not working | Check you selected correct version (V2/V3) |
| INA219 not found | Verify wiring to GPIO 4/15 (V2) or 17/18 (V3) |
| Upload fails | Press PRG button while uploading |
| Board not detected | Install CP210x USB driver if needed |

## Next Steps: LoRa Integration

The Heltec board has a built-in LoRa module (SX1276/SX1278) that can be used for:
- Long-range wireless data transmission (up to 10km line-of-sight)
- Remote monitoring without WiFi
- Creating a LoRaWAN sensor network

This will be added in future updates!

## Board Resources

- **Flash Memory**: 4MB (vs 4MB standard ESP32)
- **Flash Usage**: 324,943 bytes (9%)
- **RAM Usage**: 22,360 bytes (6%)
- **Plenty of room** for future LoRa features!
