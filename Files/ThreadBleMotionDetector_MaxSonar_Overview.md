# ThreadBleMotionDetector_MaxSonar — Technical Overview

## Table of Contents

1. [Purpose & Summary](#1-purpose--summary)
2. [Hardware Platform](#2-hardware-platform)
3. [Software Architecture](#3-software-architecture)
4. [Source File Reference](#4-source-file-reference)
5. [LV-MaxSonar EZ Sensor Integration](#5-lv-maxsonar-ez-sensor-integration)
6. [Motion Detection Logic](#6-motion-detection-logic)
7. [BLE Functionality](#7-ble-functionality)
8. [OpenThread Networking](#8-openthread-networking)
9. [Hardware Peripherals](#9-hardware-peripherals)
10. [Application State Machine & Event Flow](#10-application-state-machine--event-flow)
11. [Configuration Parameters](#11-configuration-parameters)
12. [Build System](#12-build-system)
13. [Key Data Structures](#13-key-data-structures)

---

## 1. Purpose & Summary

**ThreadBleMotionDetector_MaxSonar** is a dual-protocol IoT demo application for the QPG6200L Development Kit. It implements a proximity-based motion detector that:

- Reads distance continuously from an **LV-MaxSonar EZ** ultrasonic sensor over UART at 9600 baud.
- Declares **motion detected** when any object comes within **2 metres (200 cm)** of the sensor.
- Advertises over **Bluetooth LE (BLE)** so a smartphone can commission it onto a Thread mesh network and receive live motion/distance notifications.
- Joins an **OpenThread** mesh and multicasts every motion state change (detected or cleared) to all other nodes on the network — enabling distributed occupancy sensing or triggering remote actuators via a Thread Border Router and Node-RED.

The application targets the QPG6200L DK directly — no carrier board required. All indicators are the on-board LEDs; the only external hardware is the MaxSonar sensor itself.

---

## 2. Hardware Platform

| Property | Value |
|---|---|
| SoC | QPG6200L (ARM Cortex-M4F) |
| RAM | 256 KB |
| Flash | 1 MB |
| RTOS | FreeRTOS |
| Radio | Integrated dual-stack BLE + 802.15.4 (Thread) |
| Toolchain | GCC, ARMv7e-m, -mfpu=fpv4-sp-d16, hard-float ABI |
| Build optimisation | -Os (size) |
| C++ standard | gnu++17 |

---

## 3. Software Architecture

The application uses the same **singleton event-driven architecture** as the other QPG6200 demo apps. All event sources post into a central FreeRTOS queue; a single main task dispatches them.

```
main()
 └─ gpBaseComps_StackInit()            ← Initialises BLE + Thread radio stacks
 └─ AppTask::Init()
     ├─ Creates FreeRTOS event queue    (20 events)
     ├─ Spawns AppTask::Main() task     (6 KB stack, priority 2)
     ├─ AppButtons init                 (PB1 commissioning button)
     ├─ AppManager::Init()
     │    ├─ BleIf_Init()              ← Registers BLE callbacks
     │    ├─ StatusLed_Init()          ← Configures 3 GPIO LEDs
     │    ├─ AppButtons registration   ← PB1 multifunction button
     │    ├─ Thread_Init()             ← Creates OpenThread instance
     │    └─ BleIf_StartAdvertising()  ← Begins advertising "QPG MaxSonar Motion"
     ├─ SensorManager::Init()          ← Configures UART1 at 9600 baud
     └─ SensorManager::StartSensing()
          └─ Spawns UART poll task     (50 ms interval, priority idle+1)

AppTask::Main()  [forever]
 └─ xQueueReceive() blocks on event queue
 └─ AppManager::EventHandler() dispatches to per-type handler
```

---

## 4. Source File Reference

### Header Files (`inc/`)

| File | Role |
|---|---|
| `AppManager.h` | Core application manager. Manages BLE/Thread init, LED state machine, and motion reporting. |
| `AppTask.h` | FreeRTOS task wrapper — event queue, system reset, factory reset. |
| `AppEvent.h` | Unified `AppEvent` union covering Button, BLE, Sensor, and Thread sub-events. |
| `SensorManager.h` | UART-based sensor polling, ASCII frame parser, motion threshold logic. |
| `MotionDetector_Config.h` | BLE GATT attribute handle definitions and all custom service UUIDs. |
| `Ble_Peripheral_Config.h` | Thin shim that re-includes `MotionDetector_Config.h`. |
| `qPinCfg.h` | Canonical GPIO number assignments for all board peripherals. |

### Implementation Files (`src/`)

| File | Lines | Role |
|---|---|---|
| `AppManager.cpp` | ~633 | All application logic: BLE callbacks, Thread callbacks, motion reporting, LED control. |
| `AppTask.cpp` | ~194 | FreeRTOS task lifecycle, event queue, thread-safe `PostEvent()`, `ResetSystem()`. |
| `SensorManager.cpp` | ~185 | UART1 configuration, per-byte parser state machine, distance→motion conversion. |
| `MotionDetector_Config.c` | ~357 | Static GATT attribute tables for all three BLE services; advertising data; Thread config NVM accessors. |
| `platform_memory.c` | ~19 | `otPlatCAlloc()` / `otPlatFree()` — OpenThread heap allocation wrappers. |

### Generated Files (`gen/ThreadBleMotionDetector_MaxSonar_qpg6200/`)

| File | Role |
|---|---|
| `qorvo_config.h` | Build-time settings: BLE buffer counts, enabled components, UART baud rate. |
| `qorvo_internals.h` | Complete component manifest (BLE, Thread, FreeRTOS, crypto, etc.). |
| `esec_config.h` | Hardware security engine flags: AES, SHA, ECC, CMAC. |
| `q_info_page.h` | Bootloader info-page: programming pins, device attestation, license data. |
| `gpHal_RtSystem.c` | Flash-resident runtime system data (auto-generated). |
| `ThreadBleMotionDetector_MaxSonar_qpg6200.ld` | ARM linker script — memory map and section assignments. |

---

## 5. LV-MaxSonar EZ Sensor Integration

### Physical Connection

The LV-MaxSonar EZ uses a simple UART serial output protocol. Connect it to the QPG6200L DK as follows:

| MaxSonar Pin | DK GPIO | Function |
|---|---|---|
| Pin 5 (TX) | GPIO 8 (UART1 RX) | Sensor transmits range data |
| Pin 4 (RX) | GPIO 9 (UART1 TX) | Held HIGH → free-run mode (continuous ranging) |
| GND | GND | Common ground |
| +5V | 5 V header | Power supply |

When pin 4 (RX) is held continuously HIGH the MaxSonar enters **free-running mode**: it triggers a new range measurement approximately every **49 ms** and streams the result over its TX pin immediately afterwards.

### UART Configuration

| Parameter | Value |
|---|---|
| UART instance | 1 |
| Baud rate | 9600 baud |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Mode | Polling (no DMA, no interrupts) |
| Pin config macro | `Q_DRV_UART_PIN_CONFIG(1, GPIO9, GPIO8)` |

### Serial Protocol

The MaxSonar outputs one ASCII frame per measurement:

```
'R' <D2> <D1> <D0> '\r'
 │    │    │    │    └── Carriage return (0x0D) — frame terminator
 │    └────┴────┴─────── Three ASCII decimal digits, most-significant first
 └──────────────────────── 'R' — frame start marker
```

**Example:** `R079\r` means 79 inches.

The unit of the raw value is **inches**. Valid range: 000–765 inches (approximately 0–19.4 m for the EZ1 model; sensor range determines the upper limit).

### Frame Parser (SensorManager)

`SensorManager` implements a 3-state machine that processes one byte at a time from the UART receive buffer:

```
State 0 — Hunt for 'R':
  if (byte == 'R') → goto State 1

State 1 — Collect 3 decimal digits:
  if ('0' ≤ byte ≤ '9'):
    frameBuf[digitCount++] = byte
    if (digitCount == 3) → goto State 2
  else:
    Reset to State 0    // Error: unexpected character, restart frame

State 2 — Expect carriage return:
  if (byte == '\r'):
    inches = (frameBuf[0]-'0')×100 + (frameBuf[1]-'0')×10 + (frameBuf[2]-'0')
    cm     = inches × 254 / 100   // 1 inch = 2.54 cm → ×254 ÷ 100 avoids float
    ProcessDistance(cm)
  Reset to State 0 regardless
```

### Distance Unit Conversion

The conversion avoids floating-point arithmetic:

```c
uint16_t cm = (uint16_t)((uint32_t)inches * 254u / 100u);
```

This yields centimetres with sub-centimetre truncation. For example, 79 inches → `79 × 254 / 100 = 200 cm`.

---

## 6. Motion Detection Logic

### Threshold

```
Motion DETECTED  when:  0 < distanceCm  ≤  200
Motion CLEARED   when:  distanceCm      >  200  (or no valid frame)
```

The threshold of **200 cm (2 metres)** is defined by `MOTION_DISTANCE_THRESHOLD_CM` in `SensorManager.h` and can be changed at compile time.

### Hysteresis and State Transitions

`SensorManager` tracks the previous stable motion state. It only calls `AppManager::NotifySensorEvent()` when the **state changes**:

```
ProcessDistance(uint16_t cm)
  detected = (cm > 0 && cm ≤ MOTION_DISTANCE_THRESHOLD_CM)
  if (detected == sMotionDetected):
    return   // Same state — no event generated
  sMotionDetected = detected
  sLastDistanceCm = cm
  AppManager::NotifySensorEvent(detected, cm)  // Post SensorEvent to queue
```

This prevents the main task from being flooded with events while an object remains stationary within the detection zone.

### Polling Rate

The UART polling task runs at **50 ms** intervals (`SENSOR_POLL_MS`). Given that the MaxSonar outputs a new frame every ~49 ms, the polling interval closely matches the sensor's output rate. This means motion state changes are typically reported within one sensor cycle (~50 ms) of the physical event.

---

## 7. BLE Functionality

### Advertising

| Parameter | Value |
|---|---|
| Device name | "QPG MaxSonar Motion" |
| Advertising interval | 20 – 60 ms |
| Advertising data | Flags (0x06: General Discoverable + no BR/EDR) + Battery Service UUID (0x180F) |
| Scan response | Complete local name "QPG MaxSonar Motion" |
| BLE stack | Cordio BLE Host (ARM reference) |

### GATT Services

#### Battery Service (UUID 0x180F — Standard Bluetooth)

| Handle | Characteristic | Properties | Value |
|---|---|---|---|
| 0x2002 | Battery Level (0x2A19) | Read, Notify | Always 100 (0x64) |
| 0x2003 | CCC descriptor | — | Client notification config |

#### Motion Detection Service (Custom 128-bit UUID: 4D4F4E00-...)

| Handle | Characteristic | Properties | Meaning |
|---|---|---|---|
| 0x3002 | Motion Status | Read, Write, Notify | 0x00 = no motion, 0x01 = motion detected |
| 0x3003 | CCC descriptor | — | Notification config for Motion Status |
| 0x3005 | Distance | Read, Notify | 2 bytes, big-endian, value in centimetres |
| 0x3006 | CCC descriptor | — | Notification config for Distance |

A connected phone can **write** to handle **0x3002** to manually override the motion status — useful for testing actuators downstream on the Thread mesh without placing an object in front of the sensor.

When a real motion event occurs, the device sends:
1. A GATT notification on 0x3002 (0x00 or 0x01).
2. A GATT notification on 0x3005 (2-byte distance in cm, big-endian).

#### Thread Config Service (Custom 128-bit UUID: D00RBELL0002-...)

| Handle | Characteristic | Properties | Default | Notes |
|---|---|---|---|---|
| 0x4002 | Network Name | Read/Write | "MotionNet" | Up to 16 UTF-8 bytes |
| 0x4004 | Network Key | Write only | — | 16 bytes (AES-128 master key) |
| 0x4006 | Channel | Read/Write | 15 | Range 11–26 |
| 0x4008 | PAN ID | Read/Write | 0xABCD | 2 bytes, little-endian |
| 0x400A | Join | Write only | — | Write 0x01 to trigger Thread join |
| 0x400C | Thread Status | Read/Notify | 0x00 | 0=disabled, 1=detached, 2=child, 3=router, 4=leader |

### BLE Connection Model

- Single peripheral connection (one phone at a time).
- No pairing or encryption enforced.
- LED **WHITE_COOL (GPIO 1)**: blinking 500 ms ↔ 500 ms while advertising, solid ON while connected.

### BLE Callbacks

| Callback | Trigger | Action |
|---|---|---|
| `BLE_Stack_Callback` | Adv start/stop, connect, disconnect | Post `kEventType_BleConnection` event |
| `BLE_CharacteristicRead_Callback` | Attribute read | Auto-served by static GATT table |
| `BLE_CharacteristicWrite_Callback` | Write from phone | Process Motion Status override or Thread config |
| `BLE_CCCD_Callback` | Notification (un)subscribe | Log enable/disable of each CCC |

---

## 8. OpenThread Networking

### Library

The application links against the pre-built **OpenThread Full Thread Device (FTD)** library:
```
Work/Light_Concurrent_qpg6200/gn_out/lib/libopenthread-qpg.a
```
This must be built first by running the `Light_Concurrent_qpg6200` make target.

### Supported Roles

Child, Router, Leader (full device — capable of routing mesh traffic).

### BLE Commissioning Workflow

```
1. Phone connects to "QPG MaxSonar Motion" via BLE.
2. Phone writes Network Name   → handle 0x4002
   Phone writes Network Key    → handle 0x4004
   (Optionally) Channel        → handle 0x4006
   (Optionally) PAN ID         → handle 0x4008
3. Phone writes 0x01           → handle 0x400A  (Join)
4. Device constructs otOperationalDataset from the written values.
5. Device calls otIp6SetEnabled() + otThreadSetEnabled().
6. Device role transitions: disabled → detached → child/router/leader.
7. Role changes sent as BLE notifications on handle 0x400C.
```

Credentials persist to NVM via `otDatasetSetActive()`. On the next boot the device reads them back with `otDatasetGetActive()` and joins automatically, bypassing BLE commissioning.

### Thread Initialisation (Thread_Init)

```
otInstanceInitSingle()
 ├─ otDatasetGetActive() → check NVM for saved credentials
 ├─ If found → Thread_StartJoin() automatically
 └─ If not  → wait for BLE commissioning
otSetStateChangedCallback(Thread_StateChangeCallback)
```

### Network Join (Thread_StartJoin)

```
Build otOperationalDataset from BLE-written or NVM values:
  - Network Name, Network Key, Channel, PAN ID
otDatasetSetActive()
otIp6SetEnabled(true)
otThreadSetEnabled(true)
otUdpOpen(socket)              ← Open motion event UDP socket
otUdpBind(port 5683)           ← Bind to CoAP port
LED: GREEN blinks 200 ms ↔ 200 ms while joining
```

### Motion Event Multicast

| Parameter | Value |
|---|---|
| Protocol | UDP over Thread mesh |
| Destination address | ff03::1 (realm-local all-nodes multicast) |
| Port | 5683 (CoAP default) |
| Payload | 4 bytes |

**Payload layout:**

| Byte | Value | Meaning |
|---|---|---|
| 0 | 0x01 | Message type: motion event |
| 1 | 0x00 or 0x01 | State: 0x00 = cleared, 0x01 = detected |
| 2 | (distance >> 8) | Distance in cm, high byte |
| 3 | (distance & 0xFF) | Distance in cm, low byte |

**Example:** Object at 120 cm detected → `01 01 00 78`.
Object moves away → `01 00 00 C8` (200 cm, cleared).

### Receive Path

`Thread_UdpReceiveCallback()` listens on the same socket. It validates:
- Byte 0 == 0x01 (message type = motion event)

Then reads byte 1 (state) and bytes 2–3 (distance). It posts `kThreadEvent_MotionReceived` with the distance as the value, which triggers `ReportMotion(fromThread=true)` in the event handler. Events originating from the Thread mesh do **not** generate another UDP multicast (loop prevention).

---

## 9. Hardware Peripherals

### GPIO Map

| GPIO | Name | Direction | Connected To | Function |
|---|---|---|---|---|
| 1 | WHITE_COOL LED | Output (active-high) | White LED | BLE state indicator |
| 3 | PB1 | Input (pull-up) | Commissioning button | Short (2–4 s) = restart BLE adv; Long (≥5 s) = factory reset |
| 8 | UART1 RX | Input | MaxSonar TX (pin 5) | Receives ASCII range frames |
| 9 | UART1 TX | Output | MaxSonar RX (pin 4) | Held HIGH → sensor free-run mode |
| 11 | GREEN LED | Output (active-high) | Green LED | Thread state indicator |
| 12 | BLUE LED | Output (active-high) | Blue LED | Motion detection indicator |

### LED State Reference

| LED | GPIO | Blinking | Solid ON | OFF |
|---|---|---|---|---|
| WHITE (BLE) | 1 | Advertising (500/500 ms) | Phone connected | Disconnected |
| GREEN (Thread) | 11 | Thread joining (200/200 ms) | Attached to network | Detached / disabled |
| BLUE (Motion) | 12 | — | Object within 2 m | No motion |

Note: BLUE is **solid ON** (not blinking) when motion is detected. It turns off immediately when the object moves beyond the threshold.

### Commissioning Button (PB1, GPIO 3)

Uses the platform `AppButtons` component (interrupt-driven, tracks hold duration):

- **2–4 s hold**: Restart BLE advertising (useful after phone disconnects).
- **≥5 s hold**: Call `otInstanceFactoryReset()` then `AppTask::ResetSystem()` → software POR, credentials erased from NVM.

---

## 10. Application State Machine & Event Flow

### AppEvent Types

```cpp
enum AppEventTypes {
  kEventType_Buttons       = 0,  // PB1 commissioning button
  kEventType_BleConnection  = 1, // BLE stack events
  kEventType_Sensor         = 2, // SensorManager distance events
  kEventType_Thread         = 3, // OpenThread events
  kEventType_Invalid        = 255
};
```

### Sensor Event Sub-Types

```cpp
enum SensorEventTypes {
  kSensorEvent_MotionDetected = 0,  // Object within 200 cm
  kSensorEvent_MotionCleared  = 1,  // Object beyond 200 cm
};
// SensorEvent_t also carries: uint16_t distanceCm
```

### Thread Event Sub-Types

```cpp
enum ThreadEventTypes {
  kThreadEvent_Joined         = 0,  // Attached (child/router/leader)
  kThreadEvent_Detached       = 1,  // Lost network
  kThreadEvent_MotionReceived = 2,  // Remote node motion UDP received
  kThreadEvent_Error          = 3,  // Internal error
};
```

### Full Event Flow Diagram

```
Source                          → Queue Entry              → Handler Method
─────────────────────────────────────────────────────────────────────────────
SensorManager polling task      → kEventType_Sensor        → SensorEventHandler()
BLE stack callback              → kEventType_BleConnection → BleEventHandler()
BLE characteristic write        → (direct in callback)     → Immediate processing
OpenThread state callback       → kEventType_Thread        → ThreadEventHandler()
OpenThread UDP receive          → kEventType_Thread        → ThreadEventHandler()
PB1 button component            → kEventType_Buttons       → ButtonEventHandler()
```

### Motion Reporting (ReportMotion)

`ReportMotion(bool detected, uint16_t distanceCm, bool fromThread)` is the single entry point for all motion state changes:

```
ReportMotion(detected, distanceCm, fromThread)
  1. Update BLUE LED:
       detected=true  → StatusLed_SetLed(LED_MOTION, true)   // Solid ON
       detected=false → StatusLed_SetLed(LED_MOTION, false)  // OFF
  2. Update GATT value buffers:
       motionStatusValue[0] = detected ? 0x01 : 0x00
       distanceValue[0]     = (distanceCm >> 8) & 0xFF
       distanceValue[1]     =  distanceCm       & 0xFF
  3. If BLE connected and Motion Status CCC enabled:
       Send GATT notification on handle 0x3002
  4. If BLE connected and Distance CCC enabled:
       Send GATT notification on handle 0x3005
  5. If (!fromThread):          // Avoid re-broadcasting a received multicast
       Thread_SendMotionMulticast(detected, distanceCm)
         a. Verify Thread is in child/router/leader role
         b. Build 4-byte UDP payload {0x01, state, dist_hi, dist_lo}
         c. otUdpSend() to ff03::1:5683
         d. Log result
  6. Log motion event with distance and source label
```

### BLE Event Handling

| Sub-event | Action |
|---|---|
| `BLEIF_DM_ADV_START_IND` | Blink WHITE LED 500/500 ms |
| `BLEIF_DM_CONN_OPEN_IND` | Solid WHITE LED ON |
| `BLEIF_DM_CONN_CLOSE_IND` | WHITE LED OFF, restart advertising |

### Thread Event Handling

| Sub-event | Action |
|---|---|
| `kThreadEvent_Joined` | Solid GREEN ON; send BLE Thread Status notification |
| `kThreadEvent_Detached` | GREEN OFF; send BLE Thread Status notification |
| `kThreadEvent_MotionReceived` | Call `ReportMotion(state, distance, fromThread=true)` |
| `kThreadEvent_Error` | Log error; LED unchanged |

---

## 11. Configuration Parameters

### Sensor (SensorManager.h)

| Constant | Default | Meaning |
|---|---|---|
| `MOTION_DISTANCE_THRESHOLD_CM` | 200 | Object must be ≤ this distance (cm) to trigger motion detected |
| `SENSOR_POLL_MS` | 50 | UART polling interval in milliseconds |

### Button Hold Thresholds (AppManager.cpp)

| Constant | Value | Action |
|---|---|---|
| `BTN_RESTART_ADV_THRESHOLD` | 2 s | Restart BLE advertising |
| `BTN_FACTORY_RESET_THRESHOLD` | 5 s | Factory reset + reboot |

### LED Timing (AppManager.cpp)

| Constant | Value | LED / State |
|---|---|---|
| `ADV_BLINK_ON_MS` / `ADV_BLINK_OFF_MS` | 500 / 500 ms | WHITE during BLE advertising |
| `THREAD_JOIN_BLINK_ON_MS` / `THREAD_JOIN_BLINK_OFF_MS` | 200 / 200 ms | GREEN while joining Thread |

### BLE Advertising (MotionDetector_Config.h)

| Constant | Value | Meaning |
|---|---|---|
| `BLE_ADV_INTERVAL_MIN` | 0x0020 | ~20 ms minimum advertising interval |
| `BLE_ADV_INTERVAL_MAX` | 0x0060 | ~60 ms maximum advertising interval |
| `BLE_ADV_BROADCAST_DURATION` | 0xF000 | ~40 s per advertising window |

### Thread Network Defaults (MotionDetector_Config.c)

| Parameter | Default | Writable via BLE |
|---|---|---|
| Network Name | "MotionNet" | Yes, handle 0x4002 |
| Channel | 15 | Yes, handle 0x4006 |
| PAN ID | 0xABCD | Yes, handle 0x4008 |
| Network Key | — | Yes (write-only), handle 0x4004 |

### GATT Characteristic Sizes (MotionDetector_Config.h)

| Constant | Value |
|---|---|
| `THREAD_NET_NAME_MAX_LEN` | 16 bytes |
| `THREAD_NET_KEY_LEN` | 16 bytes (128-bit) |
| `THREAD_PANID_LEN` | 2 bytes |
| `THREAD_CHANNEL_LEN` | 1 byte |
| `THREAD_STATUS_LEN` | 1 byte |

### FreeRTOS Task Settings (AppTask.cpp)

| Constant | Value |
|---|---|
| `APP_EVENT_QUEUE_SIZE` | 20 events |
| `APP_TASK_STACK_SIZE` | 6 KB |
| `APP_TASK_PRIORITY` | 2 |
| `APP_GOTOSLEEP_THRESHOLD` | 1000 µs |

---

## 12. Build System

### Prerequisites

The OpenThread FTD library must be built first:
```bash
make -f Applications/Concurrent/Light/Makefile.Light_Concurrent_qpg6200
```
This produces:
```
Work/Light_Concurrent_qpg6200/gn_out/lib/libopenthread-qpg.a
```

### Building the Application

```bash
make -f Applications/Ble/ThreadBleMotionDetector_MaxSonar/Makefile.ThreadBleMotionDetector_MaxSonar_qpg6200
```

### Key Compiler Flags

| Flag | Meaning |
|---|---|
| `-march=armv7e-m -mcpu=cortex-m4` | Target Cortex-M4 |
| `-mfpu=fpv4-sp-d16 -mfloat-abi=hard` | Hardware FPU |
| `-mthumb` | Thumb-2 instruction set |
| `-Os` | Optimise for size |
| `-std=gnu++17` | C++17 with GCC extensions |
| `-fno-rtti -fno-use-cxa-atexit` | Reduce C++ overhead |
| `-DESEC_AUTH_ALGO=ESEC_AUTH_ALGO_ECDSA_P256` | ECDSA P-256 signing |

### Build Outputs (`Work/ThreadBleMotionDetector_MaxSonar_qpg6200/`)

| File | Content |
|---|---|
| `*.elf` | ELF with full debug symbols |
| `*.hex` | Signed firmware + bootloader merged, ready to flash |
| `*.map` | Linker map (memory layout) |
| `*.memoryoverview` | Human-readable memory usage report |

### Post-Build Script

`ThreadBleMotionDetector_MaxSonar_qpg6200_postbuild.sh` performs:
1. Signs the firmware binary with ECDSA P-256 using the developer key.
2. Merges the signed firmware with the bootloader hex image.
3. Generates the memory overview report.

---

## 13. Key Data Structures

### AppEvent

```cpp
struct AppEvent {
  enum AppEventTypes { ... } Type;

  union {
    ButtonEvent_t     ButtonEvent;        // PB1 hold duration
    Ble_Event_t       BleConnectionEvent; // connect, disconnect, adv events
    SensorEvent_t     SensorEvent;        // { EventType: detected/cleared, DistanceCm }
    ThreadEvent_t     ThreadEvent;        // { Event: joined/detached/motion, Value: role/dist }
  };
};
```

### SensorEvent_t

```cpp
struct SensorEvent_t {
  enum { kSensorEvent_MotionDetected, kSensorEvent_MotionCleared } EventType;
  uint16_t DistanceCm;  // Raw distance from sensor at time of event
};
```

### GATT Motion Status Value

```c
// Handle 0x3002 — 1 byte
uint8_t motionStatusValue[1];  // 0x00 = no motion, 0x01 = detected

// Handle 0x3005 — 2 bytes, big-endian
uint8_t distanceValue[2];
// distanceValue[0] = (cm >> 8) & 0xFF  (high byte)
// distanceValue[1] =  cm       & 0xFF  (low byte)
```

### Thread Credentials (in-memory)

```c
// Stored/loaded via otDatasetGetActive() / otDatasetSetActive()
uint8_t threadNetNameValue[16]; // UTF-8 network name  (default "MotionNet")
uint8_t threadNetKeyValue[16];  // 128-bit network master key
uint8_t threadChannelValue[1];  // Channel number (11–26, default 15)
uint8_t threadPanIdValue[2];    // PAN ID, little-endian (default 0xABCD)
```

### Thread UDP Payload (Motion Event)

```
Offset  Length  Value       Meaning
──────  ──────  ──────────  ────────────────────────────
  0       1     0x01        Message type: motion event
  1       1     0x00/0x01   State: 0=cleared, 1=detected
  2       1     (cm >> 8)   Distance high byte
  3       1     (cm & 0xFF) Distance low byte
```
