# LoRa Technical Reference

## ESP32 Hydrostatic Sensor Network

This document provides a comprehensive technical overview of the LoRa implementation in this three-node wireless sensor network for remote water level monitoring.

---

## 1. LoRa Technology Overview

### 1.1 What is LoRa?

**LoRa** (Long Range) is a proprietary spread-spectrum modulation technique developed by Semtech Corporation. It operates in unlicensed ISM (Industrial, Scientific, Medical) bands and is designed for long-range, low-power wireless communication.

**Key Characteristics:**

| Property | Description |
|----------|-------------|
| **Modulation** | Chirp Spread Spectrum (CSS) - frequency sweeps up or down over time |
| **Range** | 2-15+ km line-of-sight; 1-5 km urban/obstructed |
| **Data Rate** | 0.3 - 27 kbps (depending on spreading factor and bandwidth) |
| **Power** | Ultra-low power consumption; years on battery possible |
| **Frequency Bands** | 433 MHz (Asia), 868 MHz (EU), 915 MHz (US/AU), 923 MHz (Japan) |
| **Topology** | Point-to-point, star, or mesh (with appropriate protocols) |

### 1.2 How LoRa Works

**Chirp Spread Spectrum (CSS):**

Unlike traditional modulation (FSK, ASK), LoRa encodes data in frequency chirps - signals that continuously sweep from low to high frequency (up-chirp) or high to low (down-chirp).

```
Frequency
    ▲
    │    ╱╲    ╱╲    ╱╲
    │   ╱  ╲  ╱  ╲  ╱  ╲    ← Up-chirps encode "1" bits
    │  ╱    ╲╱    ╲╱    ╲
    │ ╱
    └─────────────────────► Time
```

**Why CSS is robust:**
- Resistant to multipath interference (reflections)
- Works below the noise floor (negative SNR)
- Resistant to Doppler shift
- Difficult to jam or interfere with

**Spreading Factor (SF):**

The spreading factor determines how much the signal is "spread" across the bandwidth:

| SF | Chips/Symbol | Sensitivity | Air Time | Range |
|----|--------------|-------------|----------|-------|
| SF7 | 128 | -123 dBm | Fastest | Shortest |
| SF8 | 256 | -126 dBm | ↓ | ↓ |
| SF9 | 512 | -129 dBm | ↓ | ↓ |
| SF10 | 1024 | -132 dBm | ↓ | ↓ |
| SF11 | 2048 | -134.5 dBm | ↓ | ↓ |
| SF12 | 4096 | -137 dBm | Slowest | Longest |

Each increment of SF doubles the air time but gains ~2.5 dB sensitivity (roughly 1.5x range increase).

**Bandwidth (BW):**

Common bandwidths: 125 kHz, 250 kHz, 500 kHz

- **Wider bandwidth** = faster data rate, less sensitivity, more interference tolerance
- **Narrower bandwidth** = slower data rate, better sensitivity, more susceptible to frequency drift

**Coding Rate (CR):**

Forward Error Correction adds redundancy:

| CR | Notation | Overhead | Error Correction |
|----|----------|----------|------------------|
| 4/5 | 1 | 25% | Minimal |
| 4/6 | 2 | 50% | Low |
| 4/7 | 3 | 75% | Medium |
| 4/8 | 4 | 100% | Maximum |

### 1.3 LoRa vs. LoRaWAN

**LoRa** is the physical layer (PHY) - the radio modulation technique.

**LoRaWAN** is a MAC (Media Access Control) protocol built on top of LoRa:

| Aspect | Raw LoRa | LoRaWAN |
|--------|----------|---------|
| **Architecture** | Point-to-point or custom | Star-of-stars with gateways |
| **Protocol** | None (you define it) | Standardized by LoRa Alliance |
| **Security** | None (you implement it) | AES-128 built-in |
| **Network Server** | Not required | Required (TTN, Chirpstack, etc.) |
| **Device Classes** | N/A | Class A, B, C |
| **Complexity** | Simple | Complex infrastructure |
| **Latency** | Low (immediate TX) | Variable (scheduled windows) |
| **Use Case** | Private networks, custom protocols | Large-scale IoT deployments |

**This project uses raw LoRa** - we define our own packet format and protocol without LoRaWAN infrastructure.

---

## 2. Comparison with Meshtastic

### 2.1 What is Meshtastic?

**Meshtastic** is an open-source mesh networking project that runs on LoRa-capable devices. It creates self-forming, multi-hop mesh networks primarily designed for off-grid text messaging and GPS location sharing.

### 2.2 Architectural Comparison

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           MESHTASTIC ARCHITECTURE                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│    [Node A] ◄───► [Node B] ◄───► [Node C] ◄───► [Node D]                   │
│        │             │             │             │                          │
│        └─────────────┴─────────────┴─────────────┘                          │
│                    Full Mesh Topology                                       │
│                                                                             │
│    • Every node can talk to every other node                                │
│    • Messages flood through the network                                     │
│    • Automatic route discovery                                              │
│    • Up to 80+ nodes per mesh                                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                        OUR IMPLEMENTATION ARCHITECTURE                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│    [RIVER] ─────────► [RIDGE] ─────────► [HOME]                            │
│    Sensor              Relay              Receiver                          │
│                                                                             │
│    • Fixed linear topology                                                  │
│    • Single-purpose nodes                                                   │
│    • Unidirectional data flow                                               │
│    • No mesh routing overhead                                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.3 Feature Comparison

| Feature | Meshtastic | Our Implementation |
|---------|------------|-------------------|
| **Primary Purpose** | Text messaging, location sharing | Sensor data telemetry |
| **Topology** | Self-forming mesh | Fixed linear relay |
| **Routing** | Flood routing with hop limits | Single-hop relay (no routing) |
| **Node Discovery** | Automatic | None (hardcoded IDs) |
| **Encryption** | AES-256 (mandatory) | None (plaintext) |
| **Compression** | Protocol Buffers (protobuf) | Raw struct (16 bytes) |
| **Message Types** | Text, position, telemetry, admin | Sensor data only |
| **Acknowledgments** | Optional ACKs and retries | None |
| **Channel Hopping** | Supports multiple channels | Single frequency |
| **Power Modes** | Client, router, router-client | Custom sleep/wake cycle |
| **Firmware** | Monolithic (all features) | Minimal (single purpose) |
| **Configuration** | Bluetooth/WiFi app | Compile-time constants |
| **Code Size** | ~1.5 MB firmware | ~200 KB firmware |
| **RAM Usage** | ~100+ KB | ~20 KB |

### 2.4 Why Not Use Meshtastic?

Meshtastic is an excellent project, but it wasn't the right fit for this application:

**1. Overhead for Simple Use Case**

```
Meshtastic packet overhead:
├── Mesh header (routing, hop count, flags)     ~12 bytes
├── Protobuf encoding overhead                  ~8 bytes
├── Encryption overhead (AES-256 + nonce)       ~16 bytes
├── Actual sensor payload                       ~16 bytes
└── Total: ~52+ bytes per transmission

Our implementation:
├── Simple header (msgType, IDs, sequence)      4 bytes
├── Sensor data                                 10 bytes
├── Metadata (RSSI, battery)                    3 bytes
├── Checksum                                    1 byte
└── Total: 16 bytes per transmission
```

Our packets are **3x smaller**, meaning:
- Shorter air time (less battery drain)
- Lower collision probability
- More transmissions per duty cycle limit

**2. Power Consumption**

Meshtastic nodes participate in mesh routing, which requires:
- Longer listen windows to receive mesh traffic
- Processing and forwarding other nodes' messages
- More complex state management

Our relay wakes for 3 seconds, listens for our specific packet, relays once, and sleeps. It doesn't participate in mesh maintenance.

**Estimated battery comparison (2000 mAh):**
- Meshtastic router mode: ~3-5 days
- Our custom relay: ~12+ days

**3. Latency and Determinism**

Meshtastic uses flood routing with random rebroadcast delays to avoid collisions:
- Message may take 1-30+ seconds to traverse mesh
- Non-deterministic delivery time
- May receive duplicate messages from multiple paths

Our implementation:
- River TX → Ridge RX → Ridge TX → Home RX
- Total latency: ~500 ms (deterministic)
- Exactly one path, no duplicates

**4. Complexity**

Meshtastic requires:
- Bluetooth pairing for configuration
- Mobile app or Python CLI
- Understanding of channels, roles, and regions
- Firmware updates via app

Our implementation:
- Edit `lora_config.h`, recompile, flash
- No external dependencies at runtime
- ~500 lines of code total vs. ~50,000+ in Meshtastic

**5. Single-Purpose Optimization**

We don't need:
- Text messaging
- GPS tracking
- Multiple channels
- Node discovery
- Remote administration
- Telemetry aggregation

By removing these features, we get:
- Smaller firmware
- Less RAM usage
- Simpler debugging
- Purpose-built behavior

### 2.5 When to Use Meshtastic Instead

Meshtastic would be the better choice if:

- You need **text messaging** between humans
- You have **many nodes** (>3) that need to communicate with each other
- You need **encryption** for sensitive data
- You want **mobile app integration**
- You need **GPS location sharing**
- Network topology is **unknown or dynamic**
- You prefer **configuration over coding**

### 2.6 Protocol Differences Summary

```
┌──────────────────────────────────────────────────────────────────┐
│                    MESHTASTIC PACKET STRUCTURE                   │
├──────────────────────────────────────────────────────────────────┤
│ LoRa Header │ Mesh Header │ Encrypted Protobuf Payload │ MIC    │
│   (PHY)     │  (Routing)  │        (Variable)          │ (Auth) │
├──────────────────────────────────────────────────────────────────┤
│ Mesh Header contains:                                            │
│ • Source node ID (4 bytes)                                       │
│ • Destination node ID (4 bytes)                                  │
│ • Packet ID (4 bytes)                                            │
│ • Flags (want_ack, via_mqtt, hop_limit, etc.)                   │
│ • Channel hash                                                   │
│ • Hop count                                                      │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                    OUR PACKET STRUCTURE                          │
├──────────────────────────────────────────────────────────────────┤
│ msgType │ srcID │ relayID │ seq │ current │ moisture │ rssi │...│
│  1 byte │1 byte │ 1 byte  │ 1   │ 4 bytes │ 4 bytes  │ 2    │...│
├──────────────────────────────────────────────────────────────────┤
│ Simple C struct, no encoding overhead                            │
│ Fixed 16-byte size                                               │
│ XOR checksum for basic integrity                                 │
└──────────────────────────────────────────────────────────────────┘
```

### 2.7 Interoperability

**Can this system communicate with Meshtastic nodes?**

No. The protocols are incompatible:

1. **Sync Word:** Meshtastic uses `0x2B` (public) or custom; we use `0x12`
2. **Packet Format:** Meshtastic uses encrypted protobuf; we use raw struct
3. **Addressing:** Meshtastic uses 4-byte node IDs; we use 1-byte unit IDs
4. **Routing:** Meshtastic expects mesh headers; we have none

To integrate with Meshtastic, you would need a gateway node running both protocols.

---

## 3. Our Network Topology

```
┌─────────────────┐         LoRa Link 1          ┌─────────────────┐         LoRa Link 2          ┌─────────────────┐
│   RIVER UNIT    │ ───────────────────────────► │   RIDGE RELAY   │ ───────────────────────────► │    HOME UNIT    │
│   (Sensor TX)   │         915 MHz              │   (Repeater)    │         915 MHz              │   (Receiver)    │
│                 │                              │                 │                              │                 │
│ • INA219 4-20mA │                              │ • Battery power │                              │ • OLED display  │
│ • Moisture sens │                              │ • Deep sleep    │                              │ • Serial log    │
│ • Continuous TX │                              │ • Wake/listen   │                              │ • Continuous RX │
└─────────────────┘                              └─────────────────┘                              └─────────────────┘
     10s TX interval                                  8s sleep cycle                                 60s timeout
```

**Use Case:** Remote river monitoring where direct line-of-sight between sensor and home is obstructed. A ridge-top relay provides store-and-forward capability with minimal battery consumption.

---

## 4. Radio Configuration

### 4.1 Physical Layer Parameters

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| **Frequency** | 915.0 MHz | US ISM band (902-928 MHz), license-free |
| **Bandwidth** | 125 kHz | Standard LoRa BW, good range/throughput balance |
| **Spreading Factor** | 9 | Mid-range SF; ~2-5 km LOS capability |
| **Coding Rate** | 4/7 | Higher redundancy (4/5 is minimum, 4/8 maximum) |
| **TX Power** | 14 dBm | ~25 mW, conservative for battery life |
| **Sync Word** | 0x12 | Private network identifier (public=0x34) |
| **Preamble Length** | 8 symbols | Standard detection threshold |
| **TCXO Voltage** | 1.6V | Temperature-compensated crystal oscillator (Heltec V3) |

### 4.2 Calculated Link Budget

```
TX Power:           +14 dBm
Antenna Gain (TX):  +2 dBi (typical Heltec onboard)
Path Loss (2 km):   ~115 dB (free space @ 915 MHz)
Antenna Gain (RX):  +2 dBi
──────────────────────────────────
Received Signal:    ~-97 dBm

SF9 Sensitivity:    -126 dBm (typical SX1262)
Link Margin:        ~29 dB
```

This provides comfortable margin for foliage, weather, and non-line-of-sight conditions.

### 4.3 Air Time Calculation

For a 16-byte payload with SF9, BW=125kHz, CR=4/7:

- Preamble: 8 symbols × 4.1 ms/symbol = ~33 ms
- Header + Payload: ~165 ms (explicit header mode)
- **Total air time: ~200 ms per packet**

At 10-second intervals: **2% duty cycle** (well under regulatory limits)

---

## 5. Hardware Configuration (Heltec WiFi LoRa 32 V3)

### 5.1 SX1262 Pin Mapping

```c
#define LORA_NSS    8    // SPI Chip Select
#define LORA_RST    12   // Reset
#define LORA_DIO1   14   // Interrupt (RX done, TX done, etc.)
#define LORA_BUSY   13   // Module busy indicator
#define LORA_SCK    9    // SPI Clock
#define LORA_MISO   11   // SPI Data In
#define LORA_MOSI   10   // SPI Data Out
```

### 5.2 SPI Configuration

```cpp
SPIClass loraSPI(HSPI);
loraSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY, loraSPI);
```

Uses the HSPI bus to avoid conflicts with the OLED display on the default SPI bus.

---

## 6. Packet Protocol

### 6.1 Message Types

```c
#define MSG_TYPE_SENSOR  0x01   // Original data from river unit
#define MSG_TYPE_RELAY   0x02   // Relayed data (modified by ridge)
#define MSG_TYPE_ACK     0x03   // Acknowledgment (reserved, not implemented)
#define MSG_TYPE_STATUS  0x04   // Heartbeat/status (reserved, not implemented)
```

### 6.2 Unit Identifiers

```c
#define UNIT_ID_RIVER   0x01   // Sensor transmitter
#define UNIT_ID_RIDGE   0x02   // Primary relay (Heltec)
#define UNIT_ID_RIDGE2  0x05   // Secondary relay (T-Deck, 300ms delayed)
#define UNIT_ID_HOME    0x03   // Receiver/display
```

### 6.3 Packet Structure

```c
typedef struct __attribute__((packed)) {
  uint8_t  msgType;         // 1 byte  - MSG_TYPE_SENSOR or MSG_TYPE_RELAY
  uint8_t  sourceId;        // 1 byte  - Always UNIT_ID_RIVER
  uint8_t  relayId;         // 1 byte  - 0 if direct, UNIT_ID_RIDGE if relayed
  uint8_t  sequence;        // 1 byte  - Rolling 0-255 counter
  float    current_mA;      // 4 bytes - INA219 reading (4-20 mA range)
  float    moisturePercent; // 4 bytes - Soil moisture 0-100%
  int16_t  rssi;            // 2 bytes - RSSI at relay (Link 1 quality)
  uint8_t  batteryPercent;  // 1 byte  - Transmitter battery (0-100)
  uint8_t  checksum;        // 1 byte  - XOR validation
} SensorPacket;             // Total: 16 bytes
```

### 6.4 Checksum Algorithm

Simple XOR of all bytes except the checksum field itself:

```c
uint8_t calculateChecksum(SensorPacket* pkt) {
  uint8_t* bytes = (uint8_t*)pkt;
  uint8_t sum = 0;
  for (int i = 0; i < sizeof(SensorPacket) - 1; i++) {
    sum ^= bytes[i];
  }
  return sum;
}

bool validateChecksum(SensorPacket* pkt) {
  return (calculateChecksum(pkt) == pkt->checksum);
}
```

**Note:** XOR checksum detects single-bit errors but not all multi-bit errors. LoRa's built-in CRC provides additional protection at the PHY layer.

---

## 7. Node Behaviors

### 7.1 River Unit (Transmitter)

**Operating Mode:** Continuous operation, mains-powered

**Transmission Cycle (every 10 seconds):**

```
1. Read INA219 current (average of 10 samples)
2. Read moisture sensor (average of 5 samples)
3. Build SensorPacket:
   - msgType = MSG_TYPE_SENSOR
   - sourceId = UNIT_ID_RIVER
   - relayId = 0 (direct transmission)
   - sequence = packetSequence++
   - current_mA, moisturePercent = sensor values
   - batteryPercent = 100 (hardcoded; TODO: implement)
   - checksum = calculateChecksum(&pkt)
4. Transmit: radio.transmit((uint8_t*)&pkt, sizeof(SensorPacket))
5. Update OLED display with TX status
6. Wait until next 10-second mark
```

**RadioLib Usage:**
```cpp
int state = radio.transmit((uint8_t*)&pkt, sizeof(SensorPacket));
if (state == RADIOLIB_ERR_NONE) {
  Serial.println("TX OK");
} else {
  Serial.printf("TX failed, code %d\n", state);
}
```

### 7.2 Ridge Relay (Repeater)

**Operating Mode:** Battery-powered deep sleep with periodic wake

**Sleep/Wake Cycle:**

```
┌────────────────────────────────────────────────────────────────┐
│ DEEP SLEEP (8 seconds)                                         │
│ • ESP32 in deep sleep mode                                     │
│ • SX1262 in sleep mode                                         │
│ • Current draw: ~25-150 µA                                     │
└────────────────────────────────────────────────────────────────┘
                              │
                              ▼ RTC timer wakeup
┌────────────────────────────────────────────────────────────────┐
│ ACTIVE PHASE (~3 seconds)                                      │
│ 1. Initialize LoRa radio                                       │
│ 2. Listen for packets (blocking receive with timeout)          │
│ 3. If packet received:                                         │
│    a. Validate checksum                                        │
│    b. Check msgType == MSG_TYPE_SENSOR (ignore relayed)        │
│    c. Check relayId == 0 (loop prevention)                     │
│    d. Capture RSSI: pkt.rssi = radio.getRSSI()                 │
│    e. Modify: msgType = MSG_TYPE_RELAY                         │
│    f. Modify: relayId = UNIT_ID_RIDGE                          │
│    g. Recalculate checksum                                     │
│    h. Wait 50 ms (collision avoidance)                         │
│    i. Retransmit packet                                        │
│ 4. Update display (if enabled)                                 │
│ 5. Return to deep sleep                                        │
└────────────────────────────────────────────────────────────────┘
```

**Critical Timing Relationship:**

```
TX_INTERVAL_MS   = 10,000 ms (river transmits every 10s)
RELAY_SLEEP_SEC  =  8,000 ms (relay sleeps for 8s)
RELAY_LISTEN_MS  =  3,000 ms (relay listens for 3s)

Relay active window: [T, T+3s]
Next sleep until:    T+11s

For guaranteed capture: RELAY_SLEEP_SEC < TX_INTERVAL_MS
Current config ensures relay wakes at least once per transmission cycle.
```

**Loop Prevention:**

The relay only forwards packets where:
1. `msgType == MSG_TYPE_SENSOR` (not already MSG_TYPE_RELAY)
2. `relayId == 0` (not already forwarded by another relay)

This prevents infinite relay loops in multi-relay scenarios.

**Test Mode:**

```c
#define TEST_MODE true   // For development/debugging
#define TEST_MODE false  // For battery deployment
```

Test mode disables deep sleep for continuous monitoring and debugging.

### 7.3 Home Unit (Receiver)

**Operating Mode:** Continuous interrupt-driven receive

**Initialization:**

```cpp
radio.setDio1Action(setRxFlag);  // ISR sets volatile flag
radio.startReceive();            // Begin continuous RX
```

**Reception Flow:**

```
┌─────────────────────────────────────────────────────────────┐
│                    IDLE (Continuous RX)                      │
│                                                              │
│    DIO1 interrupt fires ──────► rxFlag = true               │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    PACKET PROCESSING                         │
│                                                              │
│ 1. radio.readData((uint8_t*)&pkt, sizeof(SensorPacket))     │
│ 2. local_rssi = radio.getRSSI()  // Link 2 (ridge→home)     │
│ 3. local_snr = radio.getSNR()                               │
│ 4. Validate checksum                                         │
│ 5. Accept MSG_TYPE_SENSOR or MSG_TYPE_RELAY                 │
│ 6. Extract pkt.rssi as Link 1 quality (river→ridge)         │
│ 7. Detect missed packets via sequence gap                    │
│ 8. Update display with sensor data + dual RSSI values       │
│ 9. radio.startReceive()  // Resume listening                │
└─────────────────────────────────────────────────────────────┘
```

**Sequence Number Tracking:**

```cpp
if (pkt.sequence != ((lastSequence + 1) & 0xFF)) {
  int missed = (pkt.sequence - lastSequence - 1) & 0xFF;
  Serial.printf("Missed %d packet(s)\n", missed);
}
lastSequence = pkt.sequence;
```

**Connection Timeout:**

```cpp
#define RX_TIMEOUT_MS 60000  // 60 seconds

if (millis() - lastPacketTime > RX_TIMEOUT_MS) {
  connectionActive = false;
  // Display shows "LOST" status
}
```

---

## 8. Signal Quality Metrics

### 8.1 Dual RSSI Tracking

The system preserves signal quality for both LoRa hops:

| Variable | Source | Meaning |
|----------|--------|---------|
| `lastRSSI_River` | `pkt.rssi` | River→Ridge link (captured by relay) |
| `lastRSSI_Home` | `radio.getRSSI()` | Ridge→Home link (measured locally) |

This enables diagnosis of which link is problematic.

### 8.2 RSSI Interpretation

| RSSI (dBm) | Signal Quality |
|------------|----------------|
| > -70 | Excellent |
| -70 to -85 | Good |
| -85 to -100 | Fair |
| -100 to -115 | Weak |
| < -115 | Poor (near sensitivity limit) |

### 8.3 SNR Interpretation

| SNR (dB) | Meaning |
|----------|---------|
| > +10 | Strong signal, low noise |
| +5 to +10 | Good conditions |
| 0 to +5 | Acceptable |
| -5 to 0 | Marginal |
| < -5 | Poor (LoRa can decode down to ~-20 dB) |

---

## 9. Power Management

### 9.1 Ridge Relay Power Budget

**Current Consumption:**

| State | Current | Duration |
|-------|---------|----------|
| Deep sleep | ~100 µA | 8 seconds |
| Active (RX) | ~15 mA | 3 seconds |
| Active (TX) | ~120 mA | 200 ms |

**Duty Cycle Calculation:**

```
Total cycle time: 11 seconds
Active time: 3 seconds (27%)
Sleep time: 8 seconds (73%)

Average current ≈ (15mA × 3s + 0.1mA × 8s) / 11s ≈ 4.1 mA
Plus TX burst: (120mA × 0.2s) / 11s ≈ 2.2 mA
Total average: ~6-7 mA
```

**Battery Life Estimates:**

| Battery Capacity | Estimated Runtime |
|------------------|-------------------|
| 1000 mAh | ~6 days |
| 2000 mAh | ~12 days |
| 4000 mAh | ~24 days |
| 6000 mAh | ~36 days |

### 9.2 RTC Memory Persistence

Variables preserved across deep sleep cycles:

```cpp
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int packetsRelayed = 0;
RTC_DATA_ATTR int lastRSSI = 0;
RTC_DATA_ATTR float lastCurrent = 0;
RTC_DATA_ATTR float lastMoisture = 0;
```

### 9.3 Power Optimization Opportunities

1. **Increase sleep duration:** Trade latency for battery life
2. **Reduce TX power:** If link margin permits (check RSSI first)
3. **External interrupt wake:** Configure DIO1 to wake ESP32 on preamble detection (currently disabled)
4. **Adaptive duty cycle:** Longer sleep at night when monitoring is less critical

---

## 10. Error Handling

### 10.1 Error Detection

| Layer | Mechanism | Action on Error |
|-------|-----------|-----------------|
| PHY | LoRa CRC (automatic) | Packet dropped by radio |
| Application | XOR checksum | Packet discarded, logged |
| Application | Sequence gap | Logged as "missed packets" |
| Application | Timeout | Connection marked lost |

### 10.2 No Retry/ACK Mechanism

The current implementation uses a simple "fire-and-forget" approach:

- **Pro:** Minimal complexity, lowest power consumption
- **Con:** No guaranteed delivery

For this application (periodic sensor readings), missing occasional packets is acceptable since fresh data arrives every 10 seconds.

### 10.3 Potential Enhancements

1. **Acknowledgments:** Home unit could transmit ACK packets
2. **Retry on failure:** River unit could retry if no ACK received
3. **Forward Error Correction:** Increase coding rate to 4/8
4. **Duplicate detection:** Track sequence numbers at relay

---

## 11. RadioLib API Reference

### 11.1 Initialization

```cpp
SX1262 radio = new Module(NSS, DIO1, RST, BUSY, spi);

int state = radio.begin(
  915.0,    // frequency (MHz)
  125.0,    // bandwidth (kHz)
  9,        // spreading factor
  7,        // coding rate
  0x12,     // sync word
  14,       // TX power (dBm)
  8,        // preamble length
  1.6,      // TCXO voltage
  false     // use LDO (not DC-DC)
);
```

### 11.2 Transmission

```cpp
int state = radio.transmit(data, length);
// Returns RADIOLIB_ERR_NONE on success
```

### 11.3 Reception (Blocking)

```cpp
int state = radio.receive(buffer, length);
// Blocks until packet or timeout
```

### 11.4 Reception (Interrupt)

```cpp
radio.setDio1Action(isrFunction);
radio.startReceive();

// In ISR:
void isrFunction() { flag = true; }

// In loop:
if (flag) {
  flag = false;
  int state = radio.readData(buffer, length);
  // Process...
  radio.startReceive();  // Resume
}
```

### 11.5 Signal Quality

```cpp
int rssi = radio.getRSSI();    // dBm
float snr = radio.getSNR();    // dB
```

### 11.6 Power Management

```cpp
radio.sleep();     // Low-power mode
radio.standby();   // Faster wake but higher current
```

---

## 12. Configuration Reference

All LoRa parameters are centralized in `lora_config.h`:

```c
// Radio parameters
#define LORA_FREQUENCY      915.0
#define LORA_BANDWIDTH      125.0
#define LORA_SPREADING      9
#define LORA_CODING_RATE    7
#define LORA_SYNC_WORD      0x12
#define LORA_TX_POWER       14
#define LORA_PREAMBLE       8
#define LORA_TCXO_VOLTAGE   1.6

// Timing
#define TX_INTERVAL_MS      10000   // River TX interval
#define RELAY_SLEEP_SEC     8       // Ridge sleep duration
#define RELAY_LISTEN_MS     3000    // Ridge listen window
#define RX_TIMEOUT_MS       60000   // Home connection timeout

// Hardware pins (Heltec V3)
#define LORA_NSS    8
#define LORA_RST    12
#define LORA_DIO1   14
#define LORA_BUSY   13
#define LORA_SCK    9
#define LORA_MISO   11
#define LORA_MOSI   10
```

---

## 13. Troubleshooting

### 13.1 No Packets Received

1. Verify both units using same frequency, SF, BW, sync word
2. Check antenna connections
3. Verify RSSI is above sensitivity threshold (-126 dBm for SF9)
4. Check for interference on 915 MHz

### 13.2 High Packet Loss

1. Check RSSI and SNR values
2. Increase spreading factor (SF10, SF11, SF12) for more range
3. Increase TX power (max 20 dBm for SX1262)
4. Reduce bandwidth to 62.5 kHz (slower but more sensitive)
5. Check for obstructions or Fresnel zone clearance

### 13.3 Relay Not Forwarding

1. Verify relay is waking (check boot counter in serial output)
2. Check timing alignment (sleep < TX interval)
3. Verify checksum calculation matches between units
4. Check msgType and relayId filters

### 13.4 Battery Draining Fast

1. Verify deep sleep is enabled (TEST_MODE = false)
2. Check radio.sleep() is called before ESP32 sleep
3. Measure actual current draw with multimeter
4. Consider longer sleep intervals

---

## 14. Future Enhancements

1. **Bidirectional communication:** ACK packets and remote configuration
2. **Adaptive spreading factor:** Auto-adjust based on link quality
3. **Encryption:** AES-128 payload encryption for security
4. **Multi-hop mesh:** Support for additional relay nodes
5. **LoRaWAN migration:** For cloud integration and managed network
6. **GPS timestamping:** Precise timing for data logging
7. **Solar charging:** For indefinite relay operation

---

## Document History

| Date | Version | Changes |
|------|---------|---------|
| 2025-12-07 | 1.0 | Initial comprehensive documentation |
