# LoRa River Monitoring Network Setup Guide

This guide explains how to set up the three-unit LoRa network for remote river level monitoring.

## System Overview

```
┌─────────────────┐         ┌─────────────────┐         ┌─────────────────┐
│   RIVER UNIT    │  LoRa   │  RIDGE RELAY    │  LoRa   │   HOME UNIT     │
│                 │ ──────> │                 │ ──────> │                 │
│ - Hydro Sensor  │  915MHz │ - Battery Power │  915MHz │ - OLED Display  │
│ - Moisture Sen  │         │ - Deep Sleep    │         │ - Serial Log    │
│ - LoRa TX       │         │ - Auto Relay    │         │ - LoRa RX       │
└─────────────────┘         └─────────────────┘         └─────────────────┘
     At River                  On Ridge                    At Home
```

## Hardware Requirements

**All three units use the same board:**
- 3x Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262)

**River Unit additional hardware:**
- INA219 Current Sensor Module (optional, for water level)
- ALS-MPM-2F Hydrostatic Sensor (optional, for water level)
- LM393 Soil Moisture Sensor
- 19.5V Dell Power Supply (if using hydrostatic sensor)
- Weatherproof enclosure

**Ridge Relay additional hardware:**
- LiPo Battery (3.7V, 2000-6000 mAh recommended)
- Solar panel + charge controller (optional, for extended operation)
- Weatherproof enclosure

**Home Unit additional hardware:**
- USB power supply or computer connection

## Required Libraries

Install these libraries before compiling any of the sketches:

```bash
arduino-cli lib install "RadioLib"
arduino-cli lib install "Adafruit INA219"
arduino-cli lib install "Adafruit SSD1306"
```

Or via Arduino IDE: Sketch → Include Library → Manage Libraries

## Directory Structure

```
ESP32_HYDRO_STATIC/
├── lora_config.h          # Shared LoRa settings (MUST match all units!)
├── river_unit/
│   ├── river_unit.ino     # River sensor + LoRa transmitter
│   └── lora_config.h      # Copy of shared config
├── ridge_relay/
│   ├── ridge_relay.ino    # Battery-powered LoRa repeater
│   └── lora_config.h      # Copy of shared config
├── home_unit/
│   ├── home_unit.ino      # LoRa receiver + display
│   └── lora_config.h      # Copy of shared config
└── ESP32_HYDRO_STATIC.ino # Original standalone sketch (no LoRa)
```

## Compilation & Upload

### River Unit

```bash
cd /home/mdchansl/IOT/ESP32_HYDRO_STATIC/river_unit

# Compile
arduino-cli compile --build-path ./build --fqbn esp32:esp32:heltec_wifi_lora_32_V3 .

# Upload (connect river unit via USB)
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 --input-dir ./build .

# Monitor
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### Ridge Relay

```bash
cd /home/mdchansl/IOT/ESP32_HYDRO_STATIC/ridge_relay

# Compile
arduino-cli compile --build-path ./build --fqbn esp32:esp32:heltec_wifi_lora_32_V3 .

# Upload (connect ridge unit via USB)
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 --input-dir ./build .

# Monitor (optional - for testing)
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### Home Unit

```bash
cd /home/mdchansl/IOT/ESP32_HYDRO_STATIC/home_unit

# Compile
arduino-cli compile --build-path ./build --fqbn esp32:esp32:heltec_wifi_lora_32_V3 .

# Upload (connect home unit via USB)
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 --input-dir ./build .

# Monitor
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## Configuration

### LoRa Settings (lora_config.h)

**CRITICAL: These settings MUST be identical on all three units!**

```cpp
#define LORA_FREQUENCY      915.0    // MHz (US ISM band)
#define LORA_BANDWIDTH      125.0    // kHz
#define LORA_SPREADING      9        // SF7-12 (higher = longer range, slower)
#define LORA_CODING_RATE    7        // 5-8 (higher = more error correction)
#define LORA_SYNC_WORD      0x12     // Private network ID
#define LORA_TX_POWER       14       // dBm (14 is good for <1km)
```

### Adjusting for Different Ranges

**For longer range (1-5 km):**
```cpp
#define LORA_SPREADING      10       // Increase spreading factor
#define LORA_TX_POWER       20       // Increase power
```

**For maximum range (5-10 km):**
```cpp
#define LORA_SPREADING      12       // Maximum spreading factor
#define LORA_TX_POWER       22       // Maximum power (legal limit)
#define LORA_BANDWIDTH      62.5     // Narrower bandwidth
```

**For better battery life (shorter range ok):**
```cpp
#define LORA_SPREADING      7        // Minimum spreading factor
#define LORA_TX_POWER       10       // Lower power
```

### Timing Settings

In `lora_config.h`:

```cpp
// River Unit transmission interval
#define TX_INTERVAL_MS      10000    // 10 seconds

// Ridge Relay sleep/wake cycle
#define RELAY_SLEEP_SEC     8        // Sleep 8 seconds between checks
#define RELAY_LISTEN_MS     3000     // Listen for 3 seconds each wake

// Home Unit connection timeout
#define RX_TIMEOUT_MS       60000    // 60 seconds = connection lost
```

**Note:** `RELAY_SLEEP_SEC` must be less than `TX_INTERVAL_MS / 1000` to ensure the relay catches transmissions.

## River Unit Wiring

Same as the original standalone unit:

```
INA219 (Optional)           Heltec V3
──────────────              ─────────
VCC      ───────────────>   3V3
GND      ───────────────>   GND
SDA      ───────────────>   GPIO 1
SCL      ───────────────>   GPIO 2

LM393 Moisture              Heltec V3
──────────────              ─────────
VCC      ───────────────>   3V3
GND      ───────────────>   GND
AO       ───────────────>   GPIO 4

Current Loop (if using hydrostatic sensor)
──────────────────────────────────────────
Dell +19.5V  ──> INA219 VIN+ ──> Sensor RED
Sensor BLACK ──> Dell GND
All grounds common
```

## Ridge Relay Wiring

**Minimal wiring - just battery power:**

```
LiPo Battery (3.7V)         Heltec V3
───────────────             ─────────
+          ─────────────>   BAT+ (JST connector)
-          ─────────────>   BAT- (JST connector)
```

The Heltec V3 has a built-in battery charging circuit. Connect via the JST connector on the board.

**Optional: Solar charging**
- Connect a 5-6V solar panel to the USB-C port or a dedicated solar charge controller to the battery.

## Testing the Network

### Step 1: Test River Unit (alone)

1. Upload river_unit.ino to first Heltec
2. Connect sensors
3. Monitor serial output - verify sensor readings and "TX:OK" messages

### Step 2: Test Home Unit (alone)

1. Upload home_unit.ino to second Heltec
2. Monitor serial output - should show "Waiting for packets..."
3. Place near river unit - should receive direct packets

### Step 3: Add Ridge Relay

1. Upload ridge_relay.ino to third Heltec
2. Connect battery
3. Place between river and home units
4. Home unit should now receive relayed packets with both RSSI values

### Step 4: Deploy

1. Install river unit at river location
2. Install ridge relay at ridge/hilltop with clear line of sight to both
3. Verify home unit receives data
4. Monitor signal strength (RSSI) to ensure good links

## Expected Serial Output

### River Unit
```
River Unit - Hydrostatic Sensor + LoRa TX
==========================================
Board: Heltec WiFi LoRa 32 V3
I2C: OLED on GPIO17/18, INA219 on GPIO1/2
INA219 initialized successfully
OLED display initialized
Moisture sensor on GPIO4
Initializing LoRa SX1262... SUCCESS!
  Frequency: 915 MHz
  Bandwidth: 125 kHz
  SF: 9, CR: 4/7
  TX Power: 14 dBm

Starting measurements...
==========================================

--- Sensor Reading ---
Current: 8.45 mA
Depth: 10.9 in (27.8%)
Moisture: 42.5% (raw: 2650)
TX Packet #0 - Current: 8.45 mA, Moisture: 42.5% ... OK
```

### Ridge Relay
```
Ridge Relay Unit - LoRa Repeater
=================================
Boot #15 - Packets relayed: 14
Wake reason: Timer
Initializing LoRa... OK
Listening for packets...
Packet received!
  Seq #23, Current: 8.45 mA, Moisture: 42.5%
  RSSI: -67 dBm, SNR: 9.5 dB
  Relaying... OK
Going to deep sleep...
```

### Home Unit
```
Home Unit - LoRa Receiver
=========================
Board: Heltec WiFi LoRa 32 V3
OLED display initialized
Initializing LoRa... OK
  Frequency: 915 MHz
  Listening for packets from ridge relay...

Listening for packets...
========================

========== RIVER DATA RECEIVED ==========
Packet #23 (15 total)
Via: Ridge Relay
River->Ridge RSSI: -67 dBm
Ridge->Home RSSI: -52 dBm, SNR: 10.2 dB

--- Sensor Data ---
Current: 8.45 mA
Depth: 10.9 in (27.8%)
Moisture: 42.5%
River Battery: 100%
=========================================
```

## Important: Vext Power for OLED

Some Heltec V3 boards require enabling Vext power (GPIO 36) for the OLED to work. This is already included in all three sketches:

```cpp
#define VEXT_CTRL 36  // Vext power control for OLED

// In setup():
pinMode(VEXT_CTRL, OUTPUT);
digitalWrite(VEXT_CTRL, LOW);  // LOW = ON for Vext
delay(100);
```

If your OLED display is blank but LoRa works, this is likely the issue.

## Ridge Relay Test Mode

The ridge relay has a `TEST_MODE` flag at the top of the sketch:

```cpp
#define TEST_MODE true   // Display stays on, no deep sleep
#define TEST_MODE false  // Normal battery-saving operation
```

**For testing:** Set to `true` - display stays on continuously, easier to debug.

**For deployment:** Set to `false` - enables deep sleep for battery savings.

After changing, recompile and upload.

## Troubleshooting

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| OLED display blank | Vext power not enabled | Verify GPIO 36 is set LOW before OLED init |
| "LoRa init failed" | Wrong board selected | Use `heltec_wifi_lora_32_V3` FQBN |
| No packets received | Frequency mismatch | Verify `LORA_FREQUENCY` matches on all units |
| No packets received | Sync word mismatch | Verify `LORA_SYNC_WORD` matches on all units |
| Intermittent reception | Ridge relay sleeping | Decrease `RELAY_SLEEP_SEC` |
| Weak signal | Too far apart | Increase `LORA_SPREADING` and `LORA_TX_POWER` |
| Ridge relay battery dies fast | Sleep time too short | Increase `RELAY_SLEEP_SEC` |
| "CONNECTION LOST" on home | River/Ridge unit offline | Check power at remote units |
| Checksum errors | Interference or weak signal | Increase spreading factor |
| Display corrupt/garbled | Blocking LoRa receive | Ensure using interrupt-driven receive in test mode |

## Battery Life Estimates (Ridge Relay)

Based on 8-second sleep cycle with 3-second active window:

| Battery Capacity | Estimated Life |
|-----------------|----------------|
| 1000 mAh | ~8 days |
| 2000 mAh | ~16 days |
| 4000 mAh | ~32 days |
| 6000 mAh | ~48 days |

To extend battery life:
- Increase `RELAY_SLEEP_SEC` (but may miss packets)
- Decrease `RELAY_LISTEN_MS`
- Lower `LORA_TX_POWER` if signal is strong
- Add solar charging

## Frequency Selection (US)

The US ISM band allows 902-928 MHz. The default 915.0 MHz works well, but you can change if you experience interference:

```cpp
#define LORA_FREQUENCY      903.9    // Alternative frequency
// or
#define LORA_FREQUENCY      918.3    // Another alternative
```

**Note:** All three units must use the same frequency!

## Future Enhancements

Possible additions:
- **Acknowledgment packets** - River unit confirms relay received
- **Bidirectional control** - Send commands from home to river
- **Multiple sensors** - Add more sensor nodes
- **LoRaWAN integration** - Connect to The Things Network
- **Web dashboard** - ESP32 WiFi at home unit for remote monitoring
- **Alerts** - Email/SMS notifications for high water levels
