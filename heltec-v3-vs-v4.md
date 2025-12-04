# Heltec WiFi LoRa 32: V3 vs V4 Comparison

*Research date: 2025-12-03*

## Overview

The WiFi LoRa 32 V4 is the successor to V3, featuring the same ESP32-S3 MCU and SX1262 LoRa chip. V4 adds solar charging, GNSS connector, higher TX power, and improved hardware protection.

## Specifications Comparison

| Feature | V3 | V4 |
|---------|----|----|
| MCU | ESP32-S3 | ESP32-S3 (same) |
| LoRa Chip | SX1262 | SX1262 (same) |
| OLED | SSD1306, 0.96" 128x64 | SSD1315, 0.96" 128x64 |
| Flash | 8MB | 16MB |
| PSRAM | 2MB | 2MB |
| Pin Headers | 36-pin | 40-pin |
| TX Power | 21 dBm | 27-28 dBm |
| USB | Type-C | Type-C with ESD protection |
| Solar Input | No | Yes (SH1.25-2Pin) |
| GNSS Connector | No | Yes (SH1.25-8Pin) |
| Dimensions | 51.7 × 25.4 mm | 51.7 × 25.4 × 10.7 mm |
| Operating Temp | -40~85°C | -40~85°C |
| Deep Sleep | ~20 μA | ~20 μA |

## Arduino-CLI Compatibility

### Current Status

**No dedicated V4 board definition exists yet.** Use the V3 FQBN:

```bash
# Compile for V4 using V3 board definition
arduino-cli compile --build-path ./build --fqbn esp32:esp32:heltec_wifi_lora_32_V3 .

# Upload
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 --input-dir ./build .
```

### Why This Works

- Same ESP32-S3 chip
- Nearly identical pinout
- Compatible I2C bus configuration
- SSD1315 OLED is protocol-compatible with SSD1306

## Potential Compatibility Issues

### 1. OLED Driver (Minor)
V4 uses SSD1315 instead of SSD1306. These are compatible at the I2C protocol level, but there may be minor initialization differences. The Adafruit SSD1306 library should still work.

### 2. LoRa TX Power Settings
V4 has higher transmit power (27-28 dBm vs 21 dBm). Power level constants in LoRa code may not map 1:1 to actual output power.

### 3. Third-Party Libraries
Some V3-specific libraries explicitly state they don't support V4:
- `ropg/heltec_esp32_lora_v3` - Does NOT support V4

## Project-Specific Notes (ESP32_HYDRO_STATIC)

This project should work on V4 because:
- Uses I2C for INA219 and OLED (compatible)
- Uses ADC for moisture sensor (compatible)
- Does not use LoRa features
- OLED library (Adafruit SSD1306) should work with SSD1315

No code changes expected, but if OLED issues occur, check initialization parameters.

## Sources

- [Heltec WiFi LoRa 32 V4 Product Page](https://heltec.org/project/wifi-lora-32-v4/)
- [Heltec WiFi LoRa 32 V4 Wiki](https://wiki.heltec.org/docs/devices/open-source-hardware/esp32-series/lora-32/wifi-lora-32-v4/)
- [CNX Software - V4 Review (Nov 2025)](https://www.cnx-software.com/2025/11/27/heltec-wifi-lora-32-v4-an-esp32-s3-off-grid-lora-meshtastic-communicator-with-solar-input-gnss-and-28dbm-tx-power/)
- [ropg/heltec_esp32_lora_v3 Library](https://github.com/ropg/heltec_esp32_lora_v3)
- [Heltec WiFi LoRa 32 V3 Product Page](https://heltec.org/project/wifi-lora-32-v3/)
