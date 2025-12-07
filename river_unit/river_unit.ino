/*
 * River Unit - Hydrostatic Water Level Sensor with LoRa Transmitter
 *
 * Board: Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262)
 *
 * This unit reads sensors and transmits data via LoRa to the ridge relay.
 *
 * Hardware:
 * - ALS-MPM-2F Hydrostatic Sensor (4-20 mA output) - optional
 * - INA219 Current Sensor Module
 * - LM393 Soil Moisture Sensor
 * - Heltec WiFi LoRa 32 V3 (built-in OLED and LoRa)
 * - 19.5V Dell Power Supply (for hydrostatic sensor)
 *
 * Connections:
 * - INA219: SDA=GPIO1, SCL=GPIO2 (separate I2C bus)
 * - LM393: AO=GPIO4
 * - Built-in OLED: SDA=GPIO17, SCL=GPIO18 (internal)
 * - LoRa SX1262: Uses internal SPI (GPIO 8,9,10,11,12,13,14)
 */

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RadioLib.h>
#include "lora_config.h"

// Board version - River unit uses V3
#define HELTEC_V3

// Pin definitions for V3
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define VEXT_CTRL 36  // Vext power control for OLED
#define INA219_SDA 1
#define INA219_SCL 2
#define MOISTURE_SENSOR_PIN 4

// OLED display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

// Create I2C bus for INA219
TwoWire I2C_INA219 = TwoWire(1);

// Create device instances
Adafruit_INA219 ina219;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// LoRa radio instance
SPIClass loraSPI(HSPI);
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY, loraSPI);

// Flag to track if INA219 is available
bool ina219Available = false;

// LoRa status
bool loraInitialized = false;
uint8_t packetSequence = 0;

// Sensor calibration parameters
const float MIN_CURRENT_MA = 4.0;
const float MAX_CURRENT_MA = 20.0;
const float MAX_DEPTH_CM = 100.0;
const float CM_TO_INCHES = 0.393701;

// Measurement settings
const int SAMPLE_COUNT = 10;
const unsigned long SAMPLE_DELAY_MS = 100;

// Soil moisture sensor settings
const int MOISTURE_DRY_VALUE = 4095;
const int MOISTURE_WET_VALUE = 1500;
const int MOISTURE_SAMPLE_COUNT = 5;
const unsigned long MOISTURE_READ_INTERVAL_MS = 10000;

// Moisture timing
unsigned long lastMoistureReadTime = 0;
int lastMoistureRaw = 0;
float lastMoisturePercent = 0.0;

// Transmission timing
unsigned long lastTxTime = 0;

// Function declarations
float getAverageCurrent();
float calculateDepth(float current_mA);
float calculatePercentage(float current_mA);
int getAverageMoisture();
float calculateMoisturePercent(int rawValue);
void updateOLEDDisplay(float current_mA, float depthInches, float percentage, float moisturePercent, bool hasWaterLevel, bool loraTxOk);
bool initLoRa();
bool transmitSensorData(float current_mA, float moisturePercent);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("River Unit - Hydrostatic Sensor + LoRa TX");
  Serial.println("==========================================");
  Serial.println("Board: Heltec WiFi LoRa 32 V3");

  // Enable Vext power for OLED
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, LOW);  // LOW = ON for Vext
  delay(100);

  // Initialize I2C buses
  Wire.begin(OLED_SDA, OLED_SCL);
  I2C_INA219.begin(INA219_SDA, INA219_SCL);
  Serial.println("I2C: OLED on GPIO17/18, INA219 on GPIO1/2");

  // Initialize OLED reset pin
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  // Initialize the INA219
  if (!ina219.begin(&I2C_INA219)) {
    Serial.println("INA219 not found - water level disabled");
    ina219Available = false;
  } else {
    Serial.println("INA219 initialized successfully");
    ina219Available = true;
  }

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED display failed!");
    while (1) delay(1000);
  }
  Serial.println("OLED display initialized");

  // Initialize moisture sensor pin
  pinMode(MOISTURE_SENSOR_PIN, INPUT);
  analogReadResolution(12);
  Serial.println("Moisture sensor on GPIO4");

  // Initialize LoRa
  loraInitialized = initLoRa();

  // Display startup screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("RIVER UNIT");
  display.println("LoRa Sensor TX");
  display.println();
  if (loraInitialized) {
    display.println("LoRa: OK");
    display.print("Freq: ");
    display.print(LORA_FREQUENCY, 1);
    display.println(" MHz");
  } else {
    display.println("LoRa: FAILED!");
  }
  display.display();
  delay(2000);

  Serial.println();
  Serial.println("Starting measurements...");
  Serial.println("==========================================");
}

bool initLoRa() {
  Serial.print("Initializing LoRa SX1262... ");

  // Initialize SPI for LoRa
  loraSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);

  // Initialize radio with all parameters
  int state = radio.begin(
    LORA_FREQUENCY,
    LORA_BANDWIDTH,
    LORA_SPREADING,
    LORA_CODING_RATE,
    LORA_SYNC_WORD,
    LORA_TX_POWER,
    LORA_PREAMBLE,
    LORA_TCXO_VOLTAGE,
    false  // Use DC-DC regulator
  );

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("FAILED! Error code: ");
    Serial.println(state);
    return false;
  }

  Serial.println("SUCCESS!");
  Serial.print("  Frequency: ");
  Serial.print(LORA_FREQUENCY);
  Serial.println(" MHz");
  Serial.print("  Bandwidth: ");
  Serial.print(LORA_BANDWIDTH);
  Serial.println(" kHz");
  Serial.print("  SF: ");
  Serial.print(LORA_SPREADING);
  Serial.print(", CR: 4/");
  Serial.println(LORA_CODING_RATE);
  Serial.print("  TX Power: ");
  Serial.print(LORA_TX_POWER);
  Serial.println(" dBm");

  return true;
}

bool transmitSensorData(float current_mA, float moisturePercent) {
  if (!loraInitialized) return false;

  // Build packet
  SensorPacket pkt;
  pkt.msgType = MSG_TYPE_SENSOR;
  pkt.sourceId = UNIT_ID_RIVER;
  pkt.relayId = 0;  // Direct transmission (not relayed yet)
  pkt.sequence = packetSequence++;
  pkt.current_mA = current_mA;
  pkt.moisturePercent = moisturePercent;
  pkt.rssi = 0;  // Will be filled by relay
  pkt.batteryPercent = 100;  // TODO: Read actual battery level
  pkt.checksum = calculateChecksum(&pkt);

  Serial.print("TX Packet #");
  Serial.print(pkt.sequence);
  Serial.print(" - Current: ");
  Serial.print(current_mA, 2);
  Serial.print(" mA, Moisture: ");
  Serial.print(moisturePercent, 1);
  Serial.print("% ... ");

  // Transmit
  int state = radio.transmit((uint8_t*)&pkt, sizeof(SensorPacket));

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("OK");
    return true;
  } else {
    Serial.print("FAILED! Error: ");
    Serial.println(state);
    return false;
  }
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

  // Read soil moisture sensor (only every MOISTURE_READ_INTERVAL_MS)
  unsigned long currentTime = millis();
  if (currentTime - lastMoistureReadTime >= MOISTURE_READ_INTERVAL_MS || lastMoistureReadTime == 0) {
    lastMoistureRaw = getAverageMoisture();
    lastMoisturePercent = calculateMoisturePercent(lastMoistureRaw);
    lastMoistureReadTime = currentTime;
  }
  int moistureRaw = lastMoistureRaw;
  float moisturePercent = lastMoisturePercent;

  // Display results to Serial
  Serial.println("--- Sensor Reading ---");

  if (ina219Available) {
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

    if (avgCurrent < MIN_CURRENT_MA - 0.5) {
      Serial.println("WARNING: Current below 4mA!");
    } else if (avgCurrent > MAX_CURRENT_MA + 0.5) {
      Serial.println("WARNING: Current above 20mA!");
    }
  } else {
    Serial.println("Water level: N/A (INA219 not connected)");
  }

  Serial.print("Moisture: ");
  Serial.print(moisturePercent, 1);
  Serial.print("% (raw: ");
  Serial.print(moistureRaw);
  Serial.println(")");

  // Transmit via LoRa
  bool txSuccess = transmitSensorData(avgCurrent, moisturePercent);

  Serial.println();

  // Update OLED display
  updateOLEDDisplay(avgCurrent, depthInches, depthPercent, moisturePercent, ina219Available, txSuccess);

  // Wait before next reading
  delay(10000);
}

float getAverageCurrent() {
  float total = 0.0;

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    float current_mA = ina219.getCurrent_mA();
    current_mA = abs(current_mA);
    total += current_mA;

    if (i < SAMPLE_COUNT - 1) {
      delay(SAMPLE_DELAY_MS);
    }
  }

  return total / SAMPLE_COUNT;
}

float calculateDepth(float current_mA) {
  current_mA = constrain(current_mA, MIN_CURRENT_MA, MAX_CURRENT_MA);
  float depth = ((current_mA - MIN_CURRENT_MA) / (MAX_CURRENT_MA - MIN_CURRENT_MA)) * MAX_DEPTH_CM;
  return depth;
}

float calculatePercentage(float current_mA) {
  current_mA = constrain(current_mA, MIN_CURRENT_MA, MAX_CURRENT_MA);
  float percentage = ((current_mA - MIN_CURRENT_MA) / (MAX_CURRENT_MA - MIN_CURRENT_MA)) * 100.0;
  return percentage;
}

int getAverageMoisture() {
  long total = 0;

  for (int i = 0; i < MOISTURE_SAMPLE_COUNT; i++) {
    total += analogRead(MOISTURE_SENSOR_PIN);
    delay(10);
  }

  return total / MOISTURE_SAMPLE_COUNT;
}

float calculateMoisturePercent(int rawValue) {
  rawValue = constrain(rawValue, MOISTURE_WET_VALUE, MOISTURE_DRY_VALUE);
  float percentage = ((float)(MOISTURE_DRY_VALUE - rawValue) /
                      (float)(MOISTURE_DRY_VALUE - MOISTURE_WET_VALUE)) * 100.0;
  return percentage;
}

void updateOLEDDisplay(float current_mA, float depthInches, float percentage, float moisturePercent, bool hasWaterLevel, bool loraTxOk) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Header with LoRa status
  display.setCursor(0, 0);
  display.print("RIVER ");
  if (loraTxOk) {
    display.print("TX:OK");
  } else {
    display.print("TX:--");
  }
  display.print(" #");
  display.print(packetSequence - 1);
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

  if (hasWaterLevel) {
    // Current reading
    display.setCursor(0, 14);
    display.print("I:");
    display.print(current_mA, 1);
    display.print("mA ");
    display.print(percentage, 0);
    display.print("%");

    // Depth reading (larger text)
    display.setCursor(0, 26);
    display.print("Depth:");
    display.setTextSize(2);
    display.setCursor(0, 36);

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

    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print("Moist:");
    display.print(moisturePercent, 0);
    display.print("%");
  } else {
    // Moisture-only display
    display.setCursor(0, 16);
    display.println("Water Level: N/A");

    display.setCursor(0, 32);
    display.print("Moisture:");
    display.setTextSize(2);
    display.setCursor(0, 44);
    display.print(moisturePercent, 1);
    display.print("%");
    display.setTextSize(1);
  }

  display.display();
}
