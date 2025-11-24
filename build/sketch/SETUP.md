#line 1 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/SETUP.md"
# ESP32 Hydrostatic Water Level Sensor - Setup Guide

## Required Libraries

Install the following library through Arduino IDE Library Manager:

1. **Adafruit INA219** by Adafruit
   - Go to: Sketch → Include Library → Manage Libraries
   - Search for "Adafruit INA219"
   - Install the latest version
   - This will also install dependencies: Adafruit BusIO

## Configuration

Before uploading, adjust these parameters in the code to match your tank:

```cpp
const float MAX_DEPTH_CM = 100.0;  // Change to your tank's maximum depth in cm
```

For example:
- If your tank is 2 meters deep, set to `200.0`
- If your tank is 50 cm deep, set to `50.0`

## Upload Instructions

1. Connect ESP32 to your computer via USB
2. Select board: **Tools → Board → ESP32 Arduino → ESP32 Dev Module**
3. Select the correct COM port: **Tools → Port → [Your ESP32 port]**
4. Click **Upload**

## Serial Monitor Setup

1. Open Serial Monitor: **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. You should see readings every 2 seconds

## Expected Output

```
ESP32 Hydrostatic Water Level Sensor
=====================================
INA219 initialized successfully

Tank depth range: 0 - 100 cm
Current range: 4 - 20 mA

Starting measurements...
=====================================

--- Water Level Reading ---
Current: 12.05 mA
Depth: 50.3 cm (50.3%)

--- Water Level Reading ---
Current: 8.12 mA
Depth: 25.8 cm (25.8%)
```

## Calibration

The sensor uses a 4-20 mA current loop standard:
- **4 mA** = Empty tank (0% full)
- **20 mA** = Full tank (100% full)

If readings seem incorrect:

1. **Verify sensor is working:**
   - Current should be between 4-20 mA
   - Below 4 mA indicates a wiring problem
   - Above 20 mA indicates a sensor fault

2. **Check the MAX_DEPTH_CM setting:**
   - Measure your actual tank depth
   - Update the constant in the code

3. **Test with known water levels:**
   - Fill tank to a known depth
   - Compare sensor reading to actual measurement
   - If there's an offset, your sensor may need factory calibration

## Troubleshooting

### "Failed to find INA219 chip"
- Check I2C connections (SDA → GPIO21, SCL → GPIO22)
- Verify INA219 is powered (VCC → 3V3, GND → GND)
- Check if all grounds are common

### Current reads 0 mA
- Verify 19.5V supply is connected and powered on
- Check current loop wiring
- Ensure sensor is receiving power

### Current is negative
- The program takes absolute value automatically
- Negative values may indicate reversed VIN+/VIN- connections

### Readings are noisy/jumping
- Increase SAMPLE_COUNT (currently 10)
- Increase SAMPLE_DELAY_MS (currently 100)
- Check for loose connections

## Advanced Configuration

For more stable readings, adjust these parameters:

```cpp
const int SAMPLE_COUNT = 10;              // Increase for smoother readings (slower updates)
const unsigned long SAMPLE_DELAY_MS = 100; // Increase for better averaging
```

For faster updates:
```cpp
delay(2000);  // In loop() function - reduce for faster readings
```
