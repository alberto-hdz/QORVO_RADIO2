# Thread+BLE Doorbell

## Introduction

Qorvo&reg; QPG6200 Thread+BLE Doorbell is a demo application that combines Bluetooth&reg; LE and OpenThread on a single QPG6200L chip. It demonstrates:

- **BLE commissioning** — a phone connects over BLE to provision Thread network credentials onto the device.
- **Thread mesh** — once commissioned, the device joins a Thread network and multicasts a UDP ring event to all mesh nodes on every doorbell press.
- **Analog doorbell input** — a physical push-button is read via the on-chip GPADC (GPIO 28 / ANIO0), avoiding the need for a dedicated digital GPIO.

This application can serve as a starting point for any dual-radio (BLE + Thread) IoT sensor or actuator product.

## Hardware Setup

**Board:** QPG6200L Development Kit (IoT Carrier Board)

Connect a push-button between **Pin 11** (ANIO0 / GPIO 28) and **3.3 V**, with a 10 kΩ pull-down resistor to GND. Set jumper **J24 to position 1-2** to route GPIO 28 to the ANIO0 analog input pad.

## QPG6200 GPIO Configurations

| GPIO | Direction | Connected To | Function |
|:----:|:---------:|:------------:|:---------|
| RESETN | Input | SW1 | Hardware reset |
| GPIO 3 | Input (Pull-up) | PB1 | Short press: restart BLE advertising / Long press (≥5 s): factory-reset Thread credentials |
| GPIO 1 | Output | WHITE_COOL LED | BLE connection status |
| GPIO 11 | Output | GREEN LED | Thread network status |
| GPIO 12 | Output | BLUE LED | Doorbell ring indicator |
| GPIO 28 | Analog input | ANIO0 (Pin 11) | Doorbell push-button (J24 jumper 1-2) |

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
- Pressing the analog doorbell button (GPIO 28 / ANIO0)
- Writing `0x01` to the **Ring** BLE characteristic from a connected phone
- Receiving a UDP multicast ring packet from another Thread mesh node

On a ring event the BLUE LED blinks rapidly and a UDP multicast is sent to all nodes on the Thread mesh.

### Button summary

| Button | Action | Result |
|--------|--------|--------|
| PB1 | Short press (<2 s) | Restart BLE advertising |
| PB1 | Long press (≥5 s) | Factory-reset Thread credentials and reboot |

## BLE GATT Services

```
Device: QPG Thread Doorbell
└── Services
    ├── Generic Access Service (0x1800)
    │   ├── Device Name (0x2A00)              Read
    │   └── Appearance (0x2A01)               Read
    ├── Device Information Service (0x180A)
    │   ├── Manufacturer Name (0x2A29)        Read
    │   ├── Model Number (0x2A24)             Read
    │   └── Firmware Revision (0x2A26)        Read
    └── Thread Commissioning Service (proprietary)
        ├── Network Name                      Read, Write
        ├── Network Key                       Write
        ├── Channel                           Read, Write
        ├── PAN ID                            Read, Write
        ├── Join                              Write
        ├── Thread Status                     Read, Notify
        └── Ring                              Write, Notify
```

## Status Reporting

### BLE status (WHITE_COOL LED — GPIO 1)

| LED State | Meaning |
|-----------|---------|
| OFF | Disconnected, not advertising |
| Blinking | Advertising (waiting for phone) |
| Solid ON | Phone connected |

### Thread status (GREEN LED — GPIO 11)

| LED State | Meaning |
|-----------|---------|
| OFF | Detached / Thread disabled |
| Blinking | Joining / attaching to network |
| Solid ON | Attached (child, router, or leader) |

### Ring indicator (BLUE LED — GPIO 12)

| LED State | Meaning |
|-----------|---------|
| OFF | Idle |
| Rapid blink | Doorbell ring event active |

## Building

### Prerequisites

This application links against a pre-built OpenThread FTD library. Build it first using the Concurrent Light application:

```bash
make -f Applications/Concurrent/Light/Makefile.Light_Concurrent_qpg6200
```

This produces `Work/Light_Concurrent_qpg6200/gn_out/lib/libopenthread-qpg.a`.

### Build the application

```bash
make -f Applications/Ble/ThreadBleDoorbell/Makefile.ThreadBleDoorbell_qpg6200
```

Output files are written to `Work/ThreadBleDoorbell_qpg6200/`:

| File | Description |
|------|-------------|
| `ThreadBleDoorbell_qpg6200.elf` | ELF debug image |
| `ThreadBleDoorbell_qpg6200.hex` | Signed firmware merged with bootloader, ready to flash |
| `ThreadBleDoorbell_qpg6200.map` | Linker map file |

### Memory footprint (typical)

| Region | Used | Available | % |
|--------|------|-----------|---|
| Flash | ~478 KB | ~1966 KB | ~24% |
| RAM | ~83 KB | ~322 KB | ~26% |

## Flashing

Please refer to the [Installation Guide](../../../sphinx/docs/QUICKSTART.rst#install-the-sdk) to set up a build environment, and to [Building and flashing the example applications](../../../sphinx/docs/QUICKSTART.rst#working-with-examples) for programming instructions.

## SDK API Reference

See the [API Manual](../../../Documents/API Manuals/API_Manuals.rst)
