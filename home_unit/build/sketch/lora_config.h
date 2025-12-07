#line 1 "/home/mdchansl/IOT/ESP32_HYDRO_STATIC/home_unit/lora_config.h"
/*
 * LoRa Configuration for River Monitoring Network
 *
 * This file contains shared LoRa settings that MUST match on all three units:
 * - River Unit (sensor + transmitter)
 * - Ridge Relay (battery-powered repeater)
 * - Home Unit (receiver + display)
 */

#ifndef LORA_CONFIG_H
#define LORA_CONFIG_H

// ===== LoRa Radio Settings =====
// These MUST be identical on all three units!

#define LORA_FREQUENCY      915.0    // MHz (US ISM band: 902-928 MHz)
#define LORA_BANDWIDTH      125.0    // kHz (125.0 is standard)
#define LORA_SPREADING      9        // Spreading Factor (7-12, higher = longer range, slower)
#define LORA_CODING_RATE    7        // Coding Rate denominator (5-8, higher = more error correction)
#define LORA_SYNC_WORD      0x12     // Private network sync word (must match all units)
#define LORA_TX_POWER       14       // dBm (-9 to 22, 14 is good for <1km)
#define LORA_PREAMBLE       8        // Preamble length (8 is standard)

// Heltec V3 SX1262 Pin Definitions
#define LORA_NSS            8        // SPI Chip Select
#define LORA_RST            12       // Reset
#define LORA_DIO1           14       // Interrupt (labeled DIO0 in some Heltec docs)
#define LORA_BUSY           13       // Busy status
#define LORA_SCK            9        // SPI Clock
#define LORA_MISO           11       // SPI MISO
#define LORA_MOSI           10       // SPI MOSI

// TCXO voltage for Heltec V3
#define LORA_TCXO_VOLTAGE   1.6      // Volts

// ===== Message Protocol =====
// Simple packet format for sensor data

// Message types
#define MSG_TYPE_SENSOR     0x01     // Sensor data from river unit
#define MSG_TYPE_RELAY      0x02     // Relayed sensor data from ridge
#define MSG_TYPE_ACK        0x03     // Acknowledgment (optional)
#define MSG_TYPE_STATUS     0x04     // Status/heartbeat

// Network IDs (to identify units)
#define UNIT_ID_RIVER       0x01     // River sensor unit
#define UNIT_ID_RIDGE       0x02     // Ridge relay unit
#define UNIT_ID_HOME        0x03     // Home receiver unit

// ===== Timing Settings =====

// River Unit: How often to transmit (milliseconds)
#define TX_INTERVAL_MS      10000    // 10 seconds (matches sensor read interval)

// Ridge Relay: Deep sleep duration between wake cycles (seconds)
#define RELAY_SLEEP_SEC     8        // Wake every 8 seconds to check for messages
                                     // Must be less than TX_INTERVAL to catch transmissions

// Ridge Relay: How long to listen after waking (milliseconds)
#define RELAY_LISTEN_MS     3000     // 3 seconds listening window

// Home Unit: Timeout to consider connection lost (milliseconds)
#define RX_TIMEOUT_MS       60000    // 60 seconds without data = connection lost

// ===== Data Packet Structure =====
// Total: 16 bytes fixed size for reliable transmission

typedef struct __attribute__((packed)) {
  uint8_t  msgType;         // Message type (MSG_TYPE_*)
  uint8_t  sourceId;        // Original sender ID (UNIT_ID_*)
  uint8_t  relayId;         // Relay ID (0 if direct, UNIT_ID_RIDGE if relayed)
  uint8_t  sequence;        // Sequence number (0-255, wraps)
  float    current_mA;      // INA219 current reading (4 bytes)
  float    moisturePercent; // Moisture sensor reading (4 bytes)
  int16_t  rssi;            // RSSI at relay (or 0 if direct) (2 bytes)
  uint8_t  batteryPercent;  // Battery level of sender (0-100)
  uint8_t  checksum;        // Simple checksum for validation
} SensorPacket;

// Calculate simple checksum
inline uint8_t calculateChecksum(SensorPacket* pkt) {
  uint8_t sum = 0;
  uint8_t* data = (uint8_t*)pkt;
  // Sum all bytes except the last one (checksum field)
  for (int i = 0; i < sizeof(SensorPacket) - 1; i++) {
    sum ^= data[i];  // XOR checksum
  }
  return sum;
}

// Validate checksum
inline bool validateChecksum(SensorPacket* pkt) {
  return pkt->checksum == calculateChecksum(pkt);
}

#endif // LORA_CONFIG_H
