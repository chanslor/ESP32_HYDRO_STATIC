#include <Arduino.h>
#line 1 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/tdeck_relay/tdeck_relay.ino"
/*
 * T-DECK Ridge Relay Unit - LoRa Repeater (Secondary/Backup)
 *
 * Board: LILYGO T-Deck (ESP32-S3 + SX1262 + ST7789 display)
 *
 * This is a SECONDARY relay that works alongside the primary Heltec relay.
 * It uses a staggered delay (300ms) to avoid RF collisions with the primary.
 *
 * Operation:
 * 1. Receive packet from river unit
 * 2. Wait 300ms (primary relay transmits at 50ms)
 * 3. Retransmit with UNIT_ID_RIDGE2 marker
 *
 * The home unit will accept packets from either relay, providing redundancy.
 *
 * Hardware:
 * - LILYGO T-Deck (ESP32-S3 + SX1262 + 2.8" ST7789 LCD)
 * - Built-in battery support
 * - External antenna connector
 *
 * Uses Arduino_GFX library (no global User_Setup.h needed)
 */

#include <SPI.h>
#include <RadioLib.h>
#include <Arduino_GFX_Library.h>
#include "../lora_config.h"

// Color definitions (RGB565 format)
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define ORANGE  0xFD20

// ===== TEST MODE =====
// Set to true to disable deep sleep and keep display on for testing
// Set to false for normal battery-saving operation
#define TEST_MODE true

// ===== T-DECK PIN DEFINITIONS =====

// LoRa SX1262 Pins (T-Deck specific)
#define TDECK_LORA_NSS   9
#define TDECK_LORA_RST   17
#define TDECK_LORA_DIO1  45
#define TDECK_LORA_BUSY  13
#define TDECK_LORA_SCK   40
#define TDECK_LORA_MISO  38
#define TDECK_LORA_MOSI  41

// Display ST7789 Pins
#define TDECK_TFT_CS     12
#define TDECK_TFT_DC     11
#define TDECK_TFT_MOSI   41  // Shared with LoRa
#define TDECK_TFT_SCK    40  // Shared with LoRa
#define TDECK_TFT_BL     42
#define TDECK_TFT_RST    -1  // Not used / tied to board reset

// I2C Pins (for keyboard, touch, etc.)
#define TDECK_I2C_SDA    18
#define TDECK_I2C_SCL    8

// Power control - MUST be HIGH to enable peripherals
#define TDECK_POWER_ON   10

// Battery ADC
#define TDECK_BAT_ADC    4

// SD Card
#define TDECK_SD_CS      39

// Keyboard interrupt
#define TDECK_KB_INT     46

// Trackball pins
#define TDECK_TRACKBALL_UP    3
#define TDECK_TRACKBALL_DOWN  15
#define TDECK_TRACKBALL_LEFT  1
#define TDECK_TRACKBALL_RIGHT 2
#define TDECK_TRACKBALL_CLICK 0

// ===== SCREEN SLEEP SETTINGS =====
#define SCREEN_SLEEP_MS  30000  // 30 seconds until screen sleeps

// ===== RELAY TIMING =====
// Secondary relay waits longer to avoid collision with primary
#define RELAY_DELAY_MS   300  // Primary uses 50ms, we use 300ms

// Deep sleep definitions
#define uS_TO_S_FACTOR 1000000ULL

// RTC memory - survives deep sleep
RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR uint32_t packetsRelayed = 0;
RTC_DATA_ATTR int16_t lastRSSI = 0;
RTC_DATA_ATTR float lastCurrent = 0;
RTC_DATA_ATTR float lastMoisture = 0;

// Create display using Arduino_GFX - all pins defined inline, no global config needed
// T-Deck uses shared SPI bus for display and LoRa
Arduino_DataBus *bus = new Arduino_ESP32SPI(
  TDECK_TFT_DC,   // DC
  TDECK_TFT_CS,   // CS
  TDECK_TFT_SCK,  // SCK
  TDECK_TFT_MOSI, // MOSI
  TDECK_LORA_MISO, // MISO
  HSPI            // Use HSPI
);

// ST7789 with T-Deck specific settings: 240x320, IPS panel, column/row offsets
Arduino_GFX *gfx = new Arduino_ST7789(
  bus,
  -1,             // RST (not connected, use -1)
  1,              // Rotation 1 for T-Deck landscape (flipped from 3)
  true,           // IPS panel
  240,            // Width
  320,            // Height
  0,              // Column offset 1
  0,              // Row offset 1
  0,              // Column offset 2
  0               // Row offset 2
);

// LoRa radio instance - use same SPI bus (HSPI) but different CS
SPIClass loraSPI(HSPI);  // Use same HSPI bus as display
SX1262 radio = new Module(TDECK_LORA_NSS, TDECK_LORA_DIO1, TDECK_LORA_RST, TDECK_LORA_BUSY, loraSPI);

// Status flags
bool loraInitialized = false;
volatile bool rxFlag = false;

// Screen sleep state
bool screenOn = true;
unsigned long lastActivityTime = 0;
volatile bool inputDetected = false;

// Interrupt handler for LoRa receive
#line 162 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/tdeck_relay/tdeck_relay.ino"
void setup();
#line 331 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/tdeck_relay/tdeck_relay.ino"
void loop();
#line 143 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/tdeck_relay/tdeck_relay.ino"
void IRAM_ATTR setRxFlag(void) {
  rxFlag = true;
}

// Interrupt handler for trackball/keyboard input
void IRAM_ATTR onInputDetected(void) {
  inputDetected = true;
}

// Function declarations
bool initLoRa();
void initDisplay();
void goToDeepSleep();
void updateDisplay(bool hasData, int rssi, float current, float moisture, uint32_t relayed);
float readBatteryVoltage();
void setupInputInterrupts();
void screenSleep();
void screenWake();

void setup() {
  bootCount++;

  Serial.begin(115200);
  delay(500);

  // CRITICAL: Enable power to peripherals first!
  pinMode(TDECK_POWER_ON, OUTPUT);
  digitalWrite(TDECK_POWER_ON, HIGH);
  delay(200);

  // Enable backlight
  pinMode(TDECK_TFT_BL, OUTPUT);
  digitalWrite(TDECK_TFT_BL, HIGH);

  Serial.println();
  Serial.println("T-DECK Ridge Relay - Secondary LoRa Repeater");
  Serial.println("=============================================");
  Serial.print("Boot #");
  Serial.print(bootCount);
  Serial.print(" - Packets relayed: ");
  Serial.println(packetsRelayed);
  Serial.println("Relay delay: 300ms (secondary/backup unit)");

  // Check wake reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
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

  // Initialize display
  initDisplay();

  // Initialize LoRa
  loraInitialized = initLoRa();

  if (!loraInitialized) {
    Serial.println("LoRa init failed!");
    gfx->fillScreen(BLACK);
    gfx->setTextColor(GREEN);
    gfx->setTextSize(3);
    gfx->setCursor(40, 100);
    gfx->print("LoRa FAILED!");
    delay(5000);
    goToDeepSleep();
  }

  // Setup input interrupts for screen wake
  setupInputInterrupts();
  lastActivityTime = millis();

  // Show initial status
  updateDisplay(false, lastRSSI, lastCurrent, lastMoisture, packetsRelayed);

  // Listen for incoming packets
  Serial.println("Listening for packets...");
  unsigned long startTime = millis();
  bool receivedPacket = false;

  while (millis() - startTime < RELAY_LISTEN_MS) {
    SensorPacket pkt;
    int state = radio.receive((uint8_t*)&pkt, sizeof(SensorPacket));

    if (state == RADIOLIB_ERR_NONE) {
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

      // Modify packet for relay - use RIDGE2 ID for secondary relay
      pkt.msgType = MSG_TYPE_RELAY;
      pkt.relayId = UNIT_ID_RIDGE2;  // Secondary relay ID
      pkt.rssi = rxRSSI;
      pkt.checksum = calculateChecksum(&pkt);

      // STAGGERED DELAY - wait for primary relay to transmit first
      Serial.print("  Waiting ");
      Serial.print(RELAY_DELAY_MS);
      Serial.println("ms (secondary relay delay)...");
      delay(RELAY_DELAY_MS);

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

      // Update display
      updateDisplay(true, rxRSSI, pkt.current_mA, pkt.moisturePercent, packetsRelayed);

      break;  // Exit listen loop
    }

    delay(10);
  }

  if (!receivedPacket) {
    Serial.println("No packet received during listen window");
  }

  #if TEST_MODE
    Serial.println("TEST MODE: Staying awake...");
    Serial.println();

    // Set up interrupt-driven receive
    radio.setDio1Action(setRxFlag);
    int state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
      Serial.print("startReceive failed: ");
      Serial.println(state);
    }
  #else
    Serial.println("Going to deep sleep...");
    goToDeepSleep();
  #endif
}

void loop() {
  #if TEST_MODE
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 5000) {
      Serial.print("Listening... uptime: ");
      Serial.print(millis() / 1000);
      Serial.print("s, Battery: ");
      Serial.print(readBatteryVoltage(), 2);
      Serial.print("V, Screen: ");
      Serial.println(screenOn ? "ON" : "OFF");
      lastDebug = millis();
    }

    // Check for trackball/keyboard input to wake screen
    if (inputDetected) {
      inputDetected = false;
      lastActivityTime = millis();
      if (!screenOn) {
        screenWake();
        updateDisplay(lastCurrent > 0, lastRSSI, lastCurrent, lastMoisture, packetsRelayed);
      }
    }

    // Check for screen sleep timeout
    if (screenOn && (millis() - lastActivityTime > SCREEN_SLEEP_MS)) {
      screenSleep();
    }

    if (rxFlag) {
      rxFlag = false;

      SensorPacket pkt;
      int state = radio.readData((uint8_t*)&pkt, sizeof(SensorPacket));

      if (state == RADIOLIB_ERR_NONE) {
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

          lastRSSI = rxRSSI;
          lastCurrent = pkt.current_mA;
          lastMoisture = pkt.moisturePercent;

          // Modify and relay with secondary ID
          pkt.msgType = MSG_TYPE_RELAY;
          pkt.relayId = UNIT_ID_RIDGE2;
          pkt.rssi = rxRSSI;
          pkt.checksum = calculateChecksum(&pkt);

          // Staggered delay
          Serial.print("  Waiting ");
          Serial.print(RELAY_DELAY_MS);
          Serial.println("ms...");
          delay(RELAY_DELAY_MS);

          Serial.print("  Relaying... ");
          state = radio.transmit((uint8_t*)&pkt, sizeof(SensorPacket));

          if (state == RADIOLIB_ERR_NONE) {
            Serial.println("OK");
            packetsRelayed++;
          } else {
            Serial.print("FAILED! Error: ");
            Serial.println(state);
          }

          // Only update display if screen is on
          if (screenOn) {
            updateDisplay(true, rxRSSI, pkt.current_mA, pkt.moisturePercent, packetsRelayed);
          }
        }
      }

      radio.startReceive();
    }

    delay(10);
  #endif
}

void initDisplay() {
  Serial.print("Initializing display... ");

  if (!gfx->begin()) {
    Serial.println("FAILED!");
    return;
  }

  // T-Deck ST7789 display inversion off for correct colors
  gfx->invertDisplay(false);
  gfx->fillScreen(BLACK);

  Serial.println("OK (ST7789 320x240)");
}

bool initLoRa() {
  Serial.print("Initializing LoRa (T-Deck pins)... ");

  // Initialize SPI for LoRa with T-Deck pins - use FSPI (separate from display HSPI)
  loraSPI.begin(TDECK_LORA_SCK, TDECK_LORA_MISO, TDECK_LORA_MOSI, TDECK_LORA_NSS);

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
  Serial.print("  Sync Word: 0x");
  Serial.println(LORA_SYNC_WORD, HEX);

  return true;
}

void goToDeepSleep() {
  // Put radio to sleep
  radio.sleep();

  // Turn off backlight
  digitalWrite(TDECK_TFT_BL, LOW);

  // Turn off peripheral power
  digitalWrite(TDECK_POWER_ON, LOW);

  // Configure timer wake-up
  esp_sleep_enable_timer_wakeup(RELAY_SLEEP_SEC * uS_TO_S_FACTOR);

  // Enter deep sleep
  esp_deep_sleep_start();
}

float readBatteryVoltage() {
  // T-Deck battery is on GPIO4 with voltage divider
  // Typical factor is 2.0 (50% divider)
  int raw = analogRead(TDECK_BAT_ADC);
  float voltage = (raw / 4095.0) * 3.3 * 2.0;
  return voltage;
}

void updateDisplay(bool hasData, int rssi, float current, float moisture, uint32_t relayed) {
  gfx->fillScreen(BLACK);
  gfx->setTextColor(GREEN);

  // Header
  gfx->setTextSize(2);
  gfx->setCursor(10, 5);
  gfx->print("T-DECK RELAY");

  gfx->setTextSize(1);
  gfx->setCursor(10, 28);
  gfx->print("(Secondary - 300ms delay)");

  // Divider line
  gfx->drawLine(0, 42, 320, 42, GREEN);

  // Stats
  gfx->setTextSize(2);

  char buf[32];

  // Packets relayed
  gfx->setCursor(10, 50);
  sprintf(buf, "Relayed: %lu", relayed);
  gfx->print(buf);

  if (hasData || current > 0) {
    // Current
    gfx->setCursor(10, 80);
    sprintf(buf, "Current: %.2f mA", current);
    gfx->print(buf);

    // Moisture
    gfx->setCursor(10, 105);
    sprintf(buf, "Moisture: %.1f%%", moisture);
    gfx->print(buf);

    // Depth calculation
    float depthCm = 0;
    if (current >= 4.0) {
      depthCm = ((current - 4.0) / (20.0 - 4.0)) * 100.0;
    }
    float depthInches = depthCm * 0.393701;

    gfx->setCursor(10, 135);
    if (depthInches >= 12.0) {
      int feet = (int)(depthInches / 12);
      float remIn = depthInches - (feet * 12);
      sprintf(buf, "Depth: %d' %.1f\"", feet, remIn);
    } else {
      sprintf(buf, "Depth: %.1f\"", depthInches);
    }
    gfx->print(buf);

    // Signal strength
    gfx->setCursor(10, 165);
    sprintf(buf, "RSSI: %d dBm", rssi);
    gfx->print(buf);

    // Status
    gfx->setCursor(10, 195);
    gfx->print("Status: RELAYING");
  } else {
    gfx->setCursor(10, 100);
    gfx->print("Waiting for");
    gfx->setCursor(10, 125);
    gfx->print("river data...");

    gfx->setTextSize(1);
    gfx->setCursor(10, 160);
    sprintf(buf, "Freq: %.1f MHz", LORA_FREQUENCY);
    gfx->print(buf);
  }

  // Battery voltage
  gfx->setTextSize(1);
  gfx->setCursor(250, 220);
  sprintf(buf, "Bat: %.2fV", readBatteryVoltage());
  gfx->print(buf);
}

void setupInputInterrupts() {
  // Setup trackball pins with pull-up and interrupts
  pinMode(TDECK_TRACKBALL_UP, INPUT_PULLUP);
  pinMode(TDECK_TRACKBALL_DOWN, INPUT_PULLUP);
  pinMode(TDECK_TRACKBALL_LEFT, INPUT_PULLUP);
  pinMode(TDECK_TRACKBALL_RIGHT, INPUT_PULLUP);
  pinMode(TDECK_TRACKBALL_CLICK, INPUT_PULLUP);
  pinMode(TDECK_KB_INT, INPUT_PULLUP);

  // Attach interrupts for all trackball directions and keyboard
  attachInterrupt(digitalPinToInterrupt(TDECK_TRACKBALL_UP), onInputDetected, FALLING);
  attachInterrupt(digitalPinToInterrupt(TDECK_TRACKBALL_DOWN), onInputDetected, FALLING);
  attachInterrupt(digitalPinToInterrupt(TDECK_TRACKBALL_LEFT), onInputDetected, FALLING);
  attachInterrupt(digitalPinToInterrupt(TDECK_TRACKBALL_RIGHT), onInputDetected, FALLING);
  attachInterrupt(digitalPinToInterrupt(TDECK_TRACKBALL_CLICK), onInputDetected, FALLING);
  attachInterrupt(digitalPinToInterrupt(TDECK_KB_INT), onInputDetected, FALLING);

  Serial.println("Input interrupts configured for screen wake");
}

void screenSleep() {
  if (!screenOn) return;

  Serial.println("Screen going to sleep...");
  digitalWrite(TDECK_TFT_BL, LOW);  // Turn off backlight
  screenOn = false;
}

void screenWake() {
  if (screenOn) return;

  Serial.println("Screen waking up...");
  digitalWrite(TDECK_TFT_BL, HIGH);  // Turn on backlight
  screenOn = true;
  lastActivityTime = millis();
}

