# Thread+BLE Doorbell DK Analog

## Introduction

Qorvo&reg; QPG6200 Thread+BLE Doorbell DK Analog is a demo application that combines Bluetooth&reg; LE and OpenThread on a single QPG6200L chip. It is a standalone development-kit variant of the Thread+BLE Doorbell that reads the doorbell button via the on-chip **GPADC analog peripheral** on **ANIO1 (GPIO 29)**, accessible directly from the DK expansion header with no carrier board or jumper required.

Key features:

- **BLE commissioning** — a phone connects over BLE to provision Thread network credentials onto the device.
- **Thread mesh** — once commissioned, the device joins a Thread network and multicasts a UDP ring event to all mesh nodes on every doorbell press.
- **Analog doorbell input** — a physical push-button is read via the on-chip GPADC (GPIO 29 / ANIO1), with voltage-threshold detection and hysteresis debouncing, providing noise immunity superior to a simple digital GPIO read.

> **Variants**
> | Application | Doorbell input | Carrier board needed? |
> |-------------|---------------|----------------------|
> | `ThreadBleDoorbell` | ANIO0 / GPIO 28 (analog) | Yes — IoT Carrier Board + J24 jumper |
> | `ThreadBleDoorbell_DK` | PB2 / GPIO 5 (digital) | No |
> | **`ThreadBleDoorbell_DK_Analog`** | **ANIO1 / GPIO 29 (analog)** | **No** |

---

## Hardware Setup

**Board:** QPG6200L Development Kit (QPG6200L DK) — standalone, no carrier board required.

### Doorbell button wiring (GPIO 29 / ANIO1)

Connect the following to the DK expansion header:

```
3.3 V ──┬── [Push button] ── GPIO 29 (ANIO1)
        │                          │
       (open)                  [10 kΩ]
                                   │
                                  GND
```

| Pin | Signal | Notes |
|-----|--------|-------|
| 3.3 V | Supply | DK 3.3 V power header pin |
| GPIO 29 | ANIO1 analog input | DK expansion header |
| GND | Ground | DK GND header pin |

- **Button released:** GPIO 29 pulled low through 10 kΩ → ~0 V → below release threshold (500 mV)
- **Button pressed:** GPIO 29 connected to 3.3 V → ~3.3 V → above press threshold (1500 mV)

The GPADC applies hysteresis (press > 1500 mV, release < 500 mV) and requires 3 consecutive matching samples (300 ms at 100 ms poll rate) before registering a state change.

---

## QPG6200 GPIO Configurations

| GPIO | Direction | Connected To | Function |
|:----:|:---------:|:------------:|:---------|
| RESETN | Input | SW1 | Hardware reset |
| GPIO 3 | Input (Pull-up) | PB1 | Short press (<2 s): restart BLE advertising / Long press (≥5 s): factory-reset Thread credentials |
| GPIO 1 | Output | WHITE_COOL LED | BLE connection status |
| GPIO 11 | Output | GREEN LED | Thread network status |
| GPIO 12 | Output | BLUE LED | Doorbell ring indicator |
| GPIO 29 | Analog input | ANIO1 (DK expansion header) | Analog doorbell push-button |

---

## Application Usage

### Boot behaviour

On power-up the device automatically starts BLE advertising as **"QPG Thread Doorbell"**. If Thread credentials were previously stored in NVM the device also attempts to (re)join the Thread network.

### Commissioning via BLE

| Step | Action | Notes |
|------|--------|-------|
| 1 | Connect to **"QPG Thread Doorbell"** with nRF Connect or Qorvo Connect | BLE advertising must be active (WHITE_COOL LED blinking) |
| 2 | Write **Thread Network Name** characteristic | E.g. `"DoorbellNet"` |
| 3 | Write **Thread Network Key** characteristic | 16-byte master key (write-only) |
| 4 | *(Optional)* Write **Channel** characteristic | 1 byte, 11–26 (default 15) |
| 5 | *(Optional)* Write **PAN ID** characteristic | 2 bytes little-endian (default 0xABCD) |
| 6 | Write **Join** characteristic with value `0x01` | Device attempts to join the Thread network |
| 7 | Subscribe to **Thread Status** notifications | Reports device role changes |

### Ring events

A ring event is triggered by any of the following:

- Pressing the analog doorbell button (GPIO 29 / ANIO1) — voltage rises above 1500 mV
- Writing `0x01` to the **Ring** BLE characteristic from a connected phone
- Receiving a UDP multicast ring packet from another Thread mesh node

On a ring event the BLUE LED blinks rapidly, a BLE notification (value `0x01`) is sent to any connected phone, and a UDP multicast is sent to all nodes on the Thread mesh.

### Button summary

| Button | Action | Result |
|--------|--------|--------|
| PB1 | Short press (<2 s) | Restart BLE advertising |
| PB1 | Long press (≥5 s) | Factory-reset Thread credentials and reboot |

---

## GPADC Configuration

| Parameter | Value |
|-----------|-------|
| Analog input | ANIO1 (GPIO 29, alt 1) |
| Resolution | 11-bit (2048 steps over 0–3.6 V ≈ 1.76 mV/step) |
| Mode | Single-ended, high-voltage range |
| Conversion | Continuous, Buffer A |
| Poll interval | 100 ms |
| Press threshold | > 1500 mV |
| Release threshold | < 500 mV |
| Debounce count | 3 consecutive samples (300 ms) |

These thresholds can be overridden at compile time via:

```c
-DDOORBELL_ADC_PRESS_MV=<mV>
-DDOORBELL_ADC_RELEASE_MV=<mV>
-DDOORBELL_DEBOUNCE_COUNT=<count>
-DDOORBELL_ADC_POLL_MS=<ms>
```

---

## BLE GATT Services

```
Device: QPG Thread Doorbell
└── Services
    ├── Battery Service (0x180F)
    │   └── Battery Level (0x2A19)              Read, Notify
    ├── Doorbell Ring Service (custom 128-bit UUID)
    │   └── Ring Characteristic                 Read, Write, Notify
    │       0x00 = idle  |  0x01 = ringing
    └── Thread Config Service (custom 128-bit UUID)
        ├── Network Name                        Read, Write  (max 16 bytes UTF-8)
        ├── Network Key                         Write        (16 bytes)
        ├── Channel                             Read, Write  (1 byte, 11–26)
        ├── PAN ID                              Read, Write  (2 bytes LE)
        ├── Join                                Write        (0x01 = start join)
        └── Thread Status                       Read, Notify (0=disabled … 4=leader)
```

---

## Status Reporting

### BLE status (WHITE_COOL LED — GPIO 1)

| LED State | Meaning |
|-----------|---------|
| OFF | Disconnected, not advertising |
| Blinking (500 ms) | Advertising (waiting for phone) |
| Solid ON | Phone connected |

### Thread status (GREEN LED — GPIO 11)

| LED State | Meaning |
|-----------|---------|
| OFF | Detached / Thread disabled |
| Blinking (200 ms) | Joining / attaching to network |
| Solid ON | Attached (child, router, or leader) |

### Ring indicator (BLUE LED — GPIO 12)

| LED State | Meaning |
|-----------|---------|
| OFF | Idle |
| Rapid blink (100 ms) | Doorbell ring event active |

---

## Building

### Prerequisites

This application links against a pre-built OpenThread FTD library. Build it first using the Concurrent Light application:

```bash
make -f Applications/Concurrent/Light/Makefile.Light_Concurrent_qpg6200
```

This produces `Work/Light_Concurrent_qpg6200/gn_out/lib/libopenthread-qpg.a`.

### Build the application

```bash
make -f Applications/Ble/ThreadBleDoorbell_DK_Analog/Makefile.ThreadBleDoorbell_DK_Analog_qpg6200
```

Output files are written to `Work/ThreadBleDoorbell_DK_Analog_qpg6200/`:

| File | Description |
|------|-------------|
| `ThreadBleDoorbell_DK_Analog_qpg6200.elf` | ELF debug image |
| `ThreadBleDoorbell_DK_Analog_qpg6200.hex` | Signed firmware merged with bootloader, ready to flash |
| `ThreadBleDoorbell_DK_Analog_qpg6200.map` | Linker map file |

---

## Flashing

Please refer to the [Installation Guide](../../../sphinx/docs/QUICKSTART.rst#install-the-sdk) to set up a build environment, and to [Building and flashing the example applications](../../../sphinx/docs/QUICKSTART.rst#working-with-examples) for programming instructions.

## SDK API Reference

See the [API Manual](../../../Documents/API Manuals/API_Manuals.rst)
