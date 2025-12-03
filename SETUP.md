# Heltec WiFi LoRa 32 - Hydrostatic Water Level Sensor Setup Guide

## Hardware Overview

This system uses a **Heltec WiFi LoRa 32 V3** board with built-in OLED display to monitor water tank levels via a 4-20 mA hydrostatic pressure sensor and soil/rain moisture via an LM393 sensor.

**Components:**
- Heltec WiFi LoRa 32 V3 (ESP32-S3 with built-in OLED)
- LM393 Soil Moisture Sensor (for rain/moisture detection)
- INA219 Current Sensor Module (optional - for water level)
- ALS-MPM-2F Hydrostatic Sensor (4-20 mA) (optional - for water level)
- 19.5V Dell Power Supply (only needed for hydrostatic sensor)

**Note:** The INA219 and hydrostatic sensor are optional. If not connected, the system will continue to operate with moisture readings only.

## Required Libraries

Install these libraries through Arduino IDE Library Manager or arduino-cli:

1. **Adafruit INA219** - For reading current sensor
   - Dependencies: Adafruit BusIO (auto-installed)

2. **Adafruit SSD1306** - For built-in OLED display
   - Dependencies: Adafruit GFX Library (auto-installed)

### Installation Methods

**Arduino IDE:**
```
Sketch → Include Library → Manage Libraries
Search: "Adafruit INA219" → Install
Search: "Adafruit SSD1306" → Install
```

**arduino-cli:**
```bash
arduino-cli lib install "Adafruit INA219"
arduino-cli lib install "Adafruit SSD1306"
```

## About the INA219 Module

![INA219 Module](INA219-power-supply.png)

The **INA219** is the key component that allows the ESP32 to read the 4-20 mA signal from the industrial hydrostatic sensor.

**Why We Need It:**
- Industrial sensors output 4-20 mA current (not voltage)
- The ESP32 cannot directly measure current
- The INA219 measures the current and converts it to digital data via I2C
- High precision (±0.8 mA) ensures accurate depth readings

**How It Works:**
1. 4-20 mA current flows through the INA219's internal shunt resistor
2. The chip measures the tiny voltage drop across this resistor
3. It calculates the exact current value using Ohm's law
4. The ESP32 reads this value via I2C and converts it to water depth

Think of the INA219 as a "current-to-digital" converter - it's the essential bridge between the analog industrial sensor world and the digital microcontroller world.

## Wiring

### LM393 Soil Moisture Sensor to Heltec V3 (Header J3 - Left Side)

![LM393 Soil Moisture Sensor](images/soil_moisture_sensor.jpg)

```
LM393       Heltec V3
------      ---------
VCC   →     3V3 (Pin 2 or 3)
GND   →     GND (Pin 1)
AO    →     GPIO 4 (Pin 15) - Analog output
DO    →     (not connected)
```

### INA219 to Heltec V3 (Header J3 - Left Side) - Optional
```
INA219      Heltec V3
------      ---------
VCC   →     3V3 (Pin 2 or 3)
GND   →     GND (Pin 1)
SDA   →     GPIO 1 (Pin 12)
SCL   →     GPIO 2 (Pin 13)
```

### Current Loop (only if using hydrostatic sensor)
```
Dell PSU White (+19.5V) → INA219 VIN+
INA219 VIN-             → Sensor RED (+V)
Sensor BLACK            → Dell PSU Black (GND)
All grounds common
```

**Important**: The built-in OLED uses GPIO 17/18 internally - no external wiring needed!

## Configuration

### 1. Select Board Version

Edit `ESP32_HYDRO_STATIC.ino` lines 40-41:

```cpp
// For V3 (current default):
// #define HELTEC_V2
#define HELTEC_V3

// For V2 (if you have older board):
#define HELTEC_V2
// #define HELTEC_V3
```

### 2. Set Tank Depth

Edit the MAX_DEPTH_CM constant (line ~75):

```cpp
const float MAX_DEPTH_CM = 100.0;  // Your tank depth in centimeters
```

**Examples:**
- 2 meter tank: `200.0`
- 50 cm tank: `50.0`
- 6 foot tank: `182.88` (6 ft × 30.48 cm/ft)

**Note**: Enter depth in cm - the display will automatically show it in inches/feet.

## Upload Instructions

### Using Arduino IDE

1. Connect Heltec V3 via USB-C cable
2. Select Board: **Tools → Board → esp32 → Heltec WiFi LoRa 32(V3)**
3. Select Port: **Tools → Port → [Your COM/ttyUSB port]**
4. Click **Upload**
5. If upload fails, press and hold **PRG** button during upload

### Using arduino-cli

```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3 .

# Upload (adjust port as needed)
arduino-cli upload --fqbn esp32:esp32:heltec_wifi_lora_32_V3 --port /dev/ttyUSB0 .

# Monitor serial output
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

## Expected Output

### Serial Monitor (115200 baud) - With INA219
```
Heltec WiFi LoRa 32 - Hydrostatic Water Level Sensor
====================================================
Board: Heltec WiFi LoRa 32 V3
I2C: OLED on GPIO17/18, INA219 on GPIO1/2
INA219 initialized successfully
Built-in OLED display initialized successfully
Soil moisture sensor on GPIO4

Tank depth range: 0 - 100 cm
Current range: 4 - 20 mA

Starting measurements...
====================================================

--- Sensor Reading ---
Current: 7.46 mA
Depth: 8.5 in (21.6%)
Moisture: 45.2% (raw: 2850)
```

### Serial Monitor - Without INA219 (Moisture Only)
```
Heltec WiFi LoRa 32 - Hydrostatic Water Level Sensor
====================================================
Board: Heltec WiFi LoRa 32 V3
I2C: OLED on GPIO17/18, INA219 on GPIO1/2
Failed to find INA219 chip - water level disabled
Check wiring to GPIO1 (SDA) and GPIO2 (SCL)
Built-in OLED display initialized successfully
Soil moisture sensor on GPIO4

Starting measurements...
====================================================

--- Sensor Reading ---
Water level: N/A (INA219 not connected)
Moisture: 45.2% (raw: 2850)
```

### Built-in OLED Display

The 0.96" built-in OLED shows:
- **Header**: Tank % and Moisture % summary
- **Current**: Loop current in mA (if INA219 connected)
- **Depth**: Large text showing:
  - Under 12": "8.5 in"
  - 12" or more: "1 ft 7.8 in"
- **Moisture**: Moisture percentage at bottom

If INA219 is not connected, display shows moisture-only mode with large moisture reading.

Display updates every 2 seconds automatically.

## Calibration

The sensor uses the 4-20 mA current loop industrial standard:
- **4 mA** = Empty tank (0% full)
- **20 mA** = Full tank (100% full)

### Verification Steps

1. **Check current range**:
   - Current should be 4-20 mA
   - Below 4 mA: Wiring problem or disconnected sensor
   - Above 20 mA: Sensor fault

2. **Verify depth setting**:
   - Measure actual tank depth
   - Update MAX_DEPTH_CM to match

3. **Test with known levels**:
   - Fill tank to known depth
   - Compare sensor reading to actual measurement
   - Minor offsets (1-2%) are normal

## Troubleshooting

### "Failed to find INA219 chip"
**Cause**: I2C wiring issue or INA219 not connected
**Solution**:
- V3: Check GPIO 1 (SDA) and GPIO 2 (SCL) - NOT GPIO 17/18!
- Verify VCC → 3V3 and GND → GND
- Ensure all grounds are common
- **Note**: System will continue in moisture-only mode if INA219 is not found

### "Failed to find SSD1306 OLED"
**Cause**: Wrong board version selected
**Solution**: Verify HELTEC_V3 is defined in code (line 41)

### OLED blank or not working
**Cause**: Reset pin issue or wrong board selection
**Solution**:
- Verify board version in code matches your hardware
- Check OLED_RST is GPIO 21 for V3

### Upload fails
**Cause**: Board not in upload mode
**Solution**: Press and hold PRG button during upload

### Current = 0 mA
**Cause**: Power supply off or wiring issue
**Solution**:
- Turn on Dell power supply
- Check current loop wiring
- Verify INA219 VIN+/VIN- connections

### Readings are noisy
**Cause**: Electrical interference or loose connections
**Solution**:
- Increase SAMPLE_COUNT (line ~76): try 20 or 30
- Increase SAMPLE_DELAY_MS (line ~77): try 150 or 200
- Check all connections are secure
- Ensure proper grounding

### Wrong depth values
**Cause**: Incorrect MAX_DEPTH_CM setting
**Solution**: Measure actual tank depth in cm and update constant

### Display shows wrong units
**Note**: Display always shows inches/feet (code converts from cm internally)

### Moisture always 0%
**Cause**: Wrong GPIO or wiring issue
**Solution**: V3: Check AO connected to GPIO 4 (Pin 15). V2: Check AO connected to GPIO 36.

### Moisture always 100%
**Cause**: Probe may be shorted or submerged
**Solution**: Check probe connections, ensure probe is not in water during testing

### Moisture readings seem inverted
**Note**: This is normal. The LM393 outputs HIGH when dry and LOW when wet. The code handles this inversion automatically.

## Advanced Configuration

### Adjust Averaging (for stability)

Edit these values in the code:

```cpp
const int SAMPLE_COUNT = 10;              // Increase for smoother readings (10-30)
const unsigned long SAMPLE_DELAY_MS = 100; // Increase for better averaging (100-200)
```

**Trade-offs:**
- Higher values = smoother readings, slower response
- Lower values = faster response, more noise

### Change Update Rate

Edit the delay in loop() function:

```cpp
delay(2000);  // 2000 ms = 2 seconds between readings
```

### I2C Configuration (V3)

The V3 board uses two separate I2C buses:
- **Bus 1 (Wire)**: GPIO 17/18 for built-in OLED
- **Bus 2 (Wire1)**: GPIO 1/2 for INA219

This eliminates potential bus conflicts and improves reliability.

## Testing Without Water

To test the system without filling the tank:

1. Connect everything as documented
2. Power on the Dell PSU
3. Observe current reading (should be 4-20 mA)
4. Current reading depends on sensor submersion:
   - Out of water: ~4 mA
   - Partially submerged: 4-20 mA proportional to depth
   - Fully submerged: ~20 mA

## Safety Notes

- **19.5V** present in current loop - use caution
- **Water/electricity**: Keep ESP32 and INA219 dry
- **Sensor rating**: Verify max pressure rating matches tank depth
- **Weatherproofing**: Use appropriate enclosures for outdoor installations

## Need Help?

- See [README.md](README.md) for complete project documentation
- See [HELTEC_WIRING.md](HELTEC_WIRING.md) for detailed wiring diagrams
- Check GitHub issues for common problems and solutions
