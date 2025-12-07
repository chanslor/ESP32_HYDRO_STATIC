# T-DECK Ridge Relay Unit

Secondary/backup LoRa repeater for the river monitoring network using the LILYGO T-Deck.

## Overview

This unit works alongside the primary Heltec ridge relay to provide redundant signal relay from the river sensor to the home unit. It uses a **staggered 300ms delay** before retransmitting to avoid RF collisions with the primary relay (which uses 50ms).

## Hardware

- **Board**: LILYGO T-Deck (ESP32-S3 + SX1262 + ST7789 LCD)
- **Display**: 2.8" ST7789 320x240 IPS LCD
- **Radio**: SX1262 LoRa transceiver (915 MHz)
- **Antenna**: External antenna connector
- **Power**: USB-C or LiPo battery

## Pin Configuration

| Function | GPIO | Notes |
|----------|------|-------|
| **LoRa** | | |
| NSS/CS | 9 | SPI chip select |
| RST | 17 | Radio reset |
| DIO1 | 45 | Interrupt |
| BUSY | 13 | Busy status |
| SCK | 40 | SPI clock (shared) |
| MISO | 38 | SPI data in |
| MOSI | 41 | SPI data out (shared) |
| **Display** | | |
| CS | 12 | SPI chip select |
| DC | 11 | Data/Command |
| SCK | 40 | SPI clock (shared) |
| MOSI | 41 | SPI data out (shared) |
| Backlight | 42 | PWM capable |
| **Power** | | |
| Power ON | 10 | Must be HIGH for peripherals |
| Battery ADC | 4 | Voltage divider (2:1) |
| **Other** | | |
| I2C SDA | 18 | Keyboard, touch |
| I2C SCL | 8 | Keyboard, touch |
| SD Card CS | 39 | MicroSD slot |
| KB Interrupt | 46 | Keyboard |

## Network Operation

```
River Unit (sensor)
      |
      | LoRa 915 MHz
      v
+------------------+     +------------------+
| Heltec Relay     |     | T-Deck Relay     |
| (Primary)        |     | (Secondary)      |
| 50ms delay       |     | 300ms delay      |
+------------------+     +------------------+
      |                         |
      | LoRa 915 MHz           |
      v                         v
         +------------------+
         | Home Unit        |
         | (Receiver)       |
         +------------------+
```

### Dual Relay Benefits

1. **Redundancy**: If primary relay fails, secondary takes over
2. **Better Coverage**: Different antenna positions may reach different areas
3. **Collision Avoidance**: Staggered timing prevents packet corruption

### Unit IDs

| Unit | ID | Description |
|------|-----|-------------|
| River | 0x01 | Sensor transmitter |
| Ridge (Primary) | 0x02 | Heltec relay, 50ms delay |
| Ridge (Secondary) | 0x05 | T-Deck relay, 300ms delay |
| Home | 0x03 | Receiver/display |

## Build & Upload

### Prerequisites

```bash
# Install required libraries
arduino-cli lib install "RadioLib"
arduino-cli lib install "GFX Library for Arduino"
```

### Compile

```bash
cd /home/mdchansl/IOT/ESP32_HYDRO_STATIC/tdeck_relay
arduino-cli compile --build-path ./build \
  --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc .
```

### Upload

The T-Deck uses native USB and appears as `/dev/ttyACM0`:

```bash
arduino-cli upload -p /dev/ttyACM0 \
  --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc \
  --input-dir ./build .
```

If the device doesn't appear:
1. Hold **BOOT** button
2. Press and release **RST** button
3. Release **BOOT** button
4. Try upload again

### Monitor Serial Output

```bash
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200
```

## Configuration

### Test Mode

At the top of `tdeck_relay.ino`:

```cpp
#define TEST_MODE true   // Display stays on, no deep sleep
#define TEST_MODE false  // Deep sleep enabled for battery deployment
```

### LoRa Settings

Configured in `../lora_config.h` (must match all units):

| Parameter | Value |
|-----------|-------|
| Frequency | 915.0 MHz |
| Bandwidth | 125 kHz |
| Spreading Factor | 9 |
| TX Power | 14 dBm |
| Sync Word | 0x12 |

## Display

Black background with green text showing:

- **Header**: "T-DECK RELAY" with "(Secondary - 300ms delay)"
- **Relayed**: Packet count
- **Current**: INA219 reading (mA)
- **Moisture**: Soil sensor reading (%)
- **Depth**: Calculated water depth (ft/in)
- **RSSI**: Signal strength from river unit (dBm)
- **Status**: RELAYING or waiting message
- **Battery**: Voltage reading

## Libraries Used

- **RadioLib** (v7.4.0+): SX1262 LoRa driver
- **GFX Library for Arduino** (v1.6.3+): ST7789 display driver
  - No global User_Setup.h required - all pins configured in sketch

## Differences from Heltec Relay

| Feature | Heltec (Primary) | T-Deck (Secondary) |
|---------|------------------|-------------------|
| Relay Delay | 50ms | 300ms |
| Unit ID | 0x02 | 0x05 |
| Display | SSD1306 OLED 128x64 | ST7789 LCD 320x240 |
| Display Library | Adafruit SSD1306 | Arduino_GFX |
| USB Port | /dev/ttyUSB0 | /dev/ttyACM0 |
| Board FQBN | heltec_wifi_lora_32_V3 | esp32s3:CDCOnBoot=cdc |

## Troubleshooting

### Display Not Working
- Ensure `TDECK_POWER_ON` (GPIO 10) is set HIGH before display init
- Check backlight pin (GPIO 42) is HIGH
- Verify SPI pins match T-Deck hardware

### LoRa Not Receiving
- Verify antenna is connected
- Check frequency matches other units (915.0 MHz)
- Confirm sync word matches (0x12)
- Ensure adequate separation from other 915 MHz devices

### Serial Not Appearing
- Use `/dev/ttyACM0` (not ttyUSB0)
- Enter bootloader mode (BOOT + RST) if needed
- Check USB cable has data lines (not charge-only)

### Battery Reading Incorrect
- GPIO 4 has 2:1 voltage divider
- Multiply factor may need adjustment for your specific board

## Power Consumption

| Mode | Current | Notes |
|------|---------|-------|
| Active (display on) | ~80 mA | Listening for packets |
| Deep Sleep | ~25-150 uA | Display off, radio sleeping |
| Average (8s sleep/3s active) | ~8 mA | Estimated |

With a 2000mAh battery in deep sleep mode: ~10-14 days estimated runtime.
