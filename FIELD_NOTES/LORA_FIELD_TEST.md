# LoRa Field Testing Guide

This guide explains how to interpret the LoRa signal readings and what to expect during field testing of the river monitoring network.

## Network Overview

```
┌─────────────────┐         ┌─────────────────┐         ┌─────────────────┐
│   RIVER UNIT    │  Link 1 │  RIDGE RELAY    │  Link 2 │   HOME UNIT     │
│                 │ ──────> │                 │ ──────> │                 │
│  At the River   │         │  On the Ridge   │         │   At Home       │
└─────────────────┘         └─────────────────┘         └─────────────────┘
```

**Two separate radio links:**
- **Link 1**: River Unit → Ridge Relay
- **Link 2**: Ridge Relay → Home Unit

Each link's signal strength is measured and displayed independently.

## Expected Range with Current Settings

### Current Configuration
| Setting | Value | Notes |
|---------|-------|-------|
| Frequency | 915 MHz | US ISM band |
| Spreading Factor | 9 | Balanced range/speed |
| Bandwidth | 125 kHz | Standard |
| TX Power | 14 dBm | ~25 milliwatts |

### Estimated Distances

| Conditions | Expected Range |
|------------|----------------|
| Clear line of sight (open field) | 2-5 km (1.2-3 miles) |
| Partial obstructions (trees, buildings) | 500m - 2 km (0.3-1.2 miles) |
| Dense forest or urban area | 200-500m (650-1600 ft) |
| Through walls/buildings | 50-200m (160-650 ft) |

**Important:** The ridge relay MUST have line of sight to BOTH the river and home units for best results. That's why it's placed on high ground!

### Increasing Range (if needed)

If signal is weak, edit `lora_config.h` on ALL THREE units:

```cpp
// For longer range:
#define LORA_SPREADING      12       // Max spreading factor (slower but longer range)
#define LORA_TX_POWER       22       // Max power (legal limit)
```

This can extend range to 5-10 km in good conditions, but transmissions take longer.

---

## Understanding the Display Readings

### River Unit Display

```
┌────────────────────────┐
│ RIVER TX:OK #42        │  <- Transmission status and packet number
│────────────────────────│
│ I:7.45mA 21%           │  <- Current reading and tank percentage
│ Depth:                 │
│ 8.5"                   │  <- Water depth in inches
│ Moist:45%              │  <- Soil moisture percentage
└────────────────────────┘
```

| Reading | Meaning |
|---------|---------|
| **TX:OK** | Packet transmitted successfully |
| **TX:--** | Transmission failed (LoRa error) |
| **#42** | Packet sequence number (0-255, wraps around) |

### Ridge Relay Display

```
┌────────────────────────┐
│ RIDGE RELAY  RX:25     │  <- Total packets received and relayed
│────────────────────────│
│ Current: 7.45 mA       │  <- Last received current reading
│ Moisture: 45.0 %       │  <- Last received moisture reading
│ Depth: 8.5"            │  <- Calculated depth
│ Signal: -27 dBm        │  <- Signal strength from River Unit (Link 1)
│ Status: RELAYING OK    │  <- Relay status
└────────────────────────┘
```

| Reading | Meaning |
|---------|---------|
| **RX:25** | Total number of packets successfully received and relayed |
| **Signal: -27 dBm** | How strong the River Unit's signal is at the Ridge (Link 1) |
| **RELAYING OK** | Successfully forwarding packets to Home Unit |

### Home Unit Display

```
┌────────────────────────┐
│ HOME CONNECTED         │  <- Connection status
│────────────────────────│
│ Level:21% M:45%        │  <- Tank level and moisture
│ 8.5 in                 │  <- Depth (large text)
│                        │
│ Riv:-27 Hm:-35dBm      │  <- Signal strengths for both links
│ RX:25                  │  <- Total packets received
└────────────────────────┘
```

| Reading | Meaning |
|---------|---------|
| **CONNECTED** | Receiving data normally |
| **LOST** | No data received for 60+ seconds |
| **WAITING...** | Never received any data yet |
| **Riv:-27** | Signal strength of Link 1 (River → Ridge) |
| **Hm:-35** | Signal strength of Link 2 (Ridge → Home) |
| **RX:25** | Total packets received at home |

---

## Signal Strength (RSSI) Explained

**RSSI** = Received Signal Strength Indicator, measured in **dBm** (decibel-milliwatts)

### The dBm Scale

dBm is a **logarithmic** scale where:
- **Higher numbers (closer to 0) = STRONGER signal**
- **Lower numbers (more negative) = WEAKER signal**

```
STRONGER ◄──────────────────────────────────────────► WEAKER

   -20    -40    -60    -80    -100   -120   -140
    │      │      │      │       │      │      │
 Amazing  Great  Good   Fair    Weak  V.Weak  None
```

### Signal Quality Reference

| RSSI (dBm) | Quality | What to Expect |
|------------|---------|----------------|
| **-20 to -50** | Excellent | Units are very close, rock-solid connection |
| **-50 to -70** | Very Good | Reliable connection, no issues expected |
| **-70 to -90** | Good | Normal operating range, should work well |
| **-90 to -100** | Fair | Marginal, may see occasional packet loss |
| **-100 to -110** | Weak | Unreliable, expect frequent packet loss |
| **-110 to -120** | Very Weak | At the edge of reception, mostly failing |
| **Below -120** | No Signal | Too far or blocked, no communication |

### What the Numbers Mean in Practice

**Example: Ridge Relay shows "Signal: -67 dBm"**
- This is the signal from the River Unit (Link 1)
- -67 dBm is in the "Very Good" range
- The River → Ridge link is healthy

**Example: Home Unit shows "Riv:-67 Hm:-85"**
- **Riv:-67** = Link 1 (River → Ridge) is Very Good
- **Hm:-85** = Link 2 (Ridge → Home) is Good
- Both links are working, data should flow reliably

### Troubleshooting by Signal Strength

| Symptom | Likely Cause | Solution |
|---------|--------------|----------|
| Riv: weak, Hm: strong | River too far from Ridge | Move Ridge closer to River, or increase TX power |
| Riv: strong, Hm: weak | Ridge too far from Home | Move Ridge closer to Home, or raise Ridge antenna |
| Both weak | Ridge location is poor | Find higher ground with better line of sight to both |
| Signal varies wildly | Interference or movement | Check for obstacles, secure antennas |

---

## SNR (Signal-to-Noise Ratio)

The serial output also shows **SNR** in dB (decibels):

```
RSSI: -67 dBm, SNR: 9.5 dB
```

### SNR Values

| SNR (dB) | Quality | Meaning |
|----------|---------|---------|
| **> 10** | Excellent | Signal is much stronger than noise |
| **5 to 10** | Good | Clear signal, reliable |
| **0 to 5** | Fair | Signal equals noise, may have errors |
| **-5 to 0** | Poor | Noise is stronger than signal |
| **< -5** | Very Poor | Mostly noise, unreliable |

LoRa can work with negative SNR (signal weaker than noise!) due to spread spectrum technology, but positive SNR is preferred.

---

## Packet Counter (RX:#)

### What It Means

- **RX:25** = 25 packets successfully received
- Counter increments each time a valid packet arrives
- Helps verify the link is working

### Checking for Packet Loss

The River Unit sends a **sequence number** (0-255) with each packet. If you see gaps in the serial output:

```
Packet #23 received
Packet #24 received
Packet #27 received   <- #25 and #26 were lost!
```

The Home Unit will report: `Missed 2 packet(s)`

### Acceptable Packet Loss

| Packet Loss | Assessment |
|-------------|------------|
| **0%** | Perfect - ideal conditions |
| **1-5%** | Good - normal for real-world conditions |
| **5-10%** | Fair - consider improving antenna placement |
| **10-25%** | Poor - move relay or increase power |
| **> 25%** | Unreliable - major obstruction or too far |

---

## Field Testing Procedure

### Equipment Checklist

- [ ] River Unit (with sensors connected and powered)
- [ ] Ridge Relay (with charged battery)
- [ ] Home Unit (with USB power or battery)
- [ ] Phone/radio for communication between testers
- [ ] Notebook to record signal readings
- [ ] This guide!

### Step-by-Step Testing

#### 1. Bench Test (before going outside)

Place all three units on a table, power them on:
- All displays should show data
- RSSI should be excellent (-20 to -40 dBm)
- RX counter should increment every 10 seconds
- Verify all units are working before field deployment

#### 2. Initial Deployment

1. **Place River Unit** at the river location
2. **Walk with Ridge Relay** toward the ridge
3. **Watch the signal strength** on Ridge Relay display
4. **Stop when you reach the ridge** or signal drops below -90 dBm

Record: `Link 1 RSSI at ridge location: ______ dBm`

#### 3. Test Link 2

1. **Leave Ridge Relay** at the ridge location
2. **Walk with Home Unit** toward home
3. **Watch the signal strength** (Hm: value)
4. **Stop at home location** and record signal

Record: `Link 2 RSSI at home location: ______ dBm`

#### 4. Long-Term Monitoring

Leave the system running for at least 1 hour:
- Count total packets received (RX:#)
- Note any gaps in sequence numbers
- Calculate packet loss percentage

```
Packet Loss % = (Missed Packets / Total Expected) × 100
```

### Recording Your Results

| Location | Link | RSSI (dBm) | SNR (dB) | Notes |
|----------|------|------------|----------|-------|
| River → Ridge | 1 | | | |
| Ridge → Home | 2 | | | |

| Test Duration | Packets Sent | Packets Received | Loss % |
|---------------|--------------|------------------|--------|
| | | | |

---

## Quick Reference Card

Print this section for field use:

```
╔══════════════════════════════════════════════════════╗
║           LORA SIGNAL QUICK REFERENCE                ║
╠══════════════════════════════════════════════════════╣
║  RSSI (dBm)    Quality     Action                    ║
║  ─────────────────────────────────────────────────── ║
║  -20 to -50    EXCELLENT   Perfect, don't change     ║
║  -50 to -70    VERY GOOD   Ideal operating range     ║
║  -70 to -90    GOOD        Normal, should work       ║
║  -90 to -100   FAIR        May lose some packets     ║
║  -100 to -110  WEAK        Consider repositioning    ║
║  Below -110    TOO WEAK    Move closer or add power  ║
╠══════════════════════════════════════════════════════╣
║  HOME DISPLAY: "Riv:-67 Hm:-85"                      ║
║    Riv = River→Ridge signal (Link 1)                 ║
║    Hm  = Ridge→Home signal (Link 2)                  ║
╠══════════════════════════════════════════════════════╣
║  RX:## = Total packets received successfully         ║
║  TX:OK = Transmission successful                     ║
║  CONNECTED = Receiving data normally                 ║
║  LOST = No data for 60+ seconds                      ║
╚══════════════════════════════════════════════════════╝
```

---

## Troubleshooting During Field Test

| Problem | Check This |
|---------|------------|
| Ridge Relay shows no packets | Is River Unit powered on and transmitting (TX:OK)? |
| Home Unit shows LOST | Is Ridge Relay receiving (RX:#) increasing)? |
| Signal strength varies | Is there movement? Wind affecting antennas? |
| Sudden signal drop | Did someone/something block line of sight? |
| All units show no data | Are all units on the same frequency (915 MHz)? |

## Contact During Testing

When field testing with multiple people, establish check-in times:

- "River station, what's your TX count?"
- "Ridge station, what's your RX count and Link 1 signal?"
- "Home station, what's your RX count and both signal readings?"

Compare counts to identify where packets are being lost.

---

## After Testing

1. Note the final signal strengths for both links
2. Calculate overall packet loss
3. If packet loss > 10%, consider:
   - Moving the Ridge Relay to higher ground
   - Increasing spreading factor (SF 10, 11, or 12)
   - Increasing TX power (up to 22 dBm)
   - Adding external antennas

Document your findings for future reference!
