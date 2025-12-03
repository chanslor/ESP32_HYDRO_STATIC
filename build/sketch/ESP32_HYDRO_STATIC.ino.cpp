#include <Arduino.h>
#line 1 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ESP32_HYDRO_STATIC.ino"
/*
 * ESP32 Hydrostatic Water Level Sensor with OLED Display
 *
 * Board: Heltec WiFi LoRa 32 (V2 or V3)
 *
 * Hardware:
 * - ALS-MPM-2F Hydrostatic Sensor (4-20 mA output)
 * - INA219 Current Sensor Module
 * - LM393 Soil Moisture Sensor (for rain detection)
 * - Heltec WiFi LoRa 32 (built-in OLED display)
 * - 19.5V Dell Power Supply
 *
 * Connections:
 * INA219 -> Heltec WiFi LoRa 32
 *   VCC  -> 3V3
 *   GND  -> GND
 *   SDA  -> GPIO 4  (V2) or GPIO 1 (V3)
 *   SCL  -> GPIO 15 (V2) or GPIO 2 (V3)
 *
 * LM393 Soil Moisture Sensor -> Heltec WiFi LoRa 32
 *   VCC  -> 3V3
 *   GND  -> GND
 *   AO   -> GPIO 36 (V2) or GPIO 4 (V3) - Analog output
 *
 * Built-in OLED Display (SSD1306, 128x64):
 *   Heltec V2: SDA=4, SCL=15, RST=16
 *   Heltec V3: SDA=17, SCL=18, RST=21
 *
 * Current Loop:
 *   Dell +19.5V -> INA219 VIN+ -> Sensor RED (+V)
 *   Sensor BLACK (return) -> Dell GND
 *   INA219 VIN- -> Sensor RED
 *
 * The sensor outputs 4-20 mA proportional to water depth:
 *   4 mA  = Empty tank (0 depth)
 *   20 mA = Full tank (max depth)
 */

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== BOARD VERSION SELECTION =====
// Uncomment ONE of these based on your Heltec board version:
// #define HELTEC_V2     // Older version
#define HELTEC_V3        // V3 board (newer version)

// Pin definitions based on board version
#ifdef HELTEC_V2
  #define OLED_SDA 4
  #define OLED_SCL 15
  #define OLED_RST 16
  #define INA219_SDA 4   // V2: Share I2C bus with OLED
  #define INA219_SCL 15
#elif defined(HELTEC_V3)
  #define OLED_SDA 17    // V3: Internal to board, not on headers
  #define OLED_SCL 18    // V3: Internal to board, not on headers
  #define OLED_RST 21
  #define INA219_SDA 1   // V3: Use GPIO1 (Header J3, position 12)
  #define INA219_SCL 2   // V3: Use GPIO2 (Header J3, position 13)
#else
  #error "Please define either HELTEC_V2 or HELTEC_V3"
#endif

// Soil Moisture Sensor pin (LM393 analog output)
#ifdef HELTEC_V2
  #define MOISTURE_SENSOR_PIN 36  // V2: Use GPIO36 (ADC1_CH0, input only)
#elif defined(HELTEC_V3)
  #define MOISTURE_SENSOR_PIN 4   // V3: Use GPIO4 (ADC1)
#endif

// OLED display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C  // I2C address for Heltec built-in OLED

// Create device instances
#ifdef HELTEC_V3
  TwoWire I2C_INA219 = TwoWire(1);  // V3: Use second I2C bus for INA219
#endif

Adafruit_INA219 ina219;  // Will be initialized with appropriate bus in setup()
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// Flag to track if INA219 is available
bool ina219Available = false;

// Sensor calibration parameters
// Adjust these based on your tank specifications
const float MIN_CURRENT_MA = 4.0;      // Minimum current (mA) at 0 depth
const float MAX_CURRENT_MA = 20.0;     // Maximum current (mA) at max depth
const float MAX_DEPTH_CM = 100.0;      // Maximum tank depth in centimeters
                                       // Change this to match your tank

// Unit conversion constants
const float CM_TO_INCHES = 0.393701;   // Conversion factor from cm to inches

// Measurement settings
const int SAMPLE_COUNT = 10;           // Number of samples to average
const unsigned long SAMPLE_DELAY_MS = 100;  // Delay between samples

// Soil moisture sensor settings
// These values may need calibration for your specific sensor
// Dry air = high ADC value (~4095), Wet/water = low ADC value (~0)
const int MOISTURE_DRY_VALUE = 4095;   // ADC reading when sensor is dry
const int MOISTURE_WET_VALUE = 1500;   // ADC reading when sensor is in water
const int MOISTURE_SAMPLE_COUNT = 5;   // Number of samples to average

#line 110 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ESP32_HYDRO_STATIC.ino"
void setup();
#line 208 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ESP32_HYDRO_STATIC.ino"
void loop();
#line 281 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ESP32_HYDRO_STATIC.ino"
float getAverageCurrent();
#line 307 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ESP32_HYDRO_STATIC.ino"
float calculateDepth(float current_mA);
#line 322 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ESP32_HYDRO_STATIC.ino"
float calculatePercentage(float current_mA);
#line 336 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ESP32_HYDRO_STATIC.ino"
int getAverageMoisture();
#line 353 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ESP32_HYDRO_STATIC.ino"
float calculateMoisturePercent(int rawValue);
#line 369 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ESP32_HYDRO_STATIC.ino"
void updateOLEDDisplay(float current_mA, float depthInches, float percentage, float moisturePercent, bool hasWaterLevel);
#line 110 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ESP32_HYDRO_STATIC.ino"
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Heltec WiFi LoRa 32 - Hydrostatic Water Level Sensor");
  Serial.println("====================================================");

  #ifdef HELTEC_V2
    Serial.println("Board: Heltec WiFi LoRa 32 V2");
  #else
    Serial.println("Board: Heltec WiFi LoRa 32 V3");
  #endif

  // Initialize I2C buses
  #ifdef HELTEC_V3
    // V3: Separate I2C buses
    Wire.begin(OLED_SDA, OLED_SCL);           // OLED on Wire (GPIO17/18 internal)
    I2C_INA219.begin(INA219_SDA, INA219_SCL); // INA219 on Wire1 (GPIO1/2)
    Serial.println("I2C: OLED on GPIO17/18, INA219 on GPIO1/2");
  #else
    // V2: Shared I2C bus
    Wire.begin(OLED_SDA, OLED_SCL);           // Both on same bus
    Serial.println("I2C: Shared bus on GPIO4/15");
  #endif

  // Initialize OLED reset pin (required for Heltec boards)
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  // Initialize the INA219 (optional - continue if not found)
  #ifdef HELTEC_V3
    if (!ina219.begin(&I2C_INA219)) {  // V3: Use second I2C bus
  #else
    if (!ina219.begin()) {             // V2: Use default Wire bus
  #endif
    Serial.println("Failed to find INA219 chip - water level disabled");
    #ifdef HELTEC_V3
      Serial.println("Check wiring to GPIO1 (SDA) and GPIO2 (SCL)");
    #else
      Serial.println("Check wiring to GPIO4 (SDA) and GPIO15 (SCL)");
    #endif
    ina219Available = false;
  } else {
    Serial.println("INA219 initialized successfully");
    ina219Available = true;
  }

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Failed to find SSD1306 OLED!");
    Serial.println("Check OLED wiring and I2C address.");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("Built-in OLED display initialized successfully");

  // Initialize soil moisture sensor pin
  pinMode(MOISTURE_SENSOR_PIN, INPUT);
  #ifdef HELTEC_V3
    // ESP32-S3 uses 12-bit ADC by default
    analogReadResolution(12);
    Serial.print("Soil moisture sensor on GPIO");
    Serial.println(MOISTURE_SENSOR_PIN);
  #else
    Serial.print("Soil moisture sensor on GPIO");
    Serial.println(MOISTURE_SENSOR_PIN);
  #endif

  // Display startup screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Water Level");
  display.println("Sensor");
  display.println();
  display.println("Initializing...");
  display.display();
  delay(2000);
  Serial.println();
  Serial.print("Tank depth range: 0 - ");
  Serial.print(MAX_DEPTH_CM);
  Serial.println(" cm");
  Serial.print("Current range: ");
  Serial.print(MIN_CURRENT_MA);
  Serial.print(" - ");
  Serial.print(MAX_CURRENT_MA);
  Serial.println(" mA");
  Serial.println();
  Serial.println("Starting measurements...");
  Serial.println("=====================================");
  Serial.println();
}

void loop() {
  float avgCurrent = 0.0;
  float depthCm = 0.0;
  float depthInches = 0.0;
  float depthPercent = 0.0;

  // Read INA219 if available
  if (ina219Available) {
    avgCurrent = getAverageCurrent();
    depthCm = calculateDepth(avgCurrent);
    depthInches = depthCm * CM_TO_INCHES;
    depthPercent = calculatePercentage(avgCurrent);
  }

  // Read soil moisture sensor
  int moistureRaw = getAverageMoisture();
  float moisturePercent = calculateMoisturePercent(moistureRaw);

  // Display results
  Serial.println("--- Sensor Reading ---");

  if (ina219Available) {
    // Calculate feet and inches
    int feet = (int)(depthInches / 12);
    float remainingInches = depthInches - (feet * 12);

    Serial.print("Current: ");
    Serial.print(avgCurrent, 2);
    Serial.println(" mA");

    Serial.print("Depth: ");
    if (depthInches >= 12.0) {
      Serial.print(feet);
      Serial.print(" ft ");
      Serial.print(remainingInches, 1);
      Serial.print(" in (");
    } else {
      Serial.print(depthInches, 1);
      Serial.print(" in (");
    }
    Serial.print(depthPercent, 1);
    Serial.println("%)");

    // Check for sensor faults
    if (avgCurrent < MIN_CURRENT_MA - 0.5) {
      Serial.println("WARNING: Current below 4mA - Check sensor connection!");
    } else if (avgCurrent > MAX_CURRENT_MA + 0.5) {
      Serial.println("WARNING: Current above 20mA - Check sensor!");
    }
  } else {
    Serial.println("Water level: N/A (INA219 not connected)");
  }

  // Display moisture reading
  Serial.print("Moisture: ");
  Serial.print(moisturePercent, 1);
  Serial.print("% (raw: ");
  Serial.print(moistureRaw);
  Serial.println(")");

  Serial.println();

  // Update OLED display
  updateOLEDDisplay(avgCurrent, depthInches, depthPercent, moisturePercent, ina219Available);

  // Wait before next reading
  delay(2000);
}

/**
 * Read multiple current samples and return the average
 * This reduces noise and provides more stable readings
 */
float getAverageCurrent() {
  float total = 0.0;

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    float current_mA = ina219.getCurrent_mA();

    // INA219 may return negative values depending on current direction
    // Take absolute value to ensure positive reading
    current_mA = abs(current_mA);

    total += current_mA;

    if (i < SAMPLE_COUNT - 1) {
      delay(SAMPLE_DELAY_MS);
    }
  }

  return total / SAMPLE_COUNT;
}

/**
 * Convert current (mA) to water depth (cm)
 * Uses linear interpolation:
 *   4 mA  -> 0 cm
 *   20 mA -> MAX_DEPTH_CM
 */
float calculateDepth(float current_mA) {
  // Constrain current to valid range
  current_mA = constrain(current_mA, MIN_CURRENT_MA, MAX_CURRENT_MA);

  // Linear mapping from current to depth
  float depth = ((current_mA - MIN_CURRENT_MA) / (MAX_CURRENT_MA - MIN_CURRENT_MA)) * MAX_DEPTH_CM;

  return depth;
}

/**
 * Convert current (mA) to percentage of tank capacity
 *   4 mA  -> 0%
 *   20 mA -> 100%
 */
float calculatePercentage(float current_mA) {
  // Constrain current to valid range
  current_mA = constrain(current_mA, MIN_CURRENT_MA, MAX_CURRENT_MA);

  // Linear mapping from current to percentage
  float percentage = ((current_mA - MIN_CURRENT_MA) / (MAX_CURRENT_MA - MIN_CURRENT_MA)) * 100.0;

  return percentage;
}

/**
 * Read multiple soil moisture samples and return the average
 * This reduces noise from the analog sensor
 */
int getAverageMoisture() {
  long total = 0;

  for (int i = 0; i < MOISTURE_SAMPLE_COUNT; i++) {
    total += analogRead(MOISTURE_SENSOR_PIN);
    delay(10);
  }

  return total / MOISTURE_SAMPLE_COUNT;
}

/**
 * Convert raw ADC moisture reading to percentage
 * Note: LM393 sensor reads HIGH when dry, LOW when wet
 *   Dry (air)  -> ~4095 ADC -> 0% moisture
 *   Wet (water) -> ~1500 ADC -> 100% moisture
 */
float calculateMoisturePercent(int rawValue) {
  // Constrain to expected range
  rawValue = constrain(rawValue, MOISTURE_WET_VALUE, MOISTURE_DRY_VALUE);

  // Invert the mapping (high ADC = dry = 0%, low ADC = wet = 100%)
  float percentage = ((float)(MOISTURE_DRY_VALUE - rawValue) /
                      (float)(MOISTURE_DRY_VALUE - MOISTURE_WET_VALUE)) * 100.0;

  return percentage;
}

/**
 * Update OLED display with current water level and moisture readings
 * Displays depth, tank percentage, and moisture percentage
 * If INA219 is not available, shows moisture-only display
 */
void updateOLEDDisplay(float current_mA, float depthInches, float percentage, float moisturePercent, bool hasWaterLevel) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (hasWaterLevel) {
    // Full display with water level and moisture
    // Title
    display.setCursor(0, 0);
    display.print("Tank:");
    display.print(percentage, 0);
    display.print("%  Moist:");
    display.print(moisturePercent, 0);
    display.println("%");
    display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

    // Current reading
    display.setCursor(0, 14);
    display.print("Current: ");
    display.print(current_mA, 2);
    display.println(" mA");

    // Depth reading (larger text)
    display.setCursor(0, 26);
    display.print("Depth:");
    display.setTextSize(2);
    display.setCursor(0, 36);

    // Display in feet and inches if >= 12 inches, otherwise just inches
    if (depthInches >= 12.0) {
      int feet = (int)(depthInches / 12);
      float remainingInches = depthInches - (feet * 12);
      display.print(feet);
      display.print("'");
      display.print(remainingInches, 1);
      display.print("\"");
    } else {
      display.print(depthInches, 1);
      display.print("\"");
    }

    // Moisture indicator at bottom
    display.setTextSize(1);
    display.setCursor(0, 56);
  } else {
    // Moisture-only display (INA219 not available)
    display.setCursor(0, 0);
    display.println("Moisture Sensor");
    display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

    display.setCursor(0, 16);
    display.println("Water Level: N/A");
    display.println("(INA219 not found)");

    // Large moisture display
    display.setCursor(0, 38);
    display.print("Moisture:");
    display.setTextSize(2);
    display.setCursor(0, 48);
    display.print(moisturePercent, 1);
    display.print("%");
    display.setTextSize(1);
  }

  if (hasWaterLevel) {
    // This continues the moisture line for the full display
    display.print("Moisture: ");
    display.print(moisturePercent, 1);
    display.print("%");
  }

  // Display everything
  display.display();
}

