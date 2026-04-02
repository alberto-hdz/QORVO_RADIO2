# ThreadBleMotionDetector\_MaxSonar — Testing and Verification Plan

**Document Version:** 1.0
**Application:** ThreadBleMotionDetector\_MaxSonar
**Target Platform:** QPG6200L Development Kit with LV-MaxSonar EZ Sensor
**Firmware Designation:** ThreadBleMotionDetector\_MaxSonar\_qpg6200
**Date:** 2026-03-18
**Status:** Released for Review

---

## Table of Contents

1. [Purpose and Scope](#1-purpose-and-scope)
2. [Reference Documents](#2-reference-documents)
3. [Test Environment](#3-test-environment)
4. [Hardware Configuration](#4-hardware-configuration)
5. [Build Verification](#5-build-verification)
6. [Test Categories](#6-test-categories)
7. [TC-BLD — Build and Flash Tests](#7-tc-bld--build-and-flash-tests)
8. [TC-PWR — Power-On and Boot Tests](#8-tc-pwr--power-on-and-boot-tests)
9. [TC-GPIO — GPIO and Button Tests](#9-tc-gpio--gpio-and-button-tests)
10. [TC-UART — Sensor UART Interface Tests](#10-tc-uart--sensor-uart-interface-tests)
11. [TC-SNS — Sensor Logic and Motion Detection Tests](#11-tc-sns--sensor-logic-and-motion-detection-tests)
12. [TC-BLE — Bluetooth LE Tests](#12-tc-ble--bluetooth-le-tests)
13. [TC-GATT — GATT Service Tests](#13-tc-gatt--gatt-service-tests)
14. [TC-THR — OpenThread Network Tests](#14-tc-thr--openthread-network-tests)
15. [TC-MOT — Motion Event Logic Tests](#15-tc-mot--motion-event-logic-tests)
16. [TC-LED — LED Indicator Tests](#16-tc-led--led-indicator-tests)
17. [TC-NVM — NVM Persistence Tests](#17-tc-nvm--nvm-persistence-tests)
18. [TC-PERF — Performance and Stability Tests](#18-tc-perf--performance-and-stability-tests)
19. [TC-SEC — Security Tests](#19-tc-sec--security-tests)
20. [Acceptance Criteria Summary](#20-acceptance-criteria-summary)
21. [Defect Classification](#21-defect-classification)
22. [Test Results Log Template](#22-test-results-log-template)

---

## 1. Purpose and Scope

### 1.1 Purpose

This document defines the complete testing and verification strategy for the **ThreadBleMotionDetector\_MaxSonar** firmware application. It provides structured test cases to validate all functional requirements covering sensor integration, motion detection logic, wireless protocol behaviors, GATT service correctness, and system-level stability before release or deployment.

### 1.2 Scope

The test plan covers the following functional domains:

- Firmware build integrity and flash programming
- Boot sequence and subsystem initialization
- GPIO input (commissioning button) and output (LEDs)
- UART interface to the LV-MaxSonar EZ ultrasonic sensor
- Sensor frame parsing and distance calculation
- Motion detection threshold logic and hysteresis
- Bluetooth LE advertising, connection management, and GATT operations
- OpenThread network commissioning, joining, and motion event multicast
- Motion event processing from all trigger sources
- Non-volatile memory persistence across power cycles
- System performance, stability, and memory safety
- BLE security configuration

### 1.3 Out of Scope

- LV-MaxSonar EZ sensor hardware qualification and acoustic calibration
- Internal Cordio BLE stack qualification
- OpenThread certification testing
- Manufacturing (RF) calibration
- Hardware bring-up and schematic validation
- Mobile application (phone side) verification

---

## 2. Reference Documents

| ID | Document | Location |
|----|----------|----------|
| RD-01 | QPG6200L Product Datasheet | `Files/qpg6200L_datasheet.pdf` |
| RD-02 | LV-MaxSonar EZ Datasheet | `Files/11832-LV-MaSonar-EZ_Datasheet.pub.pdf` |
| RD-03 | ThreadBleMotionDetector Technical Overview | `Files/ThreadBleMotionDetector_MaxSonar_Overview.md` |
| RD-04 | IoT Gateway Stack Documentation | `Files/IoT_Gateway_Stack_Complete_Documentation.pdf` |
| RD-05 | Application Source — AppManager.cpp | `Applications/Ble/ThreadBleMotionDetector_MaxSonar/src/AppManager.cpp` |
| RD-06 | Application Source — SensorManager.cpp | `Applications/Ble/ThreadBleMotionDetector_MaxSonar/src/SensorManager.cpp` |
| RD-07 | Application Source — AppTask.cpp | `Applications/Ble/ThreadBleMotionDetector_MaxSonar/src/AppTask.cpp` |
| RD-08 | GATT Configuration | `Applications/Ble/ThreadBleMotionDetector_MaxSonar/src/MotionDetector_Config.c` |
| RD-09 | Build Configuration | `Applications/Ble/ThreadBleMotionDetector_MaxSonar/gen/ThreadBleMotionDetector_MaxSonar_qpg6200/qorvo_config.h` |

---

## 3. Test Environment

### 3.1 Software Tools

| Tool | Version | Purpose |
|------|---------|---------|
| GCC ARM Embedded | 10.3-2021.10 or later | Firmware compilation |
| GNU Make | 4.3 or later | Build system |
| J-Link Commander / nRFgo Studio | Current release | Firmware flash programming |
| nRF Connect for Mobile (Android/iOS) | 4.x or later | BLE scanning, GATT inspection, characteristic reads/writes |
| Wireshark + TI Packet Sniffer | Current release | 802.15.4 (Thread) packet capture |
| OpenThread CLI (Border Router) | Current release | Thread network management and UDP injection |
| Serial terminal (e.g., PuTTY, minicom) | Any | UART log capture at 115200 baud |
| UART simulator / signal generator | Any | Inject synthetic MaxSonar frames for test isolation |
| Python 3 | 3.10 or later | Automated test scripting and frame injection |
| Tape measure / ruler | — | Physical sensor distance calibration |

### 3.2 Test Network Infrastructure

| Component | Configuration |
|-----------|---------------|
| OpenThread Border Router (OTBR) | Raspberry Pi 4, ot-br-posix, dedicated test SSID |
| Thread channel | 15 (default, configurable per test case) |
| Thread PAN ID | 0xABCD (default) |
| Network Key | 16-byte AES-128 key provisioned via BLE |
| UDP listener host | Host PC on OTBR LAN segment, port 5683 |

### 3.3 Test Bench Setup

```
┌──────────────────────────────────────────────────────────────────────┐
│                           Test Bench                                 │
│                                                                      │
│  ┌─────────────────┐  UART1   ┌───────────────────┐                 │
│  │  QPG6200L DK    │◄────────►│ LV-MaxSonar EZ    │                 │
│  │  (DUT)          │ (9600)   │ (GPIO8 RX, GPIO9  │                 │
│  │                 │          │  TX-enable)        │                 │
│  │  GPIO 1,11,12   │          └───────────────────┘                 │
│  │  (LED outputs)  │                                                  │
│  │                 │   BLE    ┌───────────────────┐                 │
│  │                 │◄────────►│ Smartphone         │                 │
│  │                 │          │ nRF Connect        │                 │
│  │                 │ 802.15.4 ├───────────────────┤                 │
│  │                 │◄────────►│ OTBR               │                 │
│  └────────┬────────┘          │ + Packet Sniffer   │                 │
│           │                   └───────────────────┘                 │
│           │ UART0 (115200 log)                                       │
│           ▼                                                          │
│  ┌─────────────────┐                                                 │
│  │  Host PC        │ (log capture, UART frame injector, test scripts)│
│  └─────────────────┘                                                 │
└──────────────────────────────────────────────────────────────────────┘
```

### 3.4 UART Test Injection Setup

For sensor isolation tests (where the actual MaxSonar sensor is replaced by injected frames):

- Connect a USB-to-UART adapter to DK GPIO 8 (RX).
- Use a Python script to inject MaxSonar-format frames: `'R' + 3 ASCII digits + CR`.
- GPIO 9 (TX) held HIGH by firmware — no configuration needed on the injector side.

---

## 4. Hardware Configuration

### 4.1 Device Under Test (DUT)

| Parameter | Value |
|-----------|-------|
| Hardware | QPG6200L Development Kit |
| SoC | QPG6200L (ARM Cortex-M4F, 256 KB RAM, 1 MB Flash) |
| Firmware | ThreadBleMotionDetector\_MaxSonar\_qpg6200.hex |
| Debug interface | J-Link (SWD) |
| UART log port | USB-UART bridge, 115200 baud, 8N1 |

### 4.2 GPIO Assignment Reference

| GPIO | Direction | Function | Test Stimulus / Observation |
|------|-----------|----------|-----------------------------|
| 1 | Output | WHITE\_COOL LED (BLE status) | Observe / logic analyzer |
| 3 | Input | PB1 commissioning button | Manual press or bench signal |
| 8 | Input | UART1 RX (MaxSonar TX) | Serial frame injection |
| 9 | Output | UART1 TX (MaxSonar RX — held HIGH) | Verify HIGH in normal operation |
| 11 | Output | GREEN LED (Thread status) | Observe / logic analyzer |
| 12 | Output | BLUE LED (motion indicator) | Observe / logic analyzer |

### 4.3 Sensor Wiring Reference

| Sensor Pin | DK Connection | Purpose |
|------------|---------------|---------|
| GND | DK GND | Common ground |
| +5V | DK 5V rail | Sensor power (ensure DK 5V present) |
| TX (pin 5) | GPIO 8 | Sensor distance output → UART1 RX |
| RX (pin 4) | GPIO 9 | Held HIGH by firmware to enable free-run mode |

---

## 5. Build Verification

### 5.1 Prerequisites

```bash
# Step 1: Build the OpenThread library (Concurrent Light dependency)
make -f Applications/Concurrent/Light/Makefile.Light_Concurrent_qpg6200

# Step 2: Build the motion detector application
make -f Applications/Ble/ThreadBleMotionDetector_MaxSonar/Makefile.ThreadBleMotionDetector_MaxSonar_qpg6200
```

### 5.2 Expected Build Artifacts

| File | Description |
|------|-------------|
| `ThreadBleMotionDetector_MaxSonar_qpg6200.elf` | Debug ELF with symbols |
| `ThreadBleMotionDetector_MaxSonar_qpg6200.hex` | Signed firmware + bootloader merge |
| `ThreadBleMotionDetector_MaxSonar_qpg6200.map` | Linker memory map |

---

## 6. Test Categories

| Category ID | Name | Priority |
|-------------|------|----------|
| TC-BLD | Build and Flash | Critical |
| TC-PWR | Power-On and Boot | Critical |
| TC-GPIO | GPIO and Buttons | High |
| TC-UART | Sensor UART Interface | Critical |
| TC-SNS | Sensor Logic and Motion Detection | Critical |
| TC-BLE | Bluetooth LE | Critical |
| TC-GATT | GATT Services | Critical |
| TC-THR | OpenThread Networking | Critical |
| TC-MOT | Motion Event Logic | Critical |
| TC-LED | LED Indicators | Medium |
| TC-NVM | NVM Persistence | High |
| TC-PERF | Performance and Stability | High |
| TC-SEC | Security | High |

---

## 7. TC-BLD — Build and Flash Tests

### TC-BLD-001 — Clean Build Succeeds

**Objective:** Verify the firmware compiles without errors from a clean state.

**Preconditions:**
- GCC ARM toolchain installed and on PATH.
- OpenThread dependency library built.

**Steps:**
1. Run `make clean` for the motion detector target.
2. Run the full build command.
3. Inspect terminal output for errors and warnings.

**Expected Results:**
- Build completes with exit code 0.
- No compiler errors present.
- Warnings, if any, are pre-existing and documented.
- All three output artifacts are created.

**Pass Criteria:** Exit code 0, no new warnings, all artifacts present.

---

### TC-BLD-002 — Memory Footprint Within Limits

**Objective:** Verify the firmware fits within QPG6200L flash and RAM limits.

**Steps:**
1. Run `arm-none-eabi-size ThreadBleMotionDetector_MaxSonar_qpg6200.elf`.
2. Record text, data, and bss sizes.

**Expected Results:**

| Region | Limit | Acceptable Usage |
|--------|-------|-----------------|
| Flash (text + rodata) | 1,024 KB | ≤ 960 KB (≤ 93.7%) |
| RAM (data + bss + stack) | 256 KB | ≤ 220 KB (≤ 85.9%) |

**Pass Criteria:** Both regions within defined limits.

---

### TC-BLD-003 — Firmware Flash Programming

**Objective:** Verify the signed hex file programs to the DK without errors.

**Steps:**
1. Connect DK via J-Link.
2. Flash `ThreadBleMotionDetector_MaxSonar_qpg6200.hex`.
3. Perform hard reset.
4. Verify device boots (observe LED activity).

**Expected Results:**
- Flash programming completes with no errors.
- Verification (read-back) passes.
- Device resets and boots into application.

**Pass Criteria:** Programming exit code 0, device boots.

---

### TC-BLD-004 — Post-Build Script Execution

**Objective:** Verify the signing and merge script completes successfully.

**Steps:**
1. Run `ThreadBleMotionDetector_MaxSonar_qpg6200_postbuild.sh`.
2. Confirm exit code 0.
3. Confirm signed hex file is created.

**Expected Results:**
- Script runs without error.
- Output hex includes application and bootloader.

**Pass Criteria:** Script exit code 0, signed hex present.

---

## 8. TC-PWR — Power-On and Boot Tests

### TC-PWR-001 — Cold Boot Initialization

**Objective:** Verify all subsystems initialize correctly on first power-up.

**Steps:**
1. Connect UART log at 115200 baud.
2. Apply power or reset the DK (with MaxSonar sensor connected and powered).
3. Observe UART log and LED states.

**Expected Results:**
- UART log shows initialization of: FreeRTOS, BLE stack, OpenThread stack, UART1 (9600 baud), GPIO configuration.
- WHITE LED blinks at ~1 Hz (BLE advertising).
- GREEN and BLUE LEDs remain off.
- UART log shows first sensor reading within 200 ms of UART1 initialization.
- No assertion or fault messages.

**Pass Criteria:** All subsystems initialized, WHITE LED blinking, sensor reads logged.

---

### TC-PWR-002 — Cold Boot Without Sensor Connected

**Objective:** Verify device boots cleanly when MaxSonar sensor is disconnected.

**Steps:**
1. Disconnect the MaxSonar sensor (leave GPIO 8 floating or pulled high).
2. Boot device.
3. Observe UART log.

**Expected Results:**
- Device boots without crash.
- UART log may show UART timeout or invalid frame events.
- Motion state remains 0x00 (no motion) — no false positives from missing frames.
- BLE and Thread initialization proceed normally.

**Pass Criteria:** Device boots; no crash; no false motion detection.

---

### TC-PWR-003 — Reset Recovery

**Objective:** Verify clean recovery from a software or hardware reset.

**Steps:**
1. Allow device to reach active state (BLE advertising or Thread joined, sensor reading).
2. Press reset button or issue JTAG reset.
3. Observe UART log and LED states.

**Expected Results:**
- Device re-initializes all subsystems.
- Sensor polling resumes within 500 ms.
- No NVM corruption.

**Pass Criteria:** Clean reinitialization; sensor polling resumes.

---

### TC-PWR-004 — Power Cycle with NVM State

**Objective:** Verify device state survives full power removal.

**Preconditions:** Device previously commissioned into a Thread network.

**Steps:**
1. Commission device with Thread credentials; verify Thread join (GREEN solid).
2. Remove power for 5 seconds.
3. Restore power.
4. Observe device behavior.

**Expected Results:**
- Device reads credentials from NVM on boot.
- Thread auto-join begins without BLE provisioning.
- GREEN LED transitions from blinking to solid.

**Pass Criteria:** Auto-join succeeds within 60 seconds.

---

## 9. TC-GPIO — GPIO and Button Tests

### TC-GPIO-001 — PB1 Commissioning Button — BLE Restart

**Objective:** Verify holding PB1 for 2–4 seconds restarts BLE advertising.

**Steps:**
1. Establish a BLE connection (phone connected).
2. Press and hold PB1 for 3 seconds, then release.
3. Observe BLE connection and WHITE LED state.

**Expected Results:**
- BLE connection drops.
- BLE advertising restarts.
- WHITE LED resumes blinking at ~1 Hz.

**Pass Criteria:** BLE advertising restarted after 2–4 s hold.

---

### TC-GPIO-002 — PB1 Commissioning Button — Factory Reset

**Objective:** Verify holding PB1 for ≥ 5 seconds triggers a factory reset.

**Steps:**
1. Commission device with Thread credentials.
2. Press and hold PB1 for 6 seconds, then release.
3. Observe UART log and LED states.
4. Power cycle the device.

**Expected Results:**
- UART log confirms factory reset initiated.
- NVM credentials erased.
- After power cycle, device does NOT auto-join Thread.
- WHITE LED blinks; GREEN LED off.

**Pass Criteria:** Factory reset clears NVM; subsequent boot requires re-commissioning.

---

### TC-GPIO-003 — GPIO 9 Held HIGH (Sensor Enable)

**Objective:** Verify GPIO 9 (UART1 TX) is driven HIGH after initialization, enabling MaxSonar free-run mode.

**Steps:**
1. Boot device with logic analyzer connected to GPIO 9.
2. Observe GPIO 9 level after boot.

**Expected Results:**
- GPIO 9 is HIGH within 100 ms of initialization.
- GPIO 9 remains HIGH during normal operation.

**Pass Criteria:** GPIO 9 stable HIGH in operating state.

---

## 10. TC-UART — Sensor UART Interface Tests

### TC-UART-001 — UART1 Initialization

**Objective:** Verify UART1 is configured with correct parameters (9600 baud, 8N1, GPIO 8 RX / GPIO 9 TX).

**Steps:**
1. Boot device with MaxSonar connected.
2. Capture UART1 traffic with a logic analyzer on GPIO 8.
3. Verify baud rate matches 9600.

**Expected Results:**
- Logic analyzer confirms 9600 baud, 8 data bits, no parity, 1 stop bit.
- First valid frame received within 200 ms of sensor power-up.

**Pass Criteria:** UART1 parameters confirmed at 9600 8N1.

---

### TC-UART-002 — Valid Frame Parsing — Representative Values

**Objective:** Verify correct parsing of standard ASCII distance frames.

**Method:** Use UART injector (no physical sensor required).

**Test Vectors:**

| Injected Frame | Raw Bytes | Expected Inches | Expected cm |
|---------------|-----------|-----------------|-------------|
| `R000\r` | 52 30 30 30 0D | 0 | 0 |
| `R079\r` | 52 30 37 39 0D | 79 | 200 |
| `R078\r` | 52 30 37 38 0D | 78 | 198 |
| `R080\r` | 52 30 38 30 0D | 80 | 203 |
| `R255\r` | 52 32 35 35 0D | 255 | 647 |
| `R765\r` | 52 37 36 35 0D | 765 | 1943 |

**Steps (for each vector):**
1. Inject the frame via UART injector.
2. Observe UART log for parsed distance value.
3. Verify the distance formula: `cm = (inches × 254) / 100` (integer arithmetic).

**Pass Criteria:** All computed cm values match expected within ± 1 cm (integer rounding).

---

### TC-UART-003 — Frame Parser State Machine — Unexpected Characters

**Objective:** Verify the parser rejects malformed frames and resets correctly.

**Test Vectors:**

| Injected Sequence | Description | Expected Behavior |
|-------------------|-------------|-------------------|
| `R07A\r` | Non-digit in digit field | Frame rejected; parser resets |
| `R07\r` | Short frame (only 2 digits) | Incomplete frame; parser resets |
| `079\r` | Missing leading 'R' | Parser ignores bytes (in State 0) |
| `RR079\r` | Double 'R' | Second R resets State 1 back to hunting |
| `\r\r\r` | CR without data | No parse attempt; no crash |
| `R079R080\r` | No CR between frames | Second `R` triggers State 1 reset; only second parsed |

**Steps:**
1. Inject each sequence via UART injector.
2. Observe UART log and verify no false motion event from invalid frames.
3. Verify parser recovers correctly.

**Pass Criteria:** All malformed frames rejected; no crash; parser recovers.

---

### TC-UART-004 — Frame Parser — Stress Test

**Objective:** Verify the parser handles continuous high-rate frames without dropping valid frames or crashing.

**Steps:**
1. Inject 1000 valid frames at MaxSonar rate (~49 ms interval).
2. Alternate between detected range (R060\r = 152 cm, below threshold) and cleared range (R100\r = 254 cm, above threshold).
3. Count motion detected / motion cleared transitions in UART log.

**Expected Results:**
- All 1000 frames parsed.
- Motion transitions logged at expected intervals.
- No crash.

**Pass Criteria:** 1000/1000 frames parsed; correct motion transitions observed.

---

### TC-UART-005 — UART Receive Buffer Handling

**Objective:** Verify no buffer overflow occurs if the application lags behind sensor output.

**Steps:**
1. Inject frames at 5× the normal MaxSonar rate (approx. one frame every 10 ms).
2. Monitor UART log for buffer overflow messages.
3. Verify device stability.

**Expected Results:**
- No crash or hard fault.
- System may drop frames at high rate; this is acceptable.
- UART log shows no corruption of parsed values.

**Pass Criteria:** Device remains stable under 5× overrate; no crash.

---

## 11. TC-SNS — Sensor Logic and Motion Detection Tests

### TC-SNS-001 — Motion Detected Below Threshold (200 cm)

**Objective:** Verify object at ≤ 200 cm triggers motion detected event.

**Threshold:** MOTION\_DISTANCE\_THRESHOLD\_CM = 200

**Steps:**
1. Inject frame `R079\r` (79 inches = 200 cm — boundary value).
2. Observe UART log for motion event.
3. Observe BLUE LED.

**Expected Results:**
- Motion DETECTED event logged.
- BLUE LED turns solid ON.
- Distance reported as 200 cm.

**Pass Criteria:** Motion detected at boundary value (200 cm).

---

### TC-SNS-002 — No Motion Above Threshold

**Objective:** Verify object at > 200 cm does not trigger motion detected.

**Steps:**
1. Inject frame `R080\r` (80 inches = 203 cm — just above threshold).
2. Observe UART log for motion event.
3. Observe BLUE LED.

**Expected Results:**
- No motion detected event logged (or motion cleared if previously detected).
- BLUE LED remains off.

**Pass Criteria:** No false positive at 203 cm.

---

### TC-SNS-003 — Motion Cleared When Object Leaves

**Objective:** Verify motion cleared event fires when distance exceeds threshold.

**Steps:**
1. Inject frame `R060\r` (60 inches = 152 cm) — motion detected.
2. Verify BLUE LED solid ON.
3. Inject frame `R100\r` (100 inches = 254 cm) — object moved away.
4. Observe UART log and BLUE LED.

**Expected Results:**
- Motion CLEARED event logged.
- BLUE LED turns OFF.
- Distance reported as 254 cm.

**Pass Criteria:** Motion cleared event fires when distance exceeds threshold.

---

### TC-SNS-004 — Hysteresis — No Repeated Events While Stationary

**Objective:** Verify no repeated motion events are generated while object remains at same position.

**Steps:**
1. Inject 20 identical frames `R060\r` (152 cm, below threshold).
2. Observe UART log.

**Expected Results:**
- Exactly ONE motion detected event logged.
- No repeated notifications for subsequent identical frames.

**Pass Criteria:** Single event for sustained detection; no event flooding.

---

### TC-SNS-005 — Rapid Distance Oscillation

**Objective:** Verify only genuine state changes generate motion events under rapid oscillation.

**Steps:**
1. Alternately inject `R079\r` (200 cm, border detected) and `R080\r` (203 cm, border cleared) 20 times.
2. Observe UART log for motion event count.

**Expected Results:**
- Up to 20 state changes logged (one per alternation).
- No extra spurious events.
- No crash.

**Pass Criteria:** Event count matches alternation count; no spurious events.

---

### TC-SNS-006 — Zero Distance Edge Case

**Objective:** Verify zero-distance reading is handled without crash.

**Sensor Note:** 0 inches is a sensor error code on most MaxSonar models (no echo received).

**Steps:**
1. Inject frame `R000\r` (0 inches = 0 cm).
2. Observe UART log and motion state.

**Expected Results:**
- Frame parsed without crash.
- Motion state: 0 cm is ≤ 200 threshold but typically indicates no valid echo — verify application documents its behavior.
- BLUE LED behavior is consistent with defined logic (detected if ≤ 200 cm, including 0 cm).

**Pass Criteria:** No crash on 0 cm; behavior is consistent with defined threshold logic.

---

### TC-SNS-007 — Maximum Distance Edge Case

**Objective:** Verify maximum sensor distance (765 inches / 1943 cm) is handled correctly.

**Steps:**
1. Inject frame `R765\r` (765 inches = 1943 cm).
2. Observe UART log and motion state.

**Expected Results:**
- Distance 1943 cm parsed correctly.
- No motion detected (1943 cm >> 200 cm threshold).
- BLUE LED off.

**Pass Criteria:** Max value parsed; no motion trigger.

---

### TC-SNS-008 — Distance Unit Conversion Accuracy

**Objective:** Verify the integer conversion formula `cm = (inches × 254) / 100` is implemented correctly across the sensor range.

**Verification Vectors:**

| Inches | Expected cm | Formula Check |
|--------|------------|---------------|
| 0 | 0 | (0 × 254) / 100 = 0 |
| 1 | 2 | (1 × 254) / 100 = 2 |
| 10 | 25 | (10 × 254) / 100 = 25 |
| 79 | 200 | (79 × 254) / 100 = 200 |
| 100 | 254 | (100 × 254) / 100 = 254 |
| 255 | 647 | (255 × 254) / 100 = 647 |
| 765 | 1943 | (765 × 254) / 100 = 1943 |

**Steps:**
1. Inject each frame via UART injector.
2. Read the Distance characteristic (GATT handle 0x3005) or observe UART log.
3. Compare against expected values.

**Pass Criteria:** All computed values match expected ± 1 cm (integer truncation).

---

## 12. TC-BLE — Bluetooth LE Tests

### TC-BLE-001 — Advertising Presence

**Objective:** Verify the device advertises with the correct device name and service UUIDs.

**Steps:**
1. Boot device.
2. Open nRF Connect and scan.
3. Locate "QPG MaxSonar Motion" in the scan list.
4. Inspect advertising packet.

**Expected Results:**

| Field | Expected Value |
|-------|---------------|
| Device name | "QPG MaxSonar Motion" |
| Advertising flags | 0x06 (LE General Discoverable, BR/EDR Not Supported) |
| Service UUID | 0x180F (Battery Service) |
| Advertising interval | 20–60 ms |

**Pass Criteria:** Device visible with correct name and advertisement data.

---

### TC-BLE-002 — Connection Establishment

**Objective:** Verify BLE connection from smartphone establishes correctly.

**Steps:**
1. Device is advertising.
2. Connect using nRF Connect.
3. Observe WHITE LED and UART log.

**Expected Results:**
- Connection within 3 seconds.
- WHITE LED transitions to solid.
- UART log shows DM\_CONN\_OPEN event.

**Pass Criteria:** Connection established; WHITE LED solid.

---

### TC-BLE-003 — Disconnection Handling

**Objective:** Verify device handles disconnection gracefully and resumes advertising.

**Steps:**
1. Establish BLE connection.
2. Disconnect from nRF Connect.
3. Observe WHITE LED and UART log.

**Expected Results:**
- WHITE LED resumes blinking.
- UART log shows DM\_CONN\_CLOSE event.
- No fault.

**Pass Criteria:** Device resumes advertising after disconnection.

---

### TC-BLE-004 — Advertising Timeout

**Objective:** Verify advertising stops after configured duration (~40 seconds) if no connection made.

**Steps:**
1. Factory reset device.
2. Start timer when WHITE LED begins blinking.
3. Do not connect.
4. Observe WHITE LED after 40 seconds.

**Expected Results:**
- WHITE LED stops blinking after approximately 40 seconds (BLE\_ADV\_BROADCAST\_DURATION = 0xF000).

**Pass Criteria:** Advertising stops within 45 seconds.

---

### TC-BLE-005 — BLE Stable During Active Sensor Polling

**Objective:** Verify BLE throughput is not degraded by concurrent UART sensor polling (50 ms interval).

**Steps:**
1. Establish BLE connection.
2. Enable notifications on Motion Status and Distance characteristics.
3. Inject rapid sensor frames at 50 ms interval.
4. Measure BLE notification delivery latency.

**Expected Results:**
- Notifications delivered within 200 ms of state change.
- No BLE connection drops due to sensor polling load.

**Pass Criteria:** BLE notifications delivered with ≤ 200 ms latency under polling load.

---

## 13. TC-GATT — GATT Service Tests

### TC-GATT-001 — GATT Service Discovery

**Objective:** Verify all expected GATT services and characteristics are discoverable.

**Steps:**
1. Connect via nRF Connect.
2. Perform full GATT discovery.
3. Verify all services and characteristics against the expected table.

**Expected Results:**

| Service | UUID | Handle |
|---------|------|--------|
| Battery Service | 0x180F | — |
| └─ Battery Level | 0x2A19 | 0x2002 |
| Motion Detection Service | 4D4F4E00-... | — |
| └─ Motion Status | Custom | 0x3002 |
| └─ Distance | Custom | 0x3005 |
| Thread Config Service | D00RBELL-0002-... | — |
| └─ Network Name | Custom | 0x4002 |
| └─ Network Key | Custom | 0x4004 |
| └─ Channel | Custom | 0x4006 |
| └─ PAN ID | Custom | 0x4008 |
| └─ Join | Custom | 0x400A |
| └─ Thread Status | Custom | 0x400C |

**Pass Criteria:** All services and characteristics present with correct properties.

---

### TC-GATT-002 — Battery Level Read

**Objective:** Verify Battery Level characteristic returns 100.

**Steps:**
1. Connect via nRF Connect.
2. Read handle 0x2002.

**Expected Results:**
- Value: `0x64` (100 decimal).

**Pass Criteria:** Value is 100.

---

### TC-GATT-003 — Motion Status Read (Idle)

**Objective:** Verify Motion Status reads 0x00 when no motion is detected.

**Steps:**
1. Ensure no object within 200 cm of sensor.
2. Connect via nRF Connect.
3. Read handle 0x3002.

**Expected Results:**
- Value: `0x00` (no motion).

**Pass Criteria:** Idle value is 0x00.

---

### TC-GATT-004 — Motion Status Notification on Detection

**Objective:** Verify Motion Status notifications are sent on detection and clearance.

**Steps:**
1. Connect via nRF Connect.
2. Enable notifications on handle 0x3002 (write `0x0001` to handle 0x3003).
3. Inject frame `R060\r` (motion detected).
4. Inject frame `R100\r` (motion cleared).
5. Observe notifications.

**Expected Results:**
- Notification `0x01` received on motion detection.
- Notification `0x00` received on motion clearance.

**Pass Criteria:** Both notifications received correctly.

---

### TC-GATT-005 — Distance Characteristic Read

**Objective:** Verify Distance characteristic reflects the current sensor reading.

**Steps:**
1. Connect via nRF Connect.
2. Inject frame `R079\r` (200 cm).
3. Read handle 0x3005.

**Expected Results:**
- Value: `0x00 0xC8` (200 = 0x00C8 big-endian).

**Pass Criteria:** Distance value matches injected reading.

---

### TC-GATT-006 — Distance Characteristic Notification

**Objective:** Verify Distance notifications are sent when distance changes.

**Steps:**
1. Connect via nRF Connect.
2. Enable notifications on handle 0x3005 (write `0x0001` to handle 0x3006).
3. Inject several distance frames with different values.
4. Observe notifications.

**Expected Results:**
- Notifications received for each motion state change.
- Distance bytes correct for each injected reading (big-endian, cm).

**Pass Criteria:** Distance notifications correct; big-endian encoding verified.

---

### TC-GATT-007 — Remote Motion Override via BLE Write

**Objective:** Verify writing 0x01 to Motion Status triggers a motion event (for testing purposes).

**Steps:**
1. Connect via nRF Connect.
2. Enable notifications on handle 0x3002.
3. Write `0x01` to handle 0x3002.
4. Observe UART log and BLUE LED.

**Expected Results:**
- Motion detected event logged.
- BLUE LED turns solid ON.
- Thread multicast sent (if Thread is joined).

**Pass Criteria:** BLE override triggers motion event.

---

### TC-GATT-008 — Thread Config Service — Network Key Write-Only

**Objective:** Verify the Network Key characteristic is write-only.

**Steps:**
1. Connect via nRF Connect.
2. Attempt to read handle 0x4004.
3. Write 16 bytes to handle 0x4004.

**Expected Results:**
- Read returns ATT error (Read Not Permitted).
- Write completes with ATT success.

**Pass Criteria:** Read rejected; write accepted.

---

### TC-GATT-009 — Thread Config Service — Channel Boundary Values

**Objective:** Verify the Channel characteristic enforces 802.15.4 channel range (11–26).

**Steps:**
1. Write `0x0B` (11) → confirm success.
2. Write `0x1A` (26) → confirm success.
3. Write `0x0A` (10) → expect ATT error.
4. Write `0x1B` (27) → expect ATT error.

**Pass Criteria:** Boundary values enforced correctly.

---

### TC-GATT-010 — Thread Status Notification on Join

**Objective:** Verify Thread Status notifications are sent during role transitions.

**Steps:**
1. Connect via nRF Connect.
2. Enable CCCD on handle 0x400C.
3. Provision credentials and write 0x01 to Join handle.
4. Observe notifications.

**Expected Results:**
- Notification `0x01` (detached) on join start.
- Notification `0x02`/`0x03` (child/router) when joined.

**Pass Criteria:** Role transition notifications received.

---

## 14. TC-THR — OpenThread Network Tests

### TC-THR-001 — BLE Commissioning Workflow

**Objective:** Verify the full BLE commissioning workflow joins the device to a Thread network.

**Steps:**
1. Start from factory reset state.
2. Connect phone via BLE.
3. Write valid Thread credentials to all Thread Config characteristics.
4. Write `0x01` to Join handle (0x400A).
5. Monitor Thread Status notifications and GREEN LED.

**Expected Results:**
- GREEN LED transitions blinking → solid.
- Thread Status: `0x00` → `0x01` → `0x02`/`0x03`.
- OTBR dashboard shows device joined.

**Pass Criteria:** Device joins Thread network.

---

### TC-THR-002 — Thread Network Auto-Rejoin After Reset

**Objective:** Verify device auto-joins Thread on reset using NVM credentials.

**Steps:**
1. Commission and join Thread network.
2. Press reset button.
3. Observe GREEN LED without BLE interaction.

**Expected Results:**
- GREEN LED transitions to solid within 60 seconds.
- OTBR dashboard confirms rejoin.

**Pass Criteria:** Auto-rejoin within 60 seconds.

---

### TC-THR-003 — Motion Event Multicast Transmission

**Objective:** Verify a motion event sends a valid UDP multicast to ff03::1 on port 5683.

**Preconditions:** Device joined to Thread network. UDP listener running on host at port 5683.

**Steps:**
1. Inject frame `R060\r` (152 cm, motion detected).
2. Capture 802.15.4 packets with Wireshark.
3. Observe UDP listener on host PC.

**Expected Results:**
- UDP packet received on host from device's Thread IPv6 address.
- Destination: `ff03::1`, port 5683.
- Payload byte 0: `0x01` (motion event type).
- Payload byte 1: `0x01` (detected).
- Payload bytes 2–3: `0x00 0x98` (152 cm = 0x0098, big-endian).

**Pass Criteria:** Correct multicast payload received on UDP listener.

---

### TC-THR-004 — Motion Cleared Multicast Transmission

**Objective:** Verify a motion cleared event sends correct UDP multicast.

**Steps:**
1. Inject frame `R100\r` (254 cm, motion cleared).
2. Observe UDP listener on host PC.

**Expected Results:**
- Payload byte 0: `0x01`.
- Payload byte 1: `0x00` (cleared).
- Payload bytes 2–3: `0x00 0xFE` (254 cm = 0x00FE, big-endian).

**Pass Criteria:** Correct payload for cleared state.

---

### TC-THR-005 — Motion Multicast Reception and Relay

**Objective:** Verify motion multicast received from another Thread device triggers local motion event.

**Steps:**
1. From OTBR host, inject UDP packet to device: `{0x01, 0x01, 0x00, 0x78}` (detected at 120 cm).
2. Observe DUT UART log and BLUE LED.

**Expected Results:**
- DUT logs "Motion event received from Thread network".
- DUT BLUE LED turns solid ON.
- DUT does not re-broadcast (loop prevention).

**Pass Criteria:** Remote event processed locally; no re-broadcast.

---

### TC-THR-006 — Thread Role Stability

**Objective:** Verify stable Thread role over 30 minutes.

**Steps:**
1. Join Thread network with sensor active.
2. Run sensor injecting frames every 50 ms for 30 minutes.
3. Monitor UART log and GREEN LED.

**Expected Results:**
- GREEN LED remains solid.
- No unexpected role drop.

**Pass Criteria:** No role drops in 30 minutes.

---

### TC-THR-007 — Invalid Commissioning Credentials

**Objective:** Verify device handles invalid Thread credentials gracefully.

**Steps:**
1. Connect via BLE.
2. Write invalid/incomplete Network Key.
3. Write 0x01 to Join handle.
4. Observe UART log.

**Expected Results:**
- No crash.
- UART log shows error or detached state.
- Thread Status notification: `0x01` or error.

**Pass Criteria:** Invalid credentials handled without crash.

---

## 15. TC-MOT — Motion Event Logic Tests

### TC-MOT-001 — Motion from All Three Sources

**Objective:** Verify motion events from all trigger sources produce identical behavior.

**Sources:**
- Source A: Physical object within 200 cm of sensor (or injected frame R060\r)
- Source B: BLE write `0x01` to Motion Status handle 0x3002
- Source C: UDP multicast injection from OTBR host `{0x01, 0x01, 0x00, 0x78}`

**Steps (for each source):**
1. Trigger the motion event from that source.
2. Observe BLUE LED, UART log, and GATT notifications.

**Expected Results for each source:**
- BLUE LED turns solid ON.
- Motion Status GATT value updated to 0x01.
- GATT notification sent (if CCCD enabled).
- UART log identifies source.

**Pass Criteria:** All three sources produce identical system response.

---

### TC-MOT-002 — Motion Cleared from Sensor

**Objective:** Verify only the sensor (not BLE or Thread) clears motion automatically.

**Steps:**
1. Trigger motion via BLE write.
2. Observe BLUE LED (should be solid ON).
3. Inject a sensor frame `R100\r` (254 cm — no motion from sensor).
4. Observe BLUE LED and GATT notification.

**Expected Results:**
- Motion cleared event triggered by sensor reading.
- BLUE LED turns OFF.
- Motion Status notification `0x00` sent.

**Note:** If BLE and Thread sources also clear motion, verify that behavior is intentional and consistent.

**Pass Criteria:** Motion cleared by sensor reading after BLE trigger.

---

### TC-MOT-003 — Loop Prevention on Received Thread Multicast

**Objective:** Verify device does not re-broadcast motion event received from Thread mesh.

**Steps:**
1. Inject UDP motion event from OTBR host to device.
2. Monitor 802.15.4 sniffer for any re-multicast from DUT.

**Expected Results:**
- DUT processes event locally.
- DUT does not emit new UDP multicast within 1 second.

**Pass Criteria:** No re-broadcast observed.

---

### TC-MOT-004 — No Notification Without CCCD Enabled

**Objective:** Verify no BLE notification sent when CCCD is not enabled.

**Steps:**
1. Connect via nRF Connect.
2. Do NOT enable notifications on any characteristic.
3. Inject motion detected frame.
4. Monitor for unsolicited BLE notifications.

**Expected Results:**
- No BLE notifications observed.
- Motion event still processed locally (LED, Thread multicast).

**Pass Criteria:** No unsolicited notifications.

---

### TC-MOT-005 — Thread Multicast Not Sent When Thread Detached

**Objective:** Verify no Thread UDP multicast is sent if the device is not joined to Thread.

**Steps:**
1. Factory reset device (Thread not joined).
2. Inject motion detected frame.
3. Monitor 802.15.4 sniffer.

**Expected Results:**
- BLUE LED turns ON (local motion processing).
- No 802.15.4 packet transmitted.
- UART log indicates "Thread not joined, skipping multicast" or similar.

**Pass Criteria:** No Thread packet sent when not joined.

---

## 16. TC-LED — LED Indicator Tests

### TC-LED-001 — WHITE LED BLE Advertising Pattern

**Objective:** Verify WHITE LED blinks at 500 ms on / 500 ms off during BLE advertising.

**Steps:**
1. Boot device.
2. Time WHITE LED blink period with logic analyzer.

**Expected Results:**
- Period: 1000 ms ± 100 ms.

**Pass Criteria:** Timing within 10% of 1 Hz.

---

### TC-LED-002 — WHITE LED Solid When BLE Connected

**Objective:** Verify WHITE LED is solid on BLE connection.

**Steps:**
1. Connect phone via BLE.
2. Observe WHITE LED.

**Expected Results:**
- WHITE LED transitions to solid ON immediately on connection.

**Pass Criteria:** WHITE LED solid on connection.

---

### TC-LED-003 — GREEN LED Thread Joining Pattern

**Objective:** Verify GREEN LED blinks at 200 ms on / 200 ms off while joining Thread.

**Steps:**
1. Initiate Thread join.
2. Observe GREEN LED during joining phase.

**Expected Results:**
- GREEN LED blinks at 200 ms period ± 50 ms.

**Pass Criteria:** Timing within 25% of 5 Hz.

---

### TC-LED-004 — GREEN LED Solid When Thread Attached

**Objective:** Verify GREEN LED is solid after Thread join.

**Steps:**
1. Commission device and allow Thread join.
2. Observe GREEN LED after role change logged.

**Expected Results:**
- GREEN LED solid ON.

**Pass Criteria:** GREEN LED solid after attach.

---

### TC-LED-005 — BLUE LED Solid ON During Motion Detection

**Objective:** Verify BLUE LED is solid ON (not blinking) when motion is detected.

**Steps:**
1. Inject frame `R060\r` (motion detected).
2. Observe BLUE LED with logic analyzer.

**Expected Results:**
- BLUE LED goes solid ON and remains constant.
- No blinking pattern.

**Pass Criteria:** BLUE LED solid during motion.

---

### TC-LED-006 — BLUE LED OFF When Motion Cleared

**Objective:** Verify BLUE LED turns OFF when motion is cleared.

**Steps:**
1. Start with motion detected (BLUE LED ON).
2. Inject frame `R100\r` (motion cleared).
3. Observe BLUE LED.

**Expected Results:**
- BLUE LED turns OFF immediately.

**Pass Criteria:** BLUE LED OFF on motion clearance.

---

## 17. TC-NVM — NVM Persistence Tests

### TC-NVM-001 — Thread Credentials Persist Across Reset

**Objective:** Verify Thread credentials written via BLE are retained after a reset.

**Steps:**
1. Commission with known credentials and verify Thread join.
2. Perform hardware reset.
3. Observe auto-rejoin without BLE interaction.

**Expected Results:**
- GREEN LED goes solid within 60 seconds.

**Pass Criteria:** Credentials persist; auto-rejoin succeeds.

---

### TC-NVM-002 — Factory Reset Clears Credentials

**Objective:** Verify factory reset erases NVM credentials.

**Steps:**
1. Commission and verify Thread join.
2. Hold PB1 for 6 seconds.
3. Power cycle device.
4. Observe boot behavior.

**Expected Results:**
- Device does not auto-join Thread.
- WHITE LED blinks; GREEN LED off.
- Requires re-commissioning.

**Pass Criteria:** NVM cleared; re-commissioning required.

---

### TC-NVM-003 — NVM Integrity After Multiple Writes

**Objective:** Verify NVM integrity after 10 successive commissioning cycles.

**Steps:**
1. Perform 10 complete cycles: commission → join → factory reset.
2. On 11th cycle, verify normal commissioning still functions.

**Expected Results:**
- All 10 cycles successful.
- 11th cycle completes without error.
- No NVM wear indicators in UART log.

**Pass Criteria:** All 11 cycles successful.

---

## 18. TC-PERF — Performance and Stability Tests

### TC-PERF-001 — 24-Hour Continuous Operation

**Objective:** Verify system stability over 24 hours with active sensor polling and periodic motion events.

**Steps:**
1. Commission device and join Thread network.
2. Run sensor injecting alternating in-range / out-of-range frames every 30 seconds.
3. Monitor UART log continuously for 24 hours.

**Expected Results:**
- No crash, assertion, or watchdog reset.
- All motion events processed.
- BLE and Thread remain operational.

**Pass Criteria:** Zero unplanned resets; all events processed.

---

### TC-PERF-002 — BLE Reconnection Under Load

**Objective:** Verify 50 disconnect/reconnect cycles do not degrade performance.

**Steps:**
1. Commission device with active sensor.
2. Connect via BLE, disconnect, reconnect — repeat 50 times.
3. Verify full functionality after 50 cycles.

**Expected Results:**
- All 50 reconnections successful.
- GATT operates normally after 50 cycles.
- Sensor polling unaffected.

**Pass Criteria:** 50/50 reconnections; full functionality retained.

---

### TC-PERF-003 — Motion Event Latency

**Objective:** Verify end-to-end latency from sensor frame receipt to BLUE LED onset.

**Steps:**
1. Use logic analyzer to capture GPIO 8 (UART RX frame start) and GPIO 12 (BLUE LED onset).
2. Inject 10 motion detected frames.
3. Measure delay from end of UART frame to BLUE LED assertion.

**Expected Results:**
- Latency: ≤ 150 ms (50 ms poll interval + UART processing + FreeRTOS dispatch).

**Pass Criteria:** All 10 measurements ≤ 150 ms.

---

### TC-PERF-004 — Memory Leak Detection

**Objective:** Verify no heap memory leak over 1 hour of BLE connections and sensor polling.

**Steps:**
1. Note initial heap free space from boot log.
2. Connect and disconnect BLE every 30 seconds for 1 hour while sensor polling active.
3. Compare heap free space after 1 hour.

**Expected Results:**
- Heap delta ≤ 5% over 1 hour.

**Pass Criteria:** Heap stable.

---

### TC-PERF-005 — FreeRTOS Event Queue Load Test

**Objective:** Verify event queue (capacity 20) does not overflow under combined sensor and BLE load.

**Steps:**
1. Inject motion frames at maximum rate.
2. Simultaneously trigger BLE GATT writes.
3. Monitor UART log for queue overflow messages.

**Expected Results:**
- No queue overflow message.
- System remains stable.

**Pass Criteria:** No queue overflow under combined load.

---

### TC-PERF-006 — Sensor Polling CPU Load

**Objective:** Verify sensor polling (50 ms interval) does not starve higher-priority tasks.

**Steps:**
1. Monitor BLE connection stability during active sensor polling.
2. Confirm BLE characteristic read/write latency is not significantly increased.

**Expected Results:**
- BLE read/write latency within 200 ms under polling load.
- No BLE timeout due to sensor polling.

**Pass Criteria:** BLE latency ≤ 200 ms under polling load.

---

## 19. TC-SEC — Security Tests

### TC-SEC-001 — No Encryption Enforcement (Known Limitation)

**Objective:** Document and confirm the device operates without BLE pairing or encryption.

**Steps:**
1. Connect via any BLE central without pairing credentials.
2. Read and write characteristics freely.

**Expected Results:**
- Connection succeeds without pairing challenge.
- All characteristics accessible per DM\_SEC\_LEVEL\_NONE configuration.

**Note:** Known design choice for the demo application. Production deployments must enable BLE encryption.

**Pass Criteria:** Behavior matches documented security level.

---

### TC-SEC-002 — Thread Network Key Not Readable

**Objective:** Verify Thread Network Key cannot be read back via BLE.

**Steps:**
1. Connect via BLE.
2. Write a known 16-byte key to handle 0x4004.
3. Attempt to read handle 0x4004.

**Expected Results:**
- Read returns ATT error (Read Not Permitted).
- Key bytes never returned in any ATT response.

**Pass Criteria:** Key read blocked at ATT layer.

---

### TC-SEC-003 — Firmware Signature Verification

**Objective:** Verify bootloader rejects firmware with invalid ECDSA P-256 signature.

**Steps:**
1. Corrupt last 64 bytes of signed hex.
2. Attempt to flash corrupted image.
3. Observe bootloader behavior on reset.

**Expected Results:**
- Bootloader detects signature mismatch.
- Device does not boot into corrupted application.

**Pass Criteria:** Corrupted image rejected by bootloader.

---

### TC-SEC-004 — Unauthorized Join Write Without Credentials

**Objective:** Verify writing to Join handle without prior credential provisioning does not crash the device.

**Steps:**
1. Factory reset device.
2. Connect via BLE without writing any Thread credentials.
3. Write 0x01 to Join handle (0x400A).
4. Observe UART log.

**Expected Results:**
- No crash or assertion.
- UART log shows graceful error.
- Thread Status notification returns 0x00 or 0x01 with error.

**Pass Criteria:** No crash; error handled gracefully.

---

### TC-SEC-005 — Sensor Data Injection Prevention

**Objective:** Verify that injected UART data cannot cause unintended system actions beyond defined motion detection thresholds.

**Steps:**
1. Inject unusual binary patterns on UART1 RX (e.g., null bytes, control characters, overlong sequences).
2. Observe UART log and system stability.

**Expected Results:**
- Parser rejects all non-conforming frames.
- No crash, buffer overflow, or unintended state change.
- System recovers to normal parsing within 2 seconds.

**Pass Criteria:** No crash; parser recovers cleanly.

---

## 20. Acceptance Criteria Summary

For a build to be considered **production-ready**, all of the following conditions must be met:

| Condition | Requirement |
|-----------|-------------|
| TC-BLD tests | All pass |
| TC-PWR-001, TC-PWR-002 | Both pass |
| TC-GPIO tests | All pass |
| TC-UART-001, TC-UART-002, TC-UART-003 | All pass |
| TC-SNS tests | All pass |
| TC-BLE tests | All pass |
| TC-GATT tests | All pass |
| TC-THR-001 through TC-THR-005 | All pass |
| TC-MOT tests | All pass |
| TC-LED tests | All pass |
| TC-NVM tests | All pass |
| TC-PERF-001 | 24-hour test passes |
| TC-PERF-002 | 50-cycle BLE reconnect test passes |
| TC-SEC-002 | Network key not readable |
| TC-SEC-003 | Bootloader rejects invalid signature |
| Open Critical defects | Zero |
| Open High defects | Zero |

---

## 21. Defect Classification

| Severity | Definition | Example |
|----------|------------|---------|
| **Critical** | System crash, data loss, security breach, functionality completely broken | Hard fault on motion event, UART buffer overflow causing crash |
| **High** | Major feature non-functional; no acceptable workaround | Sensor never detected, BLE commissioning fails, Thread never joins |
| **Medium** | Feature degraded; workaround exists | Threshold off by 5 cm, LED timing slightly off, notification delayed |
| **Low** | Cosmetic or minor deviation; no user impact | Log message typo, minor timing discrepancy |

---

## 22. Test Results Log Template

```
Application:   ThreadBleMotionDetector_MaxSonar
Firmware:      ThreadBleMotionDetector_MaxSonar_qpg6200.hex
Build Hash:    [SHA-256 of .hex file]
DK Serial:     [hardware serial number]
Sensor Model:  LV-MaxSonar EZ [variant, e.g., EZ1, EZ2, EZ4]
Tester:        [name]
Date:          [YYYY-MM-DD]
Environment:   [OTBR version, nRF Connect version, tool versions]

┌──────────────┬────────────────────────────────────────┬────────────┬──────────┐
│ Test Case ID │ Test Name                              │ Result     │ Notes    │
├──────────────┼────────────────────────────────────────┼────────────┼──────────┤
│ TC-BLD-001   │ Clean Build Succeeds                   │ PASS / FAIL│          │
│ TC-BLD-002   │ Memory Footprint                       │ PASS / FAIL│          │
│ TC-BLD-003   │ Firmware Flash Programming             │ PASS / FAIL│          │
│ TC-BLD-004   │ Post-Build Script                      │ PASS / FAIL│          │
│ TC-PWR-001   │ Cold Boot Initialization               │ PASS / FAIL│          │
│ TC-PWR-002   │ Cold Boot Without Sensor               │ PASS / FAIL│          │
│ TC-PWR-003   │ Reset Recovery                         │ PASS / FAIL│          │
│ TC-PWR-004   │ Power Cycle with NVM State             │ PASS / FAIL│          │
│ TC-GPIO-001  │ PB1 BLE Restart                        │ PASS / FAIL│          │
│ TC-GPIO-002  │ PB1 Factory Reset                      │ PASS / FAIL│          │
│ TC-GPIO-003  │ GPIO 9 Held HIGH                       │ PASS / FAIL│          │
│ TC-UART-001  │ UART1 Initialization                   │ PASS / FAIL│          │
│ TC-UART-002  │ Valid Frame Parsing                    │ PASS / FAIL│          │
│ TC-UART-003  │ Malformed Frame Rejection              │ PASS / FAIL│          │
│ TC-UART-004  │ Frame Parser Stress Test               │ PASS / FAIL│          │
│ TC-UART-005  │ UART Receive Buffer Handling           │ PASS / FAIL│          │
│ TC-SNS-001   │ Motion Detected Below Threshold        │ PASS / FAIL│          │
│ TC-SNS-002   │ No Motion Above Threshold              │ PASS / FAIL│          │
│ TC-SNS-003   │ Motion Cleared When Object Leaves      │ PASS / FAIL│          │
│ TC-SNS-004   │ Hysteresis — No Repeated Events        │ PASS / FAIL│          │
│ TC-SNS-005   │ Rapid Distance Oscillation             │ PASS / FAIL│          │
│ TC-SNS-006   │ Zero Distance Edge Case                │ PASS / FAIL│          │
│ TC-SNS-007   │ Maximum Distance Edge Case             │ PASS / FAIL│          │
│ TC-SNS-008   │ Distance Unit Conversion Accuracy      │ PASS / FAIL│          │
│ TC-BLE-001   │ Advertising Presence                   │ PASS / FAIL│          │
│ TC-BLE-002   │ Connection Establishment               │ PASS / FAIL│          │
│ TC-BLE-003   │ Disconnection Handling                 │ PASS / FAIL│          │
│ TC-BLE-004   │ Advertising Timeout                    │ PASS / FAIL│          │
│ TC-BLE-005   │ BLE Stable Under Sensor Load           │ PASS / FAIL│          │
│ TC-GATT-001  │ GATT Service Discovery                 │ PASS / FAIL│          │
│ TC-GATT-002  │ Battery Level Read                     │ PASS / FAIL│          │
│ TC-GATT-003  │ Motion Status Read (Idle)              │ PASS / FAIL│          │
│ TC-GATT-004  │ Motion Status Notification             │ PASS / FAIL│          │
│ TC-GATT-005  │ Distance Characteristic Read           │ PASS / FAIL│          │
│ TC-GATT-006  │ Distance Notification                  │ PASS / FAIL│          │
│ TC-GATT-007  │ Remote Motion Override via BLE Write   │ PASS / FAIL│          │
│ TC-GATT-008  │ Network Key Write-Only                 │ PASS / FAIL│          │
│ TC-GATT-009  │ Channel Boundary Values                │ PASS / FAIL│          │
│ TC-GATT-010  │ Thread Status Notification on Join     │ PASS / FAIL│          │
│ TC-THR-001   │ BLE Commissioning Workflow             │ PASS / FAIL│          │
│ TC-THR-002   │ Auto-Rejoin After Reset                │ PASS / FAIL│          │
│ TC-THR-003   │ Motion Detected Multicast              │ PASS / FAIL│          │
│ TC-THR-004   │ Motion Cleared Multicast               │ PASS / FAIL│          │
│ TC-THR-005   │ Multicast Reception and Relay          │ PASS / FAIL│          │
│ TC-THR-006   │ Thread Role Stability                  │ PASS / FAIL│          │
│ TC-THR-007   │ Invalid Commissioning Credentials      │ PASS / FAIL│          │
│ TC-MOT-001   │ Motion from All Three Sources          │ PASS / FAIL│          │
│ TC-MOT-002   │ Motion Cleared from Sensor             │ PASS / FAIL│          │
│ TC-MOT-003   │ Loop Prevention on Thread Multicast    │ PASS / FAIL│          │
│ TC-MOT-004   │ No Notification Without CCCD           │ PASS / FAIL│          │
│ TC-MOT-005   │ No Multicast When Thread Detached      │ PASS / FAIL│          │
│ TC-LED-001   │ WHITE LED Advertising Pattern          │ PASS / FAIL│          │
│ TC-LED-002   │ WHITE LED Solid When Connected         │ PASS / FAIL│          │
│ TC-LED-003   │ GREEN LED Thread Joining Pattern       │ PASS / FAIL│          │
│ TC-LED-004   │ GREEN LED Solid When Attached          │ PASS / FAIL│          │
│ TC-LED-005   │ BLUE LED Solid During Motion           │ PASS / FAIL│          │
│ TC-LED-006   │ BLUE LED OFF When Motion Cleared       │ PASS / FAIL│          │
│ TC-NVM-001   │ Thread Credentials Persist             │ PASS / FAIL│          │
│ TC-NVM-002   │ Factory Reset Clears Credentials       │ PASS / FAIL│          │
│ TC-NVM-003   │ NVM Integrity Multiple Writes          │ PASS / FAIL│          │
│ TC-PERF-001  │ 24-Hour Continuous Operation           │ PASS / FAIL│          │
│ TC-PERF-002  │ BLE Reconnection Under Load            │ PASS / FAIL│          │
│ TC-PERF-003  │ Motion Event Latency                   │ PASS / FAIL│          │
│ TC-PERF-004  │ Memory Leak Detection                  │ PASS / FAIL│          │
│ TC-PERF-005  │ FreeRTOS Event Queue Load              │ PASS / FAIL│          │
│ TC-PERF-006  │ Sensor Polling CPU Load                │ PASS / FAIL│          │
│ TC-SEC-001   │ No Encryption (Known Limitation)       │ PASS / FAIL│          │
│ TC-SEC-002   │ Network Key Not Readable               │ PASS / FAIL│          │
│ TC-SEC-003   │ Firmware Signature Verification        │ PASS / FAIL│          │
│ TC-SEC-004   │ Unauthorized Join Write                │ PASS / FAIL│          │
│ TC-SEC-005   │ Sensor Data Injection Prevention       │ PASS / FAIL│          │
└──────────────┴────────────────────────────────────────┴────────────┴──────────┘

Overall Result: [ ] PASS   [ ] FAIL   [ ] PARTIAL

Defects Found:
1. [ID] [Severity] [Description]
...

Sign-off:
Tester Signature: _________________ Date: _________
Review Signature: _________________ Date: _________
```

---

*End of ThreadBleMotionDetector\_MaxSonar Testing and Verification Plan*
