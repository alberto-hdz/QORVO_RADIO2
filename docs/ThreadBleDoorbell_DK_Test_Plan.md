# ThreadBleDoorbell\_DK — Testing and Verification Plan

**Document Version:** 1.0
**Application:** ThreadBleDoorbell\_DK
**Target Platform:** QPG6200L Development Kit
**Firmware Designation:** ThreadBleDoorbell\_DK\_qpg6200
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
10. [TC-BLE — Bluetooth LE Tests](#10-tc-ble--bluetooth-le-tests)
11. [TC-GATT — GATT Service Tests](#11-tc-gatt--gatt-service-tests)
12. [TC-THR — OpenThread Network Tests](#12-tc-thr--openthread-network-tests)
13. [TC-RING — Doorbell Ring Logic Tests](#13-tc-ring--doorbell-ring-logic-tests)
14. [TC-LED — LED Indicator Tests](#14-tc-led--led-indicator-tests)
15. [TC-NVM — NVM Persistence Tests](#15-tc-nvm--nvm-persistence-tests)
16. [TC-PERF — Performance and Stability Tests](#16-tc-perf--performance-and-stability-tests)
17. [TC-SEC — Security Tests](#17-tc-sec--security-tests)
18. [Acceptance Criteria Summary](#18-acceptance-criteria-summary)
19. [Defect Classification](#19-defect-classification)
20. [Test Results Log Template](#20-test-results-log-template)

---

## 1. Purpose and Scope

### 1.1 Purpose

This document defines the complete testing and verification strategy for the **ThreadBleDoorbell\_DK** firmware application. It provides structured test cases to validate all functional requirements, hardware interactions, wireless protocol behaviors, and system-level characteristics of the application before release or integration into a larger IoT deployment.

### 1.2 Scope

The test plan covers the following functional domains:

- Firmware build integrity and flash programming
- Boot sequence and hardware initialization
- GPIO input (push buttons) and output (LEDs)
- Bluetooth LE advertising, connection management, and GATT operations
- OpenThread network commissioning, joining, and mesh communication
- Doorbell ring event processing from all three trigger sources
- Non-volatile memory persistence across power cycles
- System performance, stability, and memory safety
- BLE security configuration

### 1.3 Out of Scope

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
| RD-02 | ThreadBleDoorbell\_DK Technical Overview | `Files/ThreadBleDoorbell_DK_Overview.md` |
| RD-03 | IoT Gateway Stack Documentation | `Files/IoT_Gateway_Stack_Complete_Documentation.pdf` |
| RD-04 | Application Source — AppManager.cpp | `Applications/Ble/ThreadBleDoorbell_DK/src/AppManager.cpp` |
| RD-05 | Application Source — DoorbellManager.cpp | `Applications/Ble/ThreadBleDoorbell_DK/src/DoorbellManager.cpp` |
| RD-06 | Application Source — AppTask.cpp | `Applications/Ble/ThreadBleDoorbell_DK/src/AppTask.cpp` |
| RD-07 | GATT Configuration | `Applications/Ble/ThreadBleDoorbell_DK/src/ThreadBleDoorbell_Config.c` |
| RD-08 | Build Configuration | `Applications/Ble/ThreadBleDoorbell_DK/gen/ThreadBleDoorbell_DK_qpg6200/qorvo_config.h` |

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
| OpenThread CLI (Border Router) | Current release | Thread network management and testing |
| Serial terminal (e.g., PuTTY, minicom) | Any | UART log capture at 115200 baud |
| Python 3 | 3.10 or later | Automated test scripting |

### 3.2 Test Network Infrastructure

| Component | Configuration |
|-----------|---------------|
| OpenThread Border Router (OTBR) | Raspberry Pi 4, ot-br-posix, configured on a dedicated test SSID |
| Thread channel | 15 (default, configurable per test case) |
| Thread PAN ID | 0xABCD (default) |
| Network Key | 16-byte AES-128 key provisioned via BLE during commissioning tests |
| UDP listener host | Host PC on OTBR LAN segment, port 5683 |

### 3.3 Test Bench Setup

```
┌──────────────────────────────────────────────────────────────────┐
│                         Test Bench                               │
│                                                                  │
│  ┌─────────────┐      BLE     ┌──────────────┐                  │
│  │ QPG6200L DK │◄────────────►│ Smartphone   │                  │
│  │ (DUT)       │              │ nRF Connect  │                  │
│  │             │  802.15.4    ├──────────────┤                  │
│  │   GPIO 5    │◄────────────►│ OTBR         │                  │
│  │   GPIO 1,11 │              │ + Sniffer    │                  │
│  │   GPIO 12   │              └──────────────┘                  │
│  └──────┬──────┘                                                 │
│         │ UART (115200)                                          │
│         ▼                                                        │
│  ┌─────────────┐                                                 │
│  │  Host PC    │ (log capture, test scripts)                     │
│  └─────────────┘                                                 │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Hardware Configuration

### 4.1 Device Under Test (DUT)

| Parameter | Value |
|-----------|-------|
| Hardware | QPG6200L Development Kit |
| SoC | QPG6200L (ARM Cortex-M4F, 256 KB RAM, 1 MB Flash) |
| Firmware | ThreadBleDoorbell\_DK\_qpg6200.hex |
| Debug interface | J-Link (SWD) |
| UART log port | USB-UART bridge, 115200 baud, 8N1 |

### 4.2 GPIO Assignment Reference

| GPIO | Direction | Function | Test Stimulus |
|------|-----------|----------|---------------|
| 1 | Output | WHITE\_COOL LED (BLE status) | Observe with eye / logic analyzer |
| 3 | Input | PB1 commissioning button | Manual press or bench signal |
| 5 | Input | PB2 doorbell button (active low) | Manual press or bench signal |
| 11 | Output | GREEN LED (Thread status) | Observe with eye / logic analyzer |
| 12 | Output | BLUE LED (ring indicator) | Observe with eye / logic analyzer |

---

## 5. Build Verification

### 5.1 Prerequisites

The following must be built and available before building the doorbell application:

```bash
# Step 1: Build the OpenThread library (Concurrent Light dependency)
make -f Applications/Concurrent/Light/Makefile.Light_Concurrent_qpg6200

# Step 2: Build the doorbell application
make -f Applications/Ble/ThreadBleDoorbell_DK/Makefile.ThreadBleDoorbell_DK_qpg6200
```

### 5.2 Expected Build Artifacts

| File | Description |
|------|-------------|
| `ThreadBleDoorbell_DK_qpg6200.elf` | Debug ELF with symbols |
| `ThreadBleDoorbell_DK_qpg6200.hex` | Signed firmware + bootloader merge |
| `ThreadBleDoorbell_DK_qpg6200.map` | Linker memory map |

---

## 6. Test Categories

| Category ID | Name | Priority |
|-------------|------|----------|
| TC-BLD | Build and Flash | Critical |
| TC-PWR | Power-On and Boot | Critical |
| TC-GPIO | GPIO and Buttons | High |
| TC-BLE | Bluetooth LE | Critical |
| TC-GATT | GATT Services | Critical |
| TC-THR | OpenThread Networking | Critical |
| TC-RING | Ring Logic | Critical |
| TC-LED | LED Indicators | Medium |
| TC-NVM | NVM Persistence | High |
| TC-PERF | Performance and Stability | High |
| TC-SEC | Security | High |

---

## 7. TC-BLD — Build and Flash Tests

### TC-BLD-001 — Clean Build Succeeds

**Objective:** Verify the firmware compiles without errors or warnings from a clean state.

**Preconditions:**
- GCC ARM toolchain installed and on PATH
- OpenThread dependency library built

**Steps:**
1. Run `make clean` for the doorbell target.
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
1. After successful build, run `arm-none-eabi-size ThreadBleDoorbell_DK_qpg6200.elf`.
2. Record code (text), data (data + bss), and total values.

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
2. Run flash programmer with `ThreadBleDoorbell_DK_qpg6200.hex`.
3. Perform a hard reset after programming.
4. Verify the device boots (observe LED activity).

**Expected Results:**
- Flash programming completes with no errors.
- Verification (read-back) passes.
- Device resets and boots into application.

**Pass Criteria:** Programming exit code 0, device boots.

---

### TC-BLD-004 — Post-Build Script Execution

**Objective:** Verify the signing and merge script completes successfully.

**Steps:**
1. After build, run `ThreadBleDoorbell_DK_qpg6200_postbuild.sh`.
2. Confirm the script exits 0.
3. Confirm the signed hex file is created.

**Expected Results:**
- Script runs without error.
- Output hex includes both application firmware and bootloader.

**Pass Criteria:** Script exit code 0, signed hex present.

---

## 8. TC-PWR — Power-On and Boot Tests

### TC-PWR-001 — Cold Boot Initialization

**Objective:** Verify the device initializes all subsystems on first power-up.

**Steps:**
1. Connect UART log at 115200 baud.
2. Apply power or reset the DK.
3. Observe UART log output.
4. Observe LED states.

**Expected Results:**
- UART log shows initialization messages for: FreeRTOS, BLE stack, OpenThread stack, GPIO configuration.
- WHITE LED begins blinking at ~1 Hz (500 ms on / 500 ms off) indicating BLE advertising.
- GREEN and BLUE LEDs remain off.
- No assertion or fault messages in the log.

**Pass Criteria:** All subsystems initialized, WHITE LED blinking, no faults.

---

### TC-PWR-002 — Reset Recovery

**Objective:** Verify the device recovers cleanly from a software or hardware reset.

**Steps:**
1. Allow device to reach an active state (BLE advertising or Thread joined).
2. Press the reset button or issue a JTAG reset.
3. Observe UART log and LED states.

**Expected Results:**
- Device re-initializes cleanly.
- No corruption in NVM data.
- BLE advertising restarts.
- If previously commissioned, Thread auto-join begins.

**Pass Criteria:** Clean reinitialization with no faults.

---

### TC-PWR-003 — Power Cycle with NVM State

**Objective:** Verify device state survives a full power removal.

**Preconditions:** Device has been commissioned into a Thread network.

**Steps:**
1. Commission device (BLE provisioning with Thread credentials).
2. Confirm Thread joined (GREEN LED solid).
3. Remove power for 5 seconds.
4. Restore power.
5. Observe device behavior.

**Expected Results:**
- Device reads credentials from NVM.
- Thread auto-join begins without BLE provisioning.
- GREEN LED transitions from blinking (joining) to solid (attached).

**Pass Criteria:** Auto-join succeeds within 60 seconds of power restore.

---

## 9. TC-GPIO — GPIO and Button Tests

### TC-GPIO-001 — PB2 Doorbell Button Detection

**Objective:** Verify GPIO 5 detects a button press and triggers a ring event.

**Steps:**
1. Boot device and ensure UART log is active.
2. Press and release PB2 (GPIO 5).
3. Observe UART log for ring event message.
4. Observe BLUE LED behavior.

**Expected Results:**
- Ring event logged within 200 ms of button press completion.
- BLUE LED blinks rapidly (100 ms on/off) for one ring cycle.
- UART log shows ring count incrementing.

**Pass Criteria:** Ring event detected and BLUE LED blinks.

---

### TC-GPIO-002 — PB2 Debounce Validation

**Objective:** Verify the 150 ms debounce filter rejects glitches shorter than 3 polling intervals.

**Steps:**
1. Using a signal generator or bench switch, apply a 50 ms pulse to GPIO 5 (simulating a single-sample glitch).
2. Observe UART log.
3. Apply a 200 ms pulse (long enough to pass debounce).
4. Observe UART log.

**Expected Results:**
- 50 ms pulse: no ring event logged.
- 200 ms pulse: ring event logged.

**Pass Criteria:** Short glitches rejected; sustained presses accepted.

---

### TC-GPIO-003 — PB1 Commissioning Button — BLE Restart

**Objective:** Verify holding PB1 for 2–4 seconds restarts BLE advertising.

**Steps:**
1. Establish a BLE connection (phone connected).
2. Press and hold PB1 for 3 seconds, then release.
3. Observe BLE connection and WHITE LED state.

**Expected Results:**
- BLE connection drops.
- BLE advertising restarts.
- WHITE LED resumes blinking at ~1 Hz.
- UART log indicates advertising restart.

**Pass Criteria:** BLE advertising restarted after 2–4 s hold.

---

### TC-GPIO-004 — PB1 Commissioning Button — Factory Reset

**Objective:** Verify holding PB1 for ≥ 5 seconds triggers a factory reset.

**Steps:**
1. Commission device with Thread credentials.
2. Press and hold PB1 for 6 seconds, then release.
3. Observe UART log for factory reset indication.
4. Observe LED states.
5. Power cycle the device.

**Expected Results:**
- UART log confirms factory reset initiated.
- NVM credentials erased.
- After power cycle, device does NOT auto-join Thread (starts BLE advertising only).
- WHITE LED blinks indicating advertising.

**Pass Criteria:** Factory reset clears NVM; subsequent boot requires re-commissioning.

---

### TC-GPIO-005 — Button Simultaneous Press Handling

**Objective:** Verify no fault occurs if both PB1 and PB2 are pressed simultaneously.

**Steps:**
1. Press both PB1 and PB2 simultaneously.
2. Hold for 2 seconds, then release both.
3. Observe system stability via UART log.

**Expected Results:**
- System remains stable; no crash or assertion.
- Both buttons processed independently per their event handlers.

**Pass Criteria:** System continues normal operation.

---

## 10. TC-BLE — Bluetooth LE Tests

### TC-BLE-001 — Advertising Presence

**Objective:** Verify the device advertises with the correct device name and service UUIDs.

**Steps:**
1. Boot device (fresh or factory-reset state).
2. Open nRF Connect on a smartphone.
3. Scan for BLE devices.
4. Locate "QPG Thread Doorbell" in the scan list.
5. Inspect the advertising packet.

**Expected Results:**

| Field | Expected Value |
|-------|---------------|
| Device name | "QPG Thread Doorbell" |
| Advertising flags | 0x06 (LE General Discoverable, BR/EDR Not Supported) |
| Service UUID | 0x180F (Battery Service) |
| Advertising interval | 20–60 ms |

**Pass Criteria:** Device visible in scan with correct name and advertisement data.

---

### TC-BLE-002 — Connection Establishment

**Objective:** Verify a BLE central (smartphone) can establish a connection.

**Steps:**
1. Device is advertising.
2. Connect using nRF Connect.
3. Observe WHITE LED state.
4. Observe UART log for connection event.

**Expected Results:**
- Connection establishes within 3 seconds.
- WHITE LED transitions from blinking to solid.
- UART log shows DM\_CONN\_OPEN event with connection parameters.

**Pass Criteria:** Connection established, WHITE LED solid.

---

### TC-BLE-003 — Connection Parameter Negotiation

**Objective:** Verify connection parameters are within acceptable ranges.

**Steps:**
1. Establish BLE connection.
2. In nRF Connect, inspect the connection parameters (interval, latency, timeout).

**Expected Results:**

| Parameter | Acceptable Range |
|-----------|-----------------|
| Connection interval | 7.5 ms – 100 ms |
| Peripheral latency | 0 – 4 |
| Supervision timeout | 1000 ms – 6000 ms |

**Pass Criteria:** All parameters within defined ranges.

---

### TC-BLE-004 — Disconnection Handling

**Objective:** Verify the device handles disconnection gracefully and resumes advertising.

**Steps:**
1. Establish BLE connection.
2. Disconnect from nRF Connect (or move out of range).
3. Observe WHITE LED and UART log.

**Expected Results:**
- UART log shows DM\_CONN\_CLOSE event.
- WHITE LED resumes blinking (advertising restarted).
- No fault or assertion in the log.

**Pass Criteria:** Device resumes advertising after disconnection.

---

### TC-BLE-005 — Single Connection Limit

**Objective:** Verify only one BLE connection is accepted simultaneously.

**Steps:**
1. Connect phone A via BLE.
2. Attempt to connect phone B while phone A is connected.

**Expected Results:**
- Phone A remains connected.
- Phone B scan shows device is no longer advertising (peripheral stops advertising when connected).
- Phone B cannot connect until phone A disconnects.

**Pass Criteria:** Device refuses second connection.

---

### TC-BLE-006 — Advertising Timeout

**Objective:** Verify advertising stops after the configured broadcast duration (~60 seconds) if no connection is made.

**Steps:**
1. Factory reset device to clear any existing connection state.
2. Start timer when WHITE LED begins blinking.
3. Do not connect any device.
4. Observe WHITE LED after 60 seconds.

**Expected Results:**
- WHITE LED stops blinking after approximately 60 seconds (BLE\_ADV\_BROADCAST\_DURATION = 0xF000).
- UART log indicates advertising stopped.

**Pass Criteria:** Advertising stops within 65 seconds of start.

---

## 11. TC-GATT — GATT Service Tests

### TC-GATT-001 — GATT Service Discovery

**Objective:** Verify all expected GATT services and characteristics are discoverable.

**Steps:**
1. Connect via nRF Connect.
2. Perform full GATT discovery.
3. Verify all services and characteristics.

**Expected Results:**

| Service | UUID | Handle |
|---------|------|--------|
| Battery Service | 0x180F | — |
| └─ Battery Level | 0x2A19 | 0x2002 |
| Doorbell Ring Service | D00RBELL-0000-... | — |
| └─ Ring State | Custom | 0x3002 |
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

**Objective:** Verify the Battery Level characteristic returns 100.

**Steps:**
1. Connect via nRF Connect.
2. Read handle 0x2002 (Battery Level).

**Expected Results:**
- Value returned: `0x64` (100 decimal).
- Read completes successfully.

**Pass Criteria:** Value is 100 (0x64).

---

### TC-GATT-003 — Ring State Read

**Objective:** Verify the Ring State characteristic can be read.

**Steps:**
1. Connect via nRF Connect.
2. Read handle 0x3002 (Ring State).

**Expected Results:**
- Value is `0x00` when idle.
- Read completes with ATT success status.

**Pass Criteria:** Read succeeds; idle value is 0x00.

---

### TC-GATT-004 — Remote Ring via BLE Write

**Objective:** Verify writing 0x01 to Ring State triggers a ring event.

**Steps:**
1. Connect via nRF Connect.
2. Enable notifications on handle 0x3002.
3. Write `0x01` to handle 0x3002.
4. Observe UART log and BLUE LED.

**Expected Results:**
- Ring event logged.
- BLUE LED blinks rapidly.
- Notification received with value `0x01`.
- Ring count increments.

**Pass Criteria:** Ring event triggered, notification received.

---

### TC-GATT-005 — Ring State Notification

**Objective:** Verify Ring State notifications are sent to connected phone on ring events.

**Steps:**
1. Connect via nRF Connect.
2. Enable CCCD notifications on handle 0x3002 (write `0x0001` to handle 0x3003).
3. Press PB2 (GPIO 5).
4. Observe notifications received in nRF Connect.

**Expected Results:**
- Notification received with value `0x01` within 200 ms of button press.

**Pass Criteria:** Notification received after button press.

---

### TC-GATT-006 — Thread Config Service — Write Network Name

**Objective:** Verify the Network Name characteristic accepts writes.

**Steps:**
1. Connect via nRF Connect.
2. Write a UTF-8 string (e.g., "TestNetwork\0\0\0\0\0") to handle 0x4002.
3. Read back handle 0x4002.

**Expected Results:**
- Write completes with ATT success status.
- Read-back returns the written value.

**Pass Criteria:** Write and read-back match.

---

### TC-GATT-007 — Thread Config Service — Channel Boundary Values

**Objective:** Verify the Channel characteristic enforces 802.15.4 channel range (11–26).

**Steps:**
1. Connect via nRF Connect.
2. Write value `0x0B` (11) to handle 0x4006. Confirm success.
3. Write value `0x1A` (26) to handle 0x4006. Confirm success.
4. Write value `0x0A` (10) to handle 0x4006. Observe response.
5. Write value `0x1B` (27) to handle 0x4006. Observe response.

**Expected Results:**
- Values 11 and 26: write accepted.
- Values 10 and 27: write rejected with ATT error (Application Error or Out of Range).

**Pass Criteria:** Boundary values enforced correctly.

---

### TC-GATT-008 — Thread Network Key Write-Only

**Objective:** Verify the Network Key characteristic is write-only (read not permitted).

**Steps:**
1. Connect via nRF Connect.
2. Attempt to read handle 0x4004 (Network Key).
3. Write 16 bytes (any value) to handle 0x4004.

**Expected Results:**
- Read returns ATT error (Read Not Permitted).
- Write completes with ATT success status.

**Pass Criteria:** Read rejected; write accepted.

---

### TC-GATT-009 — Thread Status Notification on Join

**Objective:** Verify Thread Status notifications are sent as the network role changes.

**Steps:**
1. Connect via nRF Connect.
2. Enable CCCD notifications on handle 0x400C (write `0x0001` to handle 0x400D).
3. Provision credentials and write 0x01 to Join handle (0x400A).
4. Observe notifications.

**Expected Results:**
- Notification with value `0x01` (detached) sent shortly after join initiated.
- Notification with value `0x02` or `0x03` (child or router) sent once mesh joined.

**Pass Criteria:** Status notifications received reflecting role transitions.

---

## 12. TC-THR — OpenThread Network Tests

### TC-THR-001 — BLE Commissioning Workflow

**Objective:** Verify the full BLE commissioning workflow joins the device to a Thread network.

**Steps:**
1. Start from factory reset state.
2. Connect phone via BLE.
3. Write valid Thread Network Name (handle 0x4002).
4. Write valid Thread Network Key (handle 0x4004).
5. Write channel (handle 0x4006) — value matching OTBR.
6. Write PAN ID (handle 0x4008) — value matching OTBR.
7. Write `0x01` to Join handle (0x400A).
8. Monitor Thread Status notifications.
9. Monitor GREEN LED.

**Expected Results:**
- GREEN LED transitions from blinking to solid.
- Thread Status notification transitions: `0x00` → `0x01` → `0x02`/`0x03`.
- UART log shows otThreadSetEnabled and role change events.
- OTBR dashboard shows device joined.

**Pass Criteria:** Device joins Thread network and role is child, router, or leader.

---

### TC-THR-002 — Thread Network Auto-Rejoin After Reset

**Objective:** Verify device auto-joins Thread on reset when credentials are in NVM.

**Preconditions:** TC-THR-001 passed and device is joined.

**Steps:**
1. Press the DK reset button.
2. Observe GREEN LED and UART log.
3. Do not perform BLE commissioning.

**Expected Results:**
- Device reads credentials from NVM.
- GREEN LED transitions from blinking to solid within 60 seconds.
- OTBR dashboard shows device rejoined.

**Pass Criteria:** Auto-rejoin within 60 seconds without BLE interaction.

---

### TC-THR-003 — Thread Role Stability

**Objective:** Verify the device maintains a stable Thread role over an extended period.

**Steps:**
1. Join Thread network.
2. Leave device running for 30 minutes.
3. Monitor UART log and GREEN LED continuously.
4. Ping the device from OTBR.

**Expected Results:**
- GREEN LED remains solid.
- Thread role does not drop to detached unexpectedly.
- Ping responses received.

**Pass Criteria:** No role drops in 30-minute window.

---

### TC-THR-004 — Ring Multicast Transmission

**Objective:** Verify a ring event sends a valid UDP multicast to ff03::1 on port 5683.

**Preconditions:** Device joined to Thread network. UDP listener running on host at port 5683.

**Steps:**
1. Press PB2 (doorbell button).
2. Capture 802.15.4 packets with Wireshark + sniffer.
3. Observe UDP listener on host PC.

**Expected Results:**
- UDP packet received on host from device's Thread IPv6 address.
- Destination: `ff03::1`, port 5683.
- Payload bytes 0–3: `{0x02, 0x01, 0x00, 0x01}` for first press.
- Payload byte 4 and 5 (count): increments with each press.

**Pass Criteria:** Correct multicast payload received on UDP listener.

---

### TC-THR-005 — Ring Multicast Reception and Relay

**Objective:** Verify a ring multicast received from another Thread device triggers a ring event.

**Preconditions:** Two QPG6200L devices joined to the same Thread network. One is the DUT; the other sends multicast.

**Steps:**
1. Press the doorbell button on Device 2 (or inject via UDP from host).
2. Observe DUT UART log and BLUE LED.

**Expected Results:**
- DUT logs "Ring received from Thread network".
- DUT BLUE LED blinks (ring event processed).
- DUT does not re-broadcast the received ring (loop prevention).

**Pass Criteria:** Remote ring processed locally; no re-broadcast.

---

### TC-THR-006 — Thread Detach and Rejoin

**Objective:** Verify device re-attaches to the Thread network after temporary loss of connectivity.

**Steps:**
1. Device is joined to Thread network.
2. Power off OTBR for 2 minutes.
3. Power OTBR back on.
4. Observe device behavior.

**Expected Results:**
- GREEN LED blinks (detached/searching state) while OTBR is offline.
- GREEN LED returns to solid within 60 seconds of OTBR coming back online.

**Pass Criteria:** Device rejoins within 60 seconds of network recovery.

---

### TC-THR-007 — Invalid Commissioning Credentials

**Objective:** Verify device handles invalid Thread credentials gracefully.

**Steps:**
1. Connect via BLE.
2. Write an invalid Network Key (fewer than 16 bytes) to handle 0x4004.
3. Write `0x01` to Join handle.
4. Observe UART log and Thread Status notifications.

**Expected Results:**
- Thread join fails or is rejected.
- Thread Status notification shows `0x01` (detached) or error state.
- No crash or assertion.
- System remains stable.

**Pass Criteria:** Invalid credentials handled without crash.

---

## 13. TC-RING — Doorbell Ring Logic Tests

### TC-RING-001 — Ring Count Increment

**Objective:** Verify the ring counter increments correctly with each ring event.

**Steps:**
1. Observe UART log.
2. Press PB2 five times in sequence (wait 1 second between presses).
3. Observe ring count in UART log.

**Expected Results:**
- Ring count increments: 1, 2, 3, 4, 5.
- Each multicast payload encodes the correct count in bytes 2–3.

**Pass Criteria:** Count increments correctly for all 5 presses.

---

### TC-RING-002 — Ring from All Three Sources

**Objective:** Verify ring events from all three trigger sources are processed identically.

**Steps:**
1. Source A: Press PB2 (local button). Observe ring.
2. Source B: Write 0x01 to Ring State via BLE (handle 0x3002). Observe ring.
3. Source C: Inject UDP multicast `{0x02, 0x01, 0x00, 0x01}` from OTBR to device. Observe ring.

**Expected Results for each source:**
- BLUE LED blinks rapidly.
- Ring count increments.
- UART log shows ring source identifier.

**Pass Criteria:** All three sources trigger identical ring behavior.

---

### TC-RING-003 — Rapid Repeat Ring Events

**Objective:** Verify the system handles rapid successive ring events without queue overflow.

**Steps:**
1. Press PB2 10 times in rapid succession (approx. 200 ms between presses).
2. Observe UART log and BLUE LED.

**Expected Results:**
- All 10 ring events logged.
- No FreeRTOS queue overflow warning.
- Ring count reaches 10.

**Pass Criteria:** All events processed; no overflow.

---

### TC-RING-004 — Loop Prevention on Received Multicast

**Objective:** Verify the device does not re-broadcast a ring event received from the Thread mesh.

**Steps:**
1. Send a ring multicast from host to device via OTBR UDP injection.
2. Monitor 802.15.4 sniffer for any re-multicast from DUT.

**Expected Results:**
- DUT processes the received ring event locally (LED, notification).
- DUT does not emit a new UDP multicast.

**Pass Criteria:** No re-broadcast observed within 1 second of event processing.

---

### TC-RING-005 — BLE Notification Not Sent When No CCCD Enabled

**Objective:** Verify no BLE notification is sent if CCCD is not enabled.

**Steps:**
1. Connect via nRF Connect.
2. Do NOT enable notifications on handle 0x3002.
3. Press PB2 to trigger a ring.
4. Monitor for unsolicited BLE notifications in nRF Connect.

**Expected Results:**
- No BLE notification sent.
- Ring event still processed locally (LED blinks, multicast sent).

**Pass Criteria:** No notification observed in nRF Connect.

---

## 14. TC-LED — LED Indicator Tests

### TC-LED-001 — WHITE LED BLE Advertising Pattern

**Objective:** Verify WHITE LED (GPIO 1) blinks at 500 ms on / 500 ms off during BLE advertising.

**Steps:**
1. Boot device in factory reset state.
2. Time the WHITE LED blink period with a stopwatch or logic analyzer.

**Expected Results:**
- Period: 1000 ms ± 100 ms.
- Duty cycle: approximately 50%.

**Pass Criteria:** Timing within 10% of 1 Hz.

---

### TC-LED-002 — WHITE LED Solid When BLE Connected

**Objective:** Verify WHITE LED is solid on when a BLE connection is active.

**Steps:**
1. Connect phone via BLE.
2. Observe WHITE LED.

**Expected Results:**
- WHITE LED transitions from blinking to solid ON immediately on connection.

**Pass Criteria:** WHITE LED solid on BLE connection.

---

### TC-LED-003 — GREEN LED Thread Joining Pattern

**Objective:** Verify GREEN LED (GPIO 11) blinks at 200 ms on / 200 ms off while joining Thread.

**Steps:**
1. Initiate Thread join via BLE commissioning.
2. Observe GREEN LED pattern during joining phase.

**Expected Results:**
- GREEN LED blinks at 200 ms period ± 50 ms.

**Pass Criteria:** Timing within 25% of 5 Hz.

---

### TC-LED-004 — GREEN LED Solid When Thread Attached

**Objective:** Verify GREEN LED is solid once device has joined the Thread network.

**Steps:**
1. Commission device and allow it to join Thread.
2. Observe GREEN LED after UART log shows role change to child/router/leader.

**Expected Results:**
- GREEN LED transitions from blinking to solid ON.

**Pass Criteria:** GREEN LED solid after Thread attach.

---

### TC-LED-005 — BLUE LED Ring Blink Pattern

**Objective:** Verify BLUE LED (GPIO 12) blinks at 100 ms on / 100 ms off on ring event.

**Steps:**
1. Press PB2 once.
2. Observe BLUE LED with logic analyzer.

**Expected Results:**
- BLUE LED blinks at ~100 ms period during ring indication.

**Pass Criteria:** Blink pattern observed within 20% of 10 Hz.

---

## 15. TC-NVM — NVM Persistence Tests

### TC-NVM-001 — Thread Credentials Persist Across Reset

**Objective:** Verify Thread credentials written via BLE are retained after a reset.

**Steps:**
1. Commission device with known credentials.
2. Verify Thread join succeeds.
3. Perform hardware reset.
4. Observe device rejoining Thread without BLE interaction.

**Expected Results:**
- Device reads credentials from NVM on boot.
- GREEN LED goes solid within 60 seconds.

**Pass Criteria:** Credentials persist; auto-rejoin succeeds.

---

### TC-NVM-002 — Factory Reset Clears Credentials

**Objective:** Verify factory reset (PB1 held ≥ 5 s) erases NVM credentials.

**Steps:**
1. Commission device and verify Thread join.
2. Hold PB1 for 6 seconds.
3. Power cycle device.
4. Observe boot behavior.

**Expected Results:**
- Device does not auto-join Thread.
- WHITE LED blinks (advertising), no Thread joining.
- UART log shows no NVM credentials found.

**Pass Criteria:** NVM cleared; device requires re-commissioning.

---

### TC-NVM-003 — NVM Integrity After Multiple Writes

**Objective:** Verify NVM integrity after 10 successive commissioning cycles.

**Steps:**
1. Perform 10 complete commissioning cycles:
   - Write credentials via BLE
   - Verify Thread join
   - Perform factory reset
2. On the 11th cycle, verify normal commissioning still functions.

**Expected Results:**
- All 10 cycles complete successfully.
- 11th commissioning cycle completes without error.
- No NVM wear or corruption observed in UART logs.

**Pass Criteria:** All cycles successful; no NVM errors.

---

## 16. TC-PERF — Performance and Stability Tests

### TC-PERF-001 — 24-Hour Continuous Operation

**Objective:** Verify system stability over a 24-hour period with periodic ring events.

**Steps:**
1. Commission device and join Thread network.
2. Connect via BLE (or leave disconnected — test both).
3. Script periodic ring events via UDP injection every 5 minutes.
4. Monitor UART log continuously for 24 hours.

**Expected Results:**
- No crash, assertion, or watchdog reset during 24-hour period.
- All ring events processed.
- Ring count increments consistently.
- BLE and Thread remain operational.

**Pass Criteria:** Zero unplanned resets; all events processed.

---

### TC-PERF-002 — BLE Reconnection Under Load

**Objective:** Verify repeated BLE disconnect/reconnect cycles do not degrade performance.

**Steps:**
1. Commission device and join Thread.
2. Connect via BLE, disconnect, reconnect — repeat 50 times.
3. After 50 cycles, verify full functionality.

**Expected Results:**
- All 50 reconnection cycles succeed.
- After 50 cycles, BLE GATT operates normally.
- Thread connectivity maintained throughout.

**Pass Criteria:** 50/50 reconnections successful; full functionality retained.

---

### TC-PERF-003 — FreeRTOS Event Queue Load Test

**Objective:** Verify the event queue (capacity 20) does not overflow under simultaneous events.

**Steps:**
1. Simultaneously inject ring events from all sources (PB2, BLE write, UDP multicast) in rapid succession.
2. Monitor UART log for queue overflow messages.

**Expected Results:**
- No queue overflow message in UART log.
- System continues operating.

**Pass Criteria:** No queue overflow under combined load.

---

### TC-PERF-004 — Memory Leak Detection

**Objective:** Verify no heap memory leak occurs over 1 hour of operation with repeated BLE connections.

**Steps:**
1. Connect UART log.
2. Note initial heap free space from boot log.
3. Repeatedly connect and disconnect BLE every 30 seconds for 1 hour.
4. Compare heap free space before and after.

**Expected Results:**
- Heap free space remains within 5% of initial value after 1 hour.

**Pass Criteria:** Heap delta ≤ 5% over 1 hour.

---

### TC-PERF-005 — Ring Event Latency

**Objective:** Verify the end-to-end latency from button press to BLUE LED is within an acceptable human-perceptible threshold.

**Steps:**
1. Use logic analyzer to capture GPIO 5 (button press) and GPIO 12 (BLUE LED onset).
2. Press PB2 10 times.
3. Measure delay from GPIO 5 assertion to GPIO 12 assertion.

**Expected Results:**
- Latency: ≤ 250 ms (sum of 150 ms debounce + 50 ms poll + processing).

**Pass Criteria:** All 10 measurements ≤ 250 ms.

---

## 17. TC-SEC — Security Tests

### TC-SEC-001 — No Encryption Enforcement (Known Limitation)

**Objective:** Document and confirm the device operates without BLE pairing or encryption.

**Steps:**
1. Connect via any BLE central without providing pairing credentials.
2. Read and write characteristics.

**Expected Results:**
- Connection succeeds without any pairing challenge.
- All characteristics accessible (per the DM\_SEC\_LEVEL\_NONE configuration).

**Note:** This is a known design choice for the demo application. Production deployments should enable BLE encryption. This test confirms the known behavior.

**Pass Criteria:** Behavior matches documented security level (DM\_SEC\_LEVEL\_NONE).

---

### TC-SEC-002 — Thread Network Key Not Readable

**Objective:** Verify the Thread Network Key cannot be read back via BLE.

**Steps:**
1. Connect via BLE.
2. Write a known 16-byte key to handle 0x4004.
3. Attempt to read handle 0x4004.

**Expected Results:**
- Read returns ATT error (Read Not Permitted or Application Error).
- Key bytes are never returned in any ATT response.

**Pass Criteria:** Key read blocked at ATT layer.

---

### TC-SEC-003 — Firmware Signature Verification

**Objective:** Verify the bootloader rejects a firmware image with an invalid ECDSA P-256 signature.

**Steps:**
1. Corrupt the last 64 bytes of the signed hex file.
2. Attempt to flash the corrupted image.
3. Observe bootloader behavior on reset.

**Expected Results:**
- Bootloader detects signature mismatch.
- Device does not boot into the corrupted application.
- Bootloader may enter error state or retain the previous valid image.

**Pass Criteria:** Corrupted image rejected by bootloader.

---

### TC-SEC-004 — Unauthorized Write to Join Characteristic

**Objective:** Verify that initiating Thread join with no credentials set does not cause a crash.

**Steps:**
1. Factory reset device.
2. Connect via BLE without writing any Thread credentials.
3. Write 0x01 to Join handle (0x400A).
4. Observe UART log.

**Expected Results:**
- No crash or assertion.
- UART log shows a graceful error (e.g., "No dataset configured").
- Thread Status notification returns 0x00 (disabled) or 0x01 (detached with error).

**Pass Criteria:** No crash; error handled gracefully.

---

## 18. Acceptance Criteria Summary

For a build to be considered **production-ready**, the following criteria must all be met:

| Condition | Requirement |
|-----------|-------------|
| TC-BLD tests | All pass (build succeeds, no memory overrun) |
| TC-PWR tests | All pass |
| TC-GPIO tests | TC-GPIO-001 through TC-GPIO-004 all pass |
| TC-BLE tests | All pass |
| TC-GATT tests | All pass |
| TC-THR tests | TC-THR-001 through TC-THR-005 all pass |
| TC-RING tests | All pass |
| TC-LED tests | All pass |
| TC-NVM tests | All pass |
| TC-PERF-001 | 24-hour test passes |
| TC-PERF-002 | 50-cycle BLE reconnect test passes |
| TC-SEC-002 | Network key not readable via BLE |
| TC-SEC-003 | Bootloader rejects invalid signature |
| Open Critical defects | Zero |
| Open High defects | Zero |

---

## 19. Defect Classification

| Severity | Definition | Example |
|----------|------------|---------|
| **Critical** | System crash, data loss, security breach, functionality completely broken | Hard fault on ring event, NVM corruption |
| **High** | Major feature non-functional; no acceptable workaround | BLE commissioning fails, Thread never joins |
| **Medium** | Feature degraded; workaround exists | LED blink period off by 20%, advertising restarts slowly |
| **Low** | Cosmetic or minor deviation; no user impact | Log message typo, minor timing discrepancy |

---

## 20. Test Results Log Template

```
Application:   ThreadBleDoorbell_DK
Firmware:      ThreadBleDoorbell_DK_qpg6200.hex
Build Hash:    [SHA-256 of .hex file]
DK Serial:     [hardware serial number]
Tester:        [name]
Date:          [YYYY-MM-DD]
Environment:   [OTBR version, nRF Connect version, tool versions]

┌──────────────┬───────────────────────────────┬────────┬────────────┐
│ Test Case ID │ Test Name                     │ Result │ Notes      │
├──────────────┼───────────────────────────────┼────────┼────────────┤
│ TC-BLD-001   │ Clean Build Succeeds          │ PASS / FAIL │       │
│ TC-BLD-002   │ Memory Footprint              │ PASS / FAIL │       │
│ TC-BLD-003   │ Flash Programming             │ PASS / FAIL │       │
│ TC-BLD-004   │ Post-Build Script             │ PASS / FAIL │       │
│ TC-PWR-001   │ Cold Boot                     │ PASS / FAIL │       │
│ TC-PWR-002   │ Reset Recovery                │ PASS / FAIL │       │
│ TC-PWR-003   │ Power Cycle with NVM          │ PASS / FAIL │       │
│ TC-GPIO-001  │ PB2 Button Detection          │ PASS / FAIL │       │
│ TC-GPIO-002  │ PB2 Debounce                  │ PASS / FAIL │       │
│ TC-GPIO-003  │ PB1 BLE Restart               │ PASS / FAIL │       │
│ TC-GPIO-004  │ PB1 Factory Reset             │ PASS / FAIL │       │
│ TC-GPIO-005  │ Simultaneous Button Press     │ PASS / FAIL │       │
│ TC-BLE-001   │ Advertising Presence          │ PASS / FAIL │       │
│ TC-BLE-002   │ Connection Establishment      │ PASS / FAIL │       │
│ TC-BLE-003   │ Connection Parameters         │ PASS / FAIL │       │
│ TC-BLE-004   │ Disconnection Handling        │ PASS / FAIL │       │
│ TC-BLE-005   │ Single Connection Limit       │ PASS / FAIL │       │
│ TC-BLE-006   │ Advertising Timeout           │ PASS / FAIL │       │
│ TC-GATT-001  │ GATT Service Discovery        │ PASS / FAIL │       │
│ TC-GATT-002  │ Battery Level Read            │ PASS / FAIL │       │
│ TC-GATT-003  │ Ring State Read               │ PASS / FAIL │       │
│ TC-GATT-004  │ Remote Ring via BLE Write     │ PASS / FAIL │       │
│ TC-GATT-005  │ Ring State Notification       │ PASS / FAIL │       │
│ TC-GATT-006  │ Write Network Name            │ PASS / FAIL │       │
│ TC-GATT-007  │ Channel Boundary Values       │ PASS / FAIL │       │
│ TC-GATT-008  │ Network Key Write-Only        │ PASS / FAIL │       │
│ TC-GATT-009  │ Thread Status Notification    │ PASS / FAIL │       │
│ TC-THR-001   │ BLE Commissioning Workflow    │ PASS / FAIL │       │
│ TC-THR-002   │ Auto-Rejoin After Reset       │ PASS / FAIL │       │
│ TC-THR-003   │ Thread Role Stability         │ PASS / FAIL │       │
│ TC-THR-004   │ Ring Multicast Transmission   │ PASS / FAIL │       │
│ TC-THR-005   │ Ring Multicast Reception      │ PASS / FAIL │       │
│ TC-THR-006   │ Thread Detach and Rejoin      │ PASS / FAIL │       │
│ TC-THR-007   │ Invalid Credentials           │ PASS / FAIL │       │
│ TC-RING-001  │ Ring Count Increment          │ PASS / FAIL │       │
│ TC-RING-002  │ Ring from All Sources         │ PASS / FAIL │       │
│ TC-RING-003  │ Rapid Repeat Events           │ PASS / FAIL │       │
│ TC-RING-004  │ Loop Prevention               │ PASS / FAIL │       │
│ TC-RING-005  │ No Notification Without CCCD  │ PASS / FAIL │       │
│ TC-LED-001   │ WHITE LED Advertising         │ PASS / FAIL │       │
│ TC-LED-002   │ WHITE LED Connected           │ PASS / FAIL │       │
│ TC-LED-003   │ GREEN LED Joining             │ PASS / FAIL │       │
│ TC-LED-004   │ GREEN LED Attached            │ PASS / FAIL │       │
│ TC-LED-005   │ BLUE LED Ring Blink           │ PASS / FAIL │       │
│ TC-NVM-001   │ Credentials Persist           │ PASS / FAIL │       │
│ TC-NVM-002   │ Factory Reset Clears          │ PASS / FAIL │       │
│ TC-NVM-003   │ NVM Integrity Multiple Writes │ PASS / FAIL │       │
│ TC-PERF-001  │ 24-Hour Continuous Operation  │ PASS / FAIL │       │
│ TC-PERF-002  │ BLE Reconnection Under Load   │ PASS / FAIL │       │
│ TC-PERF-003  │ Event Queue Load Test         │ PASS / FAIL │       │
│ TC-PERF-004  │ Memory Leak Detection         │ PASS / FAIL │       │
│ TC-PERF-005  │ Ring Event Latency            │ PASS / FAIL │       │
│ TC-SEC-001   │ No Encryption (Known Limit.)  │ PASS / FAIL │       │
│ TC-SEC-002   │ Network Key Not Readable      │ PASS / FAIL │       │
│ TC-SEC-003   │ Firmware Signature Check      │ PASS / FAIL │       │
│ TC-SEC-004   │ Unauthorized Join Write       │ PASS / FAIL │       │
└──────────────┴───────────────────────────────┴────────────┴────────┘

Overall Result: [ ] PASS   [ ] FAIL   [ ] PARTIAL

Defects Found:
1. [ID] [Severity] [Description]
...

Sign-off:
Tester Signature: _________________ Date: _________
Review Signature: _________________ Date: _________
```

---

*End of ThreadBleDoorbell\_DK Testing and Verification Plan*
