#include <Arduino.h>
#line 1 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/home_unit/home_unit.ino"
/*
 * Home Unit - LoRa Receiver with OLED Display
 *
 * Board: Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262)
 *
 * This unit receives sensor data from the ridge relay and displays it.
 * Provides both OLED display and serial output for logging.
 *
 * Features:
 * - Receives relayed packets from ridge unit
 * - Displays water level, moisture, and signal quality
 * - Serial output for computer logging
 * - Connection status indicator
 *
 * Hardware:
 * - Heltec WiFi LoRa 32 V3 (built-in OLED and LoRa)
 * - USB connection to computer for power and serial logging
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RadioLib.h>
#include "lora_config.h"

// OLED pins for V3
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define VEXT_CTRL 36  // Vext power control for OLED

// OLED display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

// Sensor calibration (same as river unit for depth calculation)
const float MIN_CURRENT_MA = 4.0;
const float MAX_CURRENT_MA = 20.0;
const float MAX_DEPTH_CM = 100.0;
const float CM_TO_INCHES = 0.393701;

// Create device instances
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// LoRa radio instance
SPIClass loraSPI(HSPI);
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY, loraSPI);

// Status tracking
bool loraInitialized = false;
uint32_t packetsReceived = 0;
uint32_t packetErrors = 0;
unsigned long lastPacketTime = 0;
uint8_t lastSequence = 0;
bool connectionActive = false;

// Last received data
float lastCurrent = 0;
float lastMoisture = 0;
int lastRSSI_River = 0;   // RSSI at ridge (river->ridge link)
int lastRSSI_Home = 0;    // RSSI at home (ridge->home link)
float lastSNR = 0;
uint8_t lastBattery = 0;

// Interrupt flag for non-blocking receive
volatile bool receivedFlag = false;

// Interrupt handler
#line 70 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/home_unit/home_unit.ino"
void setFlag(void);
#line 82 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/home_unit/home_unit.ino"
void setup();
#line 192 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/home_unit/home_unit.ino"
void loop();
#line 70 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/home_unit/home_unit.ino"
void setFlag(void) {
  receivedFlag = true;
}

// Function declarations
bool initLoRa();
void processPacket();
float calculateDepth(float current_mA);
float calculatePercentage(float current_mA);
void updateDisplay();
void printSerialData(SensorPacket* pkt, int rssi, float snr);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Home Unit - LoRa Receiver");
  Serial.println("=========================");
  Serial.println("Board: Heltec WiFi LoRa 32 V3");

  // Enable Vext power for OLED
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, LOW);  // LOW = ON for Vext
  delay(100);

  // Initialize I2C for OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize OLED reset pin
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(50);
  digitalWrite(OLED_RST, HIGH);
  delay(100);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED display failed!");
    while (1) delay(1000);
  }
  Serial.println("OLED display initialized");

  // Show startup screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("HOME UNIT");
  display.println("LoRa Receiver");
  display.println();
  display.println("Initializing...");
  display.display();

  // Initialize LoRa
  loraInitialized = initLoRa();

  if (!loraInitialized) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("HOME UNIT");
    display.println();
    display.println("LoRa FAILED!");
    display.println("Check hardware");
    display.display();
    while (1) delay(1000);
  }

  // Set up interrupt-driven receive
  radio.setDio1Action(setFlag);

  // Start listening
  int state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("startReceive failed: ");
    Serial.println(state);
  }

  Serial.println();
  Serial.println("Listening for packets...");
  Serial.println("========================");
  Serial.println();

  // Show ready screen
  updateDisplay();
}

bool initLoRa() {
  Serial.print("Initializing LoRa... ");

  // Initialize SPI for LoRa
  loraSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);

  // Initialize radio
  int state = radio.begin(
    LORA_FREQUENCY,
    LORA_BANDWIDTH,
    LORA_SPREADING,
    LORA_CODING_RATE,
    LORA_SYNC_WORD,
    LORA_TX_POWER,
    LORA_PREAMBLE,
    LORA_TCXO_VOLTAGE,
    false
  );

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("FAILED! Error: ");
    Serial.println(state);
    return false;
  }

  Serial.println("OK");
  Serial.print("  Frequency: ");
  Serial.print(LORA_FREQUENCY);
  Serial.println(" MHz");
  Serial.print("  Listening for packets from ridge relay...");
  Serial.println();

  return true;
}

void loop() {
  // Check for received packet (interrupt-driven)
  if (receivedFlag) {
    receivedFlag = false;
    processPacket();

    // Restart receive mode
    radio.startReceive();
  }

  // Check connection status
  unsigned long now = millis();
  if (lastPacketTime > 0 && (now - lastPacketTime) > RX_TIMEOUT_MS) {
    if (connectionActive) {
      connectionActive = false;
      Serial.println("CONNECTION LOST - No data for 60 seconds");
      updateDisplay();
    }
  }

  // Small delay to prevent busy-looping
  delay(10);
}

void processPacket() {
  SensorPacket pkt;
  int state = radio.readData((uint8_t*)&pkt, sizeof(SensorPacket));

  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Read error: ");
    Serial.println(state);
    packetErrors++;
    return;
  }

  // Get signal quality
  int rssi = radio.getRSSI();
  float snr = radio.getSNR();

  // Validate checksum
  if (!validateChecksum(&pkt)) {
    Serial.println("Checksum error - packet discarded");
    packetErrors++;
    return;
  }

  // Accept both direct (MSG_TYPE_SENSOR) and relayed (MSG_TYPE_RELAY) packets
  if (pkt.msgType != MSG_TYPE_SENSOR && pkt.msgType != MSG_TYPE_RELAY) {
    Serial.print("Unknown message type: ");
    Serial.println(pkt.msgType);
    return;
  }

  // Check source
  if (pkt.sourceId != UNIT_ID_RIVER) {
    Serial.print("Unknown source: ");
    Serial.println(pkt.sourceId);
    return;
  }

  // Valid packet received!
  packetsReceived++;
  lastPacketTime = millis();
  connectionActive = true;

  // Check for missed packets
  uint8_t expectedSeq = lastSequence + 1;
  if (packetsReceived > 1 && pkt.sequence != expectedSeq) {
    int missed = (pkt.sequence - expectedSeq + 256) % 256;
    if (missed > 0 && missed < 128) {  // Forward gap
      Serial.print("Missed ");
      Serial.print(missed);
      Serial.println(" packet(s)");
    }
  }
  lastSequence = pkt.sequence;

  // Store data
  lastCurrent = pkt.current_mA;
  lastMoisture = pkt.moisturePercent;
  lastRSSI_River = pkt.rssi;  // RSSI at ridge (from river)
  lastRSSI_Home = rssi;        // RSSI here (from ridge)
  lastSNR = snr;
  lastBattery = pkt.batteryPercent;

  // Print to serial
  printSerialData(&pkt, rssi, snr);

  // Update display
  updateDisplay();
}

void printSerialData(SensorPacket* pkt, int rssi, float snr) {
  float depthCm = calculateDepth(pkt->current_mA);
  float depthInches = depthCm * CM_TO_INCHES;
  float depthPercent = calculatePercentage(pkt->current_mA);

  Serial.println("========== RIVER DATA RECEIVED ==========");
  Serial.print("Packet #");
  Serial.print(pkt->sequence);
  Serial.print(" (");
  Serial.print(packetsReceived);
  Serial.println(" total)");

  if (pkt->relayId == UNIT_ID_RIDGE) {
    Serial.println("Via: Ridge Relay");
    Serial.print("River->Ridge RSSI: ");
    Serial.print(pkt->rssi);
    Serial.println(" dBm");
  } else {
    Serial.println("Via: Direct (no relay)");
  }

  Serial.print("Ridge->Home RSSI: ");
  Serial.print(rssi);
  Serial.print(" dBm, SNR: ");
  Serial.print(snr);
  Serial.println(" dB");

  Serial.println();
  Serial.println("--- Sensor Data ---");
  Serial.print("Current: ");
  Serial.print(pkt->current_mA, 2);
  Serial.println(" mA");

  if (pkt->current_mA >= MIN_CURRENT_MA) {
    Serial.print("Depth: ");
    if (depthInches >= 12.0) {
      int feet = (int)(depthInches / 12);
      float remainingInches = depthInches - (feet * 12);
      Serial.print(feet);
      Serial.print(" ft ");
      Serial.print(remainingInches, 1);
      Serial.print(" in");
    } else {
      Serial.print(depthInches, 1);
      Serial.print(" in");
    }
    Serial.print(" (");
    Serial.print(depthPercent, 1);
    Serial.println("%)");
  } else {
    Serial.println("Depth: N/A (INA219 not connected)");
  }

  Serial.print("Moisture: ");
  Serial.print(pkt->moisturePercent, 1);
  Serial.println("%");

  Serial.print("River Battery: ");
  Serial.print(pkt->batteryPercent);
  Serial.println("%");

  Serial.println("=========================================");
  Serial.println();
}

float calculateDepth(float current_mA) {
  if (current_mA < MIN_CURRENT_MA) return 0;
  current_mA = constrain(current_mA, MIN_CURRENT_MA, MAX_CURRENT_MA);
  float depth = ((current_mA - MIN_CURRENT_MA) / (MAX_CURRENT_MA - MIN_CURRENT_MA)) * MAX_DEPTH_CM;
  return depth;
}

float calculatePercentage(float current_mA) {
  if (current_mA < MIN_CURRENT_MA) return 0;
  current_mA = constrain(current_mA, MIN_CURRENT_MA, MAX_CURRENT_MA);
  float percentage = ((current_mA - MIN_CURRENT_MA) / (MAX_CURRENT_MA - MIN_CURRENT_MA)) * 100.0;
  return percentage;
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Header
  display.setCursor(0, 0);
  display.print("HOME ");
  if (connectionActive) {
    display.print("CONNECTED");
  } else if (packetsReceived > 0) {
    display.print("LOST");
  } else {
    display.print("WAITING...");
  }
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

  if (packetsReceived > 0) {
    // Calculate depth for display
    float depthCm = calculateDepth(lastCurrent);
    float depthInches = depthCm * CM_TO_INCHES;
    float depthPercent = calculatePercentage(lastCurrent);

    // Water level display
    if (lastCurrent >= MIN_CURRENT_MA) {
      display.setCursor(0, 14);
      display.print("Level:");
      display.print(depthPercent, 0);
      display.print("% ");
      display.print("M:");
      display.print(lastMoisture, 0);
      display.print("%");

      // Large depth display
      display.setTextSize(2);
      display.setCursor(0, 26);

      if (depthInches >= 12.0) {
        int feet = (int)(depthInches / 12);
        float remainingInches = depthInches - (feet * 12);
        display.print(feet);
        display.print("'");
        display.print(remainingInches, 1);
        display.print("\"");
      } else {
        display.print(depthInches, 1);
        display.print(" in");
      }
    } else {
      // No water level sensor
      display.setCursor(0, 14);
      display.print("Moisture Only Mode");

      display.setTextSize(2);
      display.setCursor(0, 28);
      display.print(lastMoisture, 1);
      display.print("%");
    }

    // Signal quality (small text)
    display.setTextSize(1);
    display.setCursor(0, 48);
    display.print("Riv:");
    display.print(lastRSSI_River);
    display.print(" Hm:");
    display.print(lastRSSI_Home);
    display.print("dBm");

    display.setCursor(0, 56);
    display.print("RX:");
    display.print(packetsReceived);
    if (packetErrors > 0) {
      display.print(" Err:");
      display.print(packetErrors);
    }
  } else {
    // No data yet
    display.setCursor(0, 20);
    display.println("Waiting for data");
    display.println("from river sensor");
    display.println();
    display.print("Freq: ");
    display.print(LORA_FREQUENCY, 1);
    display.println(" MHz");
  }

  display.display();
}

