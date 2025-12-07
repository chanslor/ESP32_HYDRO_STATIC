#include <Arduino.h>
#line 1 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ridge_relay/ridge_relay.ino"
/*
 * Ridge Relay Unit - Battery-Powered LoRa Repeater
 *
 * Board: Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262)
 *
 * This unit receives data from the river unit and relays it to the home unit.
 * Uses deep sleep to conserve battery power.
 *
 * Operation Cycle:
 * 1. Wake from deep sleep (timer or LoRa interrupt)
 * 2. Listen for incoming packets for RELAY_LISTEN_MS
 * 3. If packet received, retransmit immediately
 * 4. Go back to deep sleep for RELAY_SLEEP_SEC
 *
 * Hardware:
 * - Heltec WiFi LoRa 32 V3 (built-in OLED and LoRa)
 * - Battery (LiPo 3.7V recommended)
 *
 * Power Consumption:
 * - Active: ~50 mA
 * - Deep Sleep: ~25-150 uA
 * - Average with 8s sleep / 3s active: ~5 mA
 * - Estimated battery life with 2000mAh: ~16 days
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RadioLib.h>
#include "lora_config.h"

// ===== TEST MODE =====
// Set to true to disable deep sleep and keep display on for testing
// Set to false for normal battery-saving operation
#define TEST_MODE true

// Deep sleep definitions
#define uS_TO_S_FACTOR 1000000ULL

// OLED pins for V3
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define VEXT_CTRL 36  // Vext power control pin on some Heltec boards

// OLED display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

// RTC memory - survives deep sleep
RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR uint32_t packetsRelayed = 0;
RTC_DATA_ATTR uint32_t lastRSSI = 0;
RTC_DATA_ATTR float lastCurrent = 0;
RTC_DATA_ATTR float lastMoisture = 0;

// Create device instances
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// LoRa radio instance
SPIClass loraSPI(HSPI);
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY, loraSPI);

// Status flags
bool loraInitialized = false;
bool packetReceived = false;
volatile bool rxFlag = false;

// Interrupt handler for LoRa receive
#line 71 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ridge_relay/ridge_relay.ino"
void setRxFlag(void);
#line 80 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ridge_relay/ridge_relay.ino"
void setup();
#line 280 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ridge_relay/ridge_relay.ino"
void loop();
#line 71 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/ridge_relay/ridge_relay.ino"
void setRxFlag(void) {
  rxFlag = true;
}

// Function declarations
bool initLoRa();
void goToDeepSleep();
void updateDisplay(bool hasData, int rssi, float current, float moisture, uint32_t relayed);

void setup() {
  bootCount++;

  Serial.begin(115200);
  delay(100);  // Short delay for serial init

  // Check wake reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  Serial.println();
  Serial.println("Ridge Relay Unit - LoRa Repeater");
  Serial.println("=================================");
  Serial.print("Boot #");
  Serial.print(bootCount);
  Serial.print(" - Packets relayed: ");
  Serial.println(packetsRelayed);

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Wake reason: Timer");
      break;
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wake reason: LoRa interrupt");
      break;
    default:
      Serial.println("Wake reason: Power on / Reset");
      break;
  }

  // Enable Vext power for OLED (required on some Heltec boards)
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, LOW);  // LOW = ON for Vext on Heltec V3
  delay(100);
  Serial.println("Vext power enabled (GPIO 36 = LOW)");

  // Initialize I2C for OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize OLED reset pin
  Serial.print("OLED Reset on GPIO ");
  Serial.println(OLED_RST);
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(50);  // Longer reset pulse
  digitalWrite(OLED_RST, HIGH);
  delay(100); // Wait for OLED to wake up

  // Initialize OLED
  Serial.print("OLED Init on I2C address 0x");
  Serial.print(SCREEN_ADDRESS, HEX);
  Serial.print(" (SDA=");
  Serial.print(OLED_SDA);
  Serial.print(", SCL=");
  Serial.print(OLED_SCL);
  Serial.print(")... ");

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("FAILED!");
  } else {
    Serial.println("OK!");
  }

  // Set display to max brightness for testing
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(0xFF);  // Max brightness (0x00 = min, 0xFF = max)

  // Test pattern to verify display works
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display.display();
  delay(500);
  display.clearDisplay();
  display.display();
  Serial.println("OLED test pattern sent");

  // Initialize LoRa
  loraInitialized = initLoRa();

  if (!loraInitialized) {
    Serial.println("LoRa init failed - sleeping...");
    updateDisplay(false, 0, 0, 0, packetsRelayed);
    delay(2000);
    goToDeepSleep();
  }

  // Start receiving
  Serial.println("Listening for packets...");

  // Show status on display
  updateDisplay(false, lastRSSI, lastCurrent, lastMoisture, packetsRelayed);

  // Listen for incoming packets
  unsigned long startTime = millis();
  bool receivedPacket = false;

  while (millis() - startTime < RELAY_LISTEN_MS) {
    // Check for received packet
    SensorPacket pkt;
    int state = radio.receive((uint8_t*)&pkt, sizeof(SensorPacket));

    if (state == RADIOLIB_ERR_NONE) {
      // Got a packet!
      Serial.println("Packet received!");

      // Validate checksum
      if (!validateChecksum(&pkt)) {
        Serial.println("  Checksum invalid - discarding");
        continue;
      }

      // Check if this is a sensor packet from the river
      if (pkt.msgType != MSG_TYPE_SENSOR || pkt.sourceId != UNIT_ID_RIVER) {
        Serial.println("  Not from river unit - discarding");
        continue;
      }

      // Check if already relayed (avoid loops)
      if (pkt.relayId != 0) {
        Serial.println("  Already relayed - discarding");
        continue;
      }

      // Log received data
      int rxRSSI = radio.getRSSI();
      float rxSNR = radio.getSNR();

      Serial.print("  Seq #");
      Serial.print(pkt.sequence);
      Serial.print(", Current: ");
      Serial.print(pkt.current_mA, 2);
      Serial.print(" mA, Moisture: ");
      Serial.print(pkt.moisturePercent, 1);
      Serial.println("%");
      Serial.print("  RSSI: ");
      Serial.print(rxRSSI);
      Serial.print(" dBm, SNR: ");
      Serial.print(rxSNR);
      Serial.println(" dB");

      // Store for display
      lastRSSI = rxRSSI;
      lastCurrent = pkt.current_mA;
      lastMoisture = pkt.moisturePercent;

      // Modify packet for relay
      pkt.msgType = MSG_TYPE_RELAY;
      pkt.relayId = UNIT_ID_RIDGE;
      pkt.rssi = rxRSSI;
      pkt.checksum = calculateChecksum(&pkt);

      // Small delay before retransmit
      delay(50);

      // Retransmit
      Serial.print("  Relaying... ");
      state = radio.transmit((uint8_t*)&pkt, sizeof(SensorPacket));

      if (state == RADIOLIB_ERR_NONE) {
        Serial.println("OK");
        packetsRelayed++;
        receivedPacket = true;
      } else {
        Serial.print("FAILED! Error: ");
        Serial.println(state);
      }

      // Update display with new data
      updateDisplay(true, rxRSSI, pkt.current_mA, pkt.moisturePercent, packetsRelayed);

      // Exit listen loop - we relayed a packet
      break;
    }

    // Small delay to prevent busy-looping
    delay(10);
  }

  if (!receivedPacket) {
    Serial.println("No packet received during listen window");
  }

  #if TEST_MODE
    // Test mode: stay awake and keep listening
    Serial.println("TEST MODE: Staying awake...");
    Serial.println();

    // Set up interrupt-driven receive for test mode
    radio.setDio1Action(setRxFlag);
    int state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
      Serial.print("startReceive failed: ");
      Serial.println(state);
    }
  #else
    Serial.println("Going to deep sleep...");
    Serial.println();
    goToDeepSleep();
  #endif
}

void loop() {
  #if TEST_MODE
    // Test mode: continuously listen for packets (non-blocking)
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 5000) {
      Serial.print("Listening... uptime: ");
      Serial.print(millis() / 1000);
      Serial.println("s");
      lastDebug = millis();
    }

    // Check if we received a packet via interrupt
    if (rxFlag) {
      rxFlag = false;

      SensorPacket pkt;
      int state = radio.readData((uint8_t*)&pkt, sizeof(SensorPacket));

      if (state == RADIOLIB_ERR_NONE) {
        // Validate and process packet
        if (validateChecksum(&pkt) &&
            pkt.msgType == MSG_TYPE_SENSOR &&
            pkt.sourceId == UNIT_ID_RIVER &&
            pkt.relayId == 0) {

          int rxRSSI = radio.getRSSI();
          float rxSNR = radio.getSNR();

          Serial.println("Packet received!");
          Serial.print("  Seq #");
          Serial.print(pkt.sequence);
          Serial.print(", Current: ");
          Serial.print(pkt.current_mA, 2);
          Serial.print(" mA, Moisture: ");
          Serial.print(pkt.moisturePercent, 1);
          Serial.println("%");
          Serial.print("  RSSI: ");
          Serial.print(rxRSSI);
          Serial.print(" dBm, SNR: ");
          Serial.print(rxSNR);
          Serial.println(" dB");

          // Store for display
          lastRSSI = rxRSSI;
          lastCurrent = pkt.current_mA;
          lastMoisture = pkt.moisturePercent;

          // Modify and relay
          pkt.msgType = MSG_TYPE_RELAY;
          pkt.relayId = UNIT_ID_RIDGE;
          pkt.rssi = rxRSSI;
          pkt.checksum = calculateChecksum(&pkt);

          delay(50);
          Serial.print("  Relaying... ");
          state = radio.transmit((uint8_t*)&pkt, sizeof(SensorPacket));

          if (state == RADIOLIB_ERR_NONE) {
            Serial.println("OK");
            packetsRelayed++;
          } else {
            Serial.print("FAILED! Error: ");
            Serial.println(state);
          }

          updateDisplay(true, rxRSSI, pkt.current_mA, pkt.moisturePercent, packetsRelayed);
        }
      }

      // Restart receive mode
      radio.startReceive();
    }

    delay(10);
  #endif
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
  return true;
}

void goToDeepSleep() {
  // Put radio to sleep
  radio.sleep();

  // Configure timer wake-up
  esp_sleep_enable_timer_wakeup(RELAY_SLEEP_SEC * uS_TO_S_FACTOR);

  // Optional: Also wake on LoRa DIO1 interrupt
  // This allows immediate wake when a packet arrives
  // Note: GPIO 14 is RTC-capable on ESP32-S3
  // Uncomment below for interrupt wake (uses more power)
  // esp_sleep_enable_ext0_wakeup((gpio_num_t)LORA_DIO1, 1);

  // Turn off display to save power
  display.ssd1306_command(SSD1306_DISPLAYOFF);

  // Enter deep sleep
  esp_deep_sleep_start();
}

void updateDisplay(bool hasData, int rssi, float current, float moisture, uint32_t relayed) {
  // Clear display buffer completely
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_BLACK);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);  // White text, black background

  // Header
  display.setCursor(0, 0);
  display.print("RIDGE RELAY  RX:");
  display.print(relayed);
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

  if (hasData || current > 0) {
    // Show relayed sensor data
    display.setCursor(0, 14);
    display.print("Current: ");
    display.print(current, 2);
    display.println(" mA");

    display.setCursor(0, 24);
    display.print("Moisture: ");
    display.print(moisture, 1);
    display.println(" %");

    // Calculate and show depth in inches
    float depthCm = 0;
    if (current >= 4.0) {
      depthCm = ((current - 4.0) / (20.0 - 4.0)) * 100.0;  // MAX_DEPTH_CM = 100
    }
    float depthInches = depthCm * 0.393701;
    display.setCursor(0, 34);
    display.print("Depth: ");
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

    display.setCursor(0, 48);
    display.print("Signal: ");
    display.print(rssi);
    display.println(" dBm");

    display.setCursor(0, 56);
    display.print("Status: RELAYING OK");
  } else {
    display.setCursor(0, 24);
    display.println("Waiting for data");
    display.println("from river unit...");
    display.setCursor(0, 48);
    display.print("Freq: ");
    display.print(LORA_FREQUENCY, 1);
    display.println(" MHz");
  }

  display.display();
}

