# ThreadBleDoorbell_DK — Technical Overview

## Table of Contents

1. [Purpose & Summary](#1-purpose--summary)
2. [Hardware Platform](#2-hardware-platform)
3. [Software Architecture](#3-software-architecture)
4. [Source File Reference](#4-source-file-reference)
5. [BLE Functionality](#5-ble-functionality)
6. [OpenThread Networking](#6-openthread-networking)
7. [Hardware Peripherals](#7-hardware-peripherals)
8. [Application State Machine & Event Flow](#8-application-state-machine--event-flow)
9. [Ring Event Flow](#9-ring-event-flow)
10. [Configuration Parameters](#10-configuration-parameters)
11. [Build System](#11-build-system)
12. [Key Data Structures](#12-key-data-structures)

---

## 1. Purpose & Summary

**ThreadBleDoorbell_DK** is a dual-protocol IoT demo application for the QPG6200L Development Kit. It implements a smart doorbell that:

- Advertises over **Bluetooth LE (BLE)** so a smartphone can commission it onto a Thread mesh network and trigger ring events remotely.
- Joins an **OpenThread** mesh and multicasts every ring event to all other nodes on the network (e.g., indoor chime units, Node-RED dashboard via a Thread Border Router).
- Accepts ring input from **three independent sources**: the physical doorbell button (GPIO 5), a connected phone via BLE GATT write, or a remote Thread node's multicast UDP packet.

The application targets the QPG6200L DK directly — no carrier board is required. All indicators and inputs are the on-board LEDs and buttons.

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

The application follows a **singleton event-driven architecture** running on FreeRTOS. There are three main classes and one utility class:

```
main()
 └─ gpBaseComps_StackInit()          ← Initialises BLE + Thread radio stacks
 └─ AppTask::Init()
     ├─ Creates FreeRTOS event queue  (20 events)
     ├─ Spawns AppTask::Main() task   (6 KB stack, priority 2)
     ├─ AppManager::Init()
     │    ├─ BleIf_Init()             ← Registers BLE callbacks
     │    ├─ StatusLed_Init()         ← Configures 3 GPIO LEDs
     │    ├─ AppButtons registration  ← PB1 commissioning button
     │    ├─ Thread_Init()            ← Creates OpenThread instance
     │    └─ BleIf_StartAdvertising() ← Begins advertising "QPG Thread Doorbell"
     └─ DoorbellManager::Init() + StartPolling()
          └─ Spawns GPIO poll task    (50 ms interval, priority idle+1)

AppTask::Main()  [forever]
 └─ xQueueReceive() blocks on event queue
 └─ AppManager::EventHandler() dispatches to per-type handler
```

All event sources — BLE stack callbacks, GPIO polling, OpenThread state-change callbacks, and UDP receive callbacks — funnel events into the same central FreeRTOS queue. This decouples producers from consumers and ensures all application logic runs on a single task with no locking required.

---

## 4. Source File Reference

### Header Files (`inc/`)

| File | Role |
|---|---|
| `AppManager.h` | Core application manager class declaration. Handles BLE/Thread init, LED state machine, and ring logic. |
| `AppTask.h` | FreeRTOS task wrapper — event queue creation, system reset. |
| `AppEvent.h` | Unified `AppEvent` union covering Button, BLE, Analog (doorbell), and Thread sub-events. |
| `DoorbellManager.h` | GPIO 5 (PB2) polling task with debounce configuration constants. |
| `ThreadBleDoorbell_Config.h` | BLE GATT attribute handle definitions and Thread Config Service UUIDs. |
| `Ble_Peripheral_Config.h` | Thin shim that re-includes `ThreadBleDoorbell_Config.h`. |
| `qPinCfg.h` | Canonical GPIO number assignments for all board peripherals. |

### Implementation Files (`src/`)

| File | Lines | Role |
|---|---|---|
| `AppManager.cpp` | ~846 | All application logic: BLE callbacks, Thread callbacks, ring processing, LED control. |
| `AppTask.cpp` | ~245 | FreeRTOS task lifecycle, event queue, thread-safe `PostEvent()`, `ResetSystem()`. |
| `DoorbellManager.cpp` | ~177 | GPIO 5 input configuration, polling task, 3-sample debounce, `NotifyAnalogEvent()`. |
| `ThreadBleDoorbell_Config.c` | ~346 | Static GATT attribute tables for all three BLE services; advertising data; Thread config NVM accessors. |
| `platform_memory.c` | ~20 | `otPlatCAlloc()` / `otPlatFree()` — OpenThread heap allocation wrappers. |

### Generated Files (`gen/ThreadBleDoorbell_DK_qpg6200/`)

| File | Role |
|---|---|
| `qorvo_config.h` | Build-time configuration: BLE buffer counts, UART baud rate (115200), enabled components. |
| `qorvo_internals.h` | Component manifest listing every enabled SW module (BLE, Thread, FreeRTOS, crypto, etc.). |
| `esec_config.h` | Hardware security engine flags: AES, SHA, ECC, CMAC. |
| `q_info_page.h` | Bootloader info-page structures: programming pins, device attestation, license data. |
| `gpHal_RtSystem.c` | Flash-resident runtime system data (auto-generated). |
| `ThreadBleDoorbell_DK_qpg6200.ld` | ARM linker script — memory map and section assignments. |

---

## 5. BLE Functionality

### Advertising

| Parameter | Value |
|---|---|
| Device name | "QPG Thread Doorbell" |
| Advertising interval | 20 – 60 ms |
| Advertising data | Flags (0x06: General Discoverable + no BR/EDR) + Battery Service UUID (0x180F) |
| Scan response | Complete local name "QPG Thread Doorbell" |
| BLE stack | Cordio BLE Host (ARM reference) |

### GATT Services

#### Battery Service (UUID 0x180F — Standard Bluetooth)

| Handle | Characteristic | Properties | Value |
|---|---|---|---|
| 0x2002 | Battery Level (0x2A19) | Read, Notify | Always 100 (0x64) |
| 0x2003 | CCC descriptor | — | Client notification config |

#### Doorbell Ring Service (Custom 128-bit UUID: D00RBELL-0000-...)

| Handle | Characteristic | Properties | Meaning |
|---|---|---|---|
| 0x3002 | Ring State | Read, Write, Notify | 0x00 = idle, 0x01 = ringing |
| 0x3003 | CCC descriptor | — | Client notification config |

A phone writes `0x01` to handle **0x3002** to trigger a remote ring. The device sends a notification to the phone whenever any ring event occurs (local button, remote BLE, or Thread multicast).

#### Thread Config Service (Custom 128-bit UUID: D00RBELL-0002-...)

| Handle | Characteristic | Properties | Default | Notes |
|---|---|---|---|---|
| 0x4002 | Network Name | Read/Write | "DoorbellNet" | Up to 16 UTF-8 bytes |
| 0x4004 | Network Key | Write only | — | 16 bytes (AES-128 master key) |
| 0x4006 | Channel | Read/Write | 15 | Range 11–26 |
| 0x4008 | PAN ID | Read/Write | 0xABCD | 2 bytes, little-endian |
| 0x400A | Join | Write only | — | Write 0x01 to start Thread join |
| 0x400C | Thread Status | Read/Notify | 0x00 | 0=disabled, 1=detached, 2=child, 3=router, 4=leader |

### BLE Connection Model

- Single peripheral connection (one phone at a time).
- No pairing or encryption enforced (`DM_SEC_LEVEL_NONE`).
- LED **WHITE_COOL (GPIO 1)**: blinking 500 ms ↔ 500 ms while advertising, solid ON while connected.

### BLE Callbacks Registered in AppManager

| Callback | Trigger | Action |
|---|---|---|
| `BLE_Stack_Callback` | Adv start/stop, connect, disconnect | Post `kEventType_BleConnection` event |
| `BLE_CharacteristicRead_Callback` | Attribute read (mostly static) | Auto-served by GATT table |
| `BLE_CharacteristicWrite_Callback` | Attribute write from phone | Post ring or Thread config event |
| `BLE_CCCD_Callback` | Notification (un)subscribe | Track which CCC is enabled |

---

## 6. OpenThread Networking

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
1. Phone connects to "QPG Thread Doorbell" via BLE.
2. Phone writes Network Name  → handle 0x4002
   Phone writes Network Key   → handle 0x4004
   (Optionally) Channel       → handle 0x4006
   (Optionally) PAN ID        → handle 0x4008
3. Phone writes 0x01          → handle 0x400A  (Join)
4. Device constructs otOperationalDataset from the written values.
5. Device calls otIp6SetEnabled() + otThreadSetEnabled().
6. Device role transitions: disabled → detached → child/router/leader.
7. Role changes are sent as BLE notifications on handle 0x400C.
```

Credentials are persisted to NVM via `otDatasetSetActive()`. On the next boot the device reads them back with `otDatasetGetActive()` and joins automatically without requiring BLE commissioning again.

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
otUdpOpen(socket)              ← Open ring event UDP socket
otUdpBind(port 5683)           ← Bind to CoAP port on all interfaces
LED: GREEN blinks 200 ms ↔ 200 ms while joining
```

### Ring Event Multicast

| Parameter | Value |
|---|---|
| Protocol | UDP over Thread mesh |
| Destination address | ff03::1 (realm-local all-nodes multicast) |
| Port | 5683 (CoAP default) |
| Payload | 4 bytes |

**Payload layout:**

| Byte | Value | Meaning |
|---|---|---|
| 0 | 0x02 | Message type: doorbell ring event |
| 1 | 0x01 | State: ringing |
| 2 | (count >> 8) | Ring count, high byte |
| 3 | (count & 0xFF) | Ring count, low byte |

Example: fifth press → `02 01 00 05`.

### Receive Path

`Thread_UdpReceiveCallback()` listens on the same socket. It validates:
- Byte 0 == 0x02 (message type)
- Byte 1 == 0x01 (ringing)

Then posts `kThreadEvent_RingReceived` with the ring count as the value. This triggers `RingDoorbell(fromThread=true)` in the main event handler, which runs the local ring effects (LED blink, BLE notify) **without** re-multicasting (to avoid looping).

---

## 7. Hardware Peripherals

### GPIO Map

| GPIO | Name | Direction | Connected To | Function |
|---|---|---|---|---|
| 1 | WHITE_COOL LED | Output (active-high) | White LED | BLE state indicator |
| 3 | PB1 | Input (pull-up) | Commissioning button | Short (<2 s) = restart BLE adv; Long (≥5 s) = factory reset |
| 5 | PB2 | Input (pull-up) | **Doorbell button** | Active-low, polled every 50 ms, 3-sample debounce |
| 11 | GREEN LED | Output (active-high) | Green LED | Thread state indicator |
| 12 | BLUE LED | Output (active-high) | Blue LED | Ring event indicator |
| RESETN | SW1 | Input | Hardware reset | System power-on reset |

### LED State Reference

| LED | GPIO | Blinking | Solid ON | OFF |
|---|---|---|---|---|
| WHITE (BLE) | 1 | BLE advertising (500/500 ms) | Phone connected | Disconnected |
| GREEN (Thread) | 11 | Thread joining (200/200 ms) | Attached to network | Detached / disabled |
| BLUE (Ring) | 12 | Ring event in progress (100/100 ms) | — | Idle |

### Doorbell Button — DoorbellManager

The doorbell button is polled (not interrupt-driven) to prevent noise from long cable runs:

```
FreeRTOS polling task (priority idle+1):
  Every 50 ms:
    read = qDrvGPIO_Read(GPIO_5)
    currentlyPressed = !read          // Active-low: low = pressed
    if (currentlyPressed == lastSample):
      debounceCount++
    else:
      debounceCount = 0
      lastSample = currentlyPressed
    if (debounceCount == 3):          // 3 × 50 ms = 150 ms debounce
      if (currentlyPressed != stableState):
        stableState = currentlyPressed
        AppManager::NotifyAnalogEvent(stableState, 0)
```

### Commissioning Button (PB1) — AppButtons Component

Uses the platform `ButtonHandler` component. Detects hold duration by tracking press and release timestamps:
- **2–4 s hold**: Restart BLE advertising (useful after phone disconnects).
- **≥5 s hold**: Call `otInstanceFactoryReset()` then `AppTask::ResetSystem()` → software POR.

---

## 8. Application State Machine & Event Flow

### AppEvent Types

```cpp
enum AppEventTypes {
  kEventType_Buttons      = 0,  // PB1 commissioning button
  kEventType_BleConnection = 1, // BLE stack events
  kEventType_Analog        = 2, // GPIO 5 doorbell button
  kEventType_Thread        = 3, // OpenThread events
  kEventType_Invalid       = 255
};
```

### Event Sources → Queue → Handlers

```
Source                       → Queue Entry              → Handler Method
────────────────────────────────────────────────────────────────────────
BLE stack callback           → kEventType_BleConnection → BleEventHandler()
BLE characteristic write     → (direct in callback)     → Immediate processing
GPIO 5 polling task          → kEventType_Analog        → AnalogEventHandler()
OpenThread state callback    → kEventType_Thread        → ThreadEventHandler()
OpenThread UDP receive       → kEventType_Thread        → ThreadEventHandler()
PB1 button component         → kEventType_Buttons       → ButtonEventHandler()
```

### BLE Event Sub-Types

| Sub-event | Action |
|---|---|
| `BLEIF_DM_ADV_START_IND` | Blink WHITE LED 500/500 ms |
| `BLEIF_DM_CONN_OPEN_IND` | Solid WHITE LED ON |
| `BLEIF_DM_CONN_CLOSE_IND` | WHITE LED OFF, restart advertising |

### Thread Event Sub-Types

| Sub-event | Action |
|---|---|
| `kThreadEvent_Joined` | Solid GREEN ON; send BLE status notification (role value) |
| `kThreadEvent_Detached` | GREEN OFF; send BLE status notification (0x01 = detached) |
| `kThreadEvent_RingReceived` | Call `RingDoorbell(fromThread=true)` |
| `kThreadEvent_Error` | Log error; LED unchanged |

---

## 9. Ring Event Flow

`RingDoorbell(fromThread, fromPhone)` is the single entry point for all ring actions:

```
RingDoorbell(bool fromThread, bool fromPhone)
  1. sRingCount++
  2. Blink BLUE LED rapidly (100 ms on, 100 ms off)
  3. If BLE phone connected and CCC enabled:
       Send GATT notification on handle 0x3002 → value 0x01
  4. If (!fromThread):           // Avoid re-broadcasting a received multicast
       Thread_SendRingMulticast()
         a. Verify Thread is in child/router/leader role
         b. Build 4-byte UDP payload {0x02, 0x01, count_hi, count_lo}
         c. otUdpSend() to ff03::1:5683
         d. Log result
  5. Log "DING DONG! Ring #N" with source label
```

All three ring triggers (local button, BLE phone, Thread multicast) produce identical local effects: BLUE LED blinks, BLE notification sent to the connected phone, and (unless the event originated from the mesh) a UDP multicast is dispatched to the Thread network.

---

## 10. Configuration Parameters

### Doorbell Button (DoorbellManager.h)

| Constant | Value | Meaning |
|---|---|---|
| `DOORBELL_POLL_MS` | 50 | GPIO polling interval in milliseconds |
| `DOORBELL_DEBOUNCE_COUNT` | 3 | Consecutive matching samples before state change is accepted |

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
| `RING_BLINK_ON_MS` / `RING_BLINK_OFF_MS` | 100 / 100 ms | BLUE during ring event |

### BLE Advertising (ThreadBleDoorbell_Config.h)

| Constant | Value | Meaning |
|---|---|---|
| `BLE_ADV_INTERVAL_MIN` | 0x0020 | 20 ms minimum advertising interval |
| `BLE_ADV_INTERVAL_MAX` | 0x0060 | 60 ms maximum advertising interval |
| `BLE_ADV_BROADCAST_DURATION` | 0xF000 | ~60 s per advertising window |

### Thread Network Defaults (ThreadBleDoorbell_Config.c)

| Parameter | Default | Writable via BLE |
|---|---|---|
| Network Name | "DoorbellNet" | Yes, handle 0x4002 |
| Channel | 15 | Yes, handle 0x4006 |
| PAN ID | 0xABCD | Yes, handle 0x4008 |
| Network Key | — | Yes (write-only), handle 0x4004 |

### GATT Characteristic Sizes (ThreadBleDoorbell_Config.h)

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

## 11. Build System

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
make -f Applications/Ble/ThreadBleDoorbell_DK/Makefile.ThreadBleDoorbell_DK_qpg6200
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

### Build Outputs (`Work/ThreadBleDoorbell_DK_qpg6200/`)

| File | Content |
|---|---|
| `*.elf` | ELF with full debug symbols |
| `*.hex` | Signed firmware + bootloader merged, ready to flash |
| `*.map` | Linker map (memory layout) |
| `*.memoryoverview` | Human-readable memory usage report |

### Post-Build Script

`ThreadBleDoorbell_DK_qpg6200_postbuild.sh` performs three steps:
1. Signs the firmware binary with ECDSA P-256 using the developer key.
2. Merges the signed firmware with the bootloader hex image.
3. Generates the memory overview report.

---

## 12. Key Data Structures

### AppEvent

```cpp
struct AppEvent {
  enum AppEventTypes { ... } Type;

  union {
    ButtonEvent_t     ButtonEvent;        // PB1 hold duration
    Ble_Event_t       BleConnectionEvent; // connect, disconnect, adv events
    AnalogEvent_t     AnalogEvent;        // { State: pressed/released, AdcRaw: 0 }
    ThreadEvent_t     ThreadEvent;        // { Event: joined/detached/ring, Value: role/count }
  };
};
```

### GATT Attribute Entry (Cordio attsAttr_t)

```c
typedef struct {
  uint8_t*  pUuid;       // Pointer to 16-bit or 128-bit UUID
  uint8_t*  pValue;      // Pointer to characteristic value buffer
  uint16_t* pValueLen;   // Pointer to current length
  uint16_t  maxLen;      // Maximum allowed length
  uint8_t   properties;  // ATTS_SET_* flags (read, write, notify, …)
  uint8_t   permissions; // ATTS_PERMIT_* flags
} attsAttr_t;
```

### GATT Service Groups

```c
// Three service groups registered with the Cordio attribute server:
{handle range 0x2000–0x2003}  // Battery Service
{handle range 0x3000–0x3003}  // Doorbell Ring Service
{handle range 0x4000–0x400D}  // Thread Config Service
```

### Thread Credentials (in-memory)

```c
// Stored/loaded via otDatasetGetActive() / otDatasetSetActive()
uint8_t threadNetNameValue[16]; // UTF-8 network name
uint8_t threadNetKeyValue[16];  // 128-bit network master key
uint8_t threadChannelValue[1];  // Channel number (11–26)
uint8_t threadPanIdValue[2];    // PAN ID, little-endian
```
