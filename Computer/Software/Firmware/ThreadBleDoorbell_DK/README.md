# Thread+BLE Doorbell DK

## Introduction

Qorvo&reg; QPG6200 Thread+BLE Doorbell DK is a demo application that combines Bluetooth&reg; LE and OpenThread on a single QPG6200L chip. It is a standalone development-kit variant of the Thread+BLE Doorbell that reads the doorbell button via the **built-in PB2 push button** (GPIO 5) on the QPG6200L DK — no carrier board, jumper, or external wiring required.

Key features:

- **BLE commissioning** — a phone connects over BLE to provision Thread network credentials onto the device.
- **Thread mesh** — once commissioned, the device joins a Thread network and multicasts a structured UDP ring event to all mesh nodes on every doorbell press.
- **Gateway integration** — a Thread Border Router forwards ring events to Node-RED via UDP on port 5683.
- **Node-RED + Dashboard UI** — events are displayed and logged in real time on a browser-based dashboard.
- **Digital doorbell input** — the built-in PB2 button (GPIO 5, active low) requires no additional hardware.

> **Variants**
> | Application | Doorbell input | Carrier board needed? |
> |-------------|---------------|----------------------|
> | `ThreadBleDoorbell` | ANIO0 / GPIO 28 (analog) | Yes — IoT Carrier Board + J24 jumper |
> | **`ThreadBleDoorbell_DK`** | **PB2 / GPIO 5 (digital)** | **No** |
> | `ThreadBleDoorbell_DK_Analog` | ANIO1 / GPIO 29 (analog) | No |

---

## Hardware Setup

**Board:** QPG6200L Development Kit (QPG6200L DK) — standalone, no carrier board required.

### Doorbell button

The doorbell uses the DK's built-in **PB2** push button. No external wiring is needed.

- **Button released:** GPIO 5 pulled high through internal pull-up → logic high → not pressed
- **Button pressed:** GPIO 5 connected to GND → logic low → pressed (active low)

The GPIO driver applies a debounce filter requiring 3 consecutive matching samples (150 ms at 50 ms poll rate) before registering a state change.

---

## QPG6200 GPIO Configurations

| GPIO | Direction | Connected To | Function |
|:----:|:---------:|:------------:|:---------|
| RESETN | Input | SW1 | Hardware reset |
| GPIO 3 | Input (Pull-up) | PB1 | Short press (<2 s): restart BLE advertising / Long press (≥5 s): factory-reset Thread credentials |
| GPIO 5 | Input (Pull-up) | PB2 | Doorbell button (active low) |
| GPIO 1 | Output | WHITE_COOL LED | BLE connection status |
| GPIO 11 | Output | GREEN LED | Thread network status |
| GPIO 12 | Output | BLUE LED | Doorbell ring indicator |

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

- Pressing PB2 (GPIO 5) — button connects GPIO to GND (active low)
- Writing `0x01` to the **Ring** BLE characteristic from a connected phone
- Receiving a UDP multicast ring packet from another Thread mesh node

On a ring event the BLUE LED blinks rapidly, a BLE notification (value `0x01`) is sent to any connected phone, and a UDP multicast is sent to all nodes on the Thread mesh.

### Button summary

| Button | Action | Result |
|--------|--------|--------|
| PB1 | Short press (<2 s) | Restart BLE advertising |
| PB1 | Long press (≥5 s) | Factory-reset Thread credentials and reboot |
| PB2 | Press | Ring the doorbell |

---

## Doorbell Button Configuration

| Parameter | Value |
|-----------|-------|
| GPIO | GPIO 5 (PB2) |
| Logic | Active low (press connects GPIO to GND) |
| Pull | Internal pull-up enabled |
| Poll interval | 50 ms |
| Debounce count | 3 consecutive samples (150 ms) |

These parameters can be overridden at compile time via:

```c
-DDOORBELL_POLL_MS=<ms>
-DDOORBELL_DEBOUNCE_COUNT=<count>
```

---

## Thread UDP Payload Format

Each ring event is sent as a 4-byte UDP multicast to `ff03::1` on port `5683`.

| Byte | Value | Description |
|------|-------|-------------|
| 0 | `0x02` | Message type: doorbell event |
| 1 | `0x01` | Ring state (always 0x01 for a ring) |
| 2 | ring count high byte | Total ring count since boot (big-endian) |
| 3 | ring count low byte | Total ring count since boot (big-endian) |

Example — first doorbell press:
```
02 01 00 01
```

Example — fifth press:
```
02 01 00 05
```

---

## Gateway Integration

The device sends UDP multicast packets to `ff03::1` port `5683`. A **Thread Border Router** (e.g. Raspberry Pi with OpenThread Border Router) bridges the Thread mesh to your LAN so these packets are reachable from Node-RED.

### Border Router setup (summary)

1. Flash OpenThread Border Router firmware (or run `ot-daemon` + `otbr-agent` on a Linux host).
2. Connect the Border Router to the same Thread network (same credentials as provisioned via BLE).
3. Note the Border Router's LAN IP address — Node-RED will receive multicast traffic forwarded through it on UDP port `5683`.

---

## Node-RED + Dashboard UI

### Prerequisites

```sh
# Install Node-RED (if not already installed)
npm install -g --unsafe-perm node-red

# Install Dashboard add-on
cd ~/.node-red
npm install node-red-dashboard
```

### Flow overview

```
[UDP in :5683] → [Parse doorbell payload] → [Set msg properties] → [Dashboard gauge / text / notification]
                                                                   → [Debug]
```

### Node-RED flow JSON

Import the following flow via **Menu → Import**:

```json
[
  {
    "id": "udp_in_doorbell",
    "type": "udp in",
    "name": "Thread UDP :5683",
    "iface": "",
    "port": "5683",
    "ipv": "udp4",
    "multicast": "false",
    "group": "",
    "datatype": "buffer",
    "x": 120,
    "y": 100,
    "wires": [["parse_doorbell"]]
  },
  {
    "id": "parse_doorbell",
    "type": "function",
    "name": "Parse doorbell payload",
    "func": "const buf = msg.payload;\nif (!Buffer.isBuffer(buf) || buf.length < 4) return null;\nif (buf[0] !== 0x02) return null;\nconst ringCount = (buf[2] << 8) | buf[3];\nmsg.topic    = 'doorbell';\nmsg.ring     = buf[1] === 0x01;\nmsg.ringCount = ringCount;\nmsg.payload  = {\n    type:      'doorbell',\n    ring:      msg.ring,\n    ringCount: ringCount,\n    timestamp: new Date().toISOString()\n};\nreturn msg;",
    "x": 360,
    "y": 100,
    "wires": [["dashboard_text", "dashboard_notif", "debug_out"]]
  },
  {
    "id": "dashboard_text",
    "type": "ui_text",
    "name": "Ring Count",
    "label": "Doorbell presses",
    "format": "{{msg.ringCount}}",
    "x": 620,
    "y": 60,
    "wires": []
  },
  {
    "id": "dashboard_notif",
    "type": "ui_toast",
    "name": "Ring notification",
    "position": "top right",
    "displayTime": "3",
    "title": "DING DONG!",
    "raw": false,
    "x": 620,
    "y": 120,
    "wires": []
  },
  {
    "id": "debug_out",
    "type": "debug",
    "name": "Debug",
    "active": true,
    "x": 620,
    "y": 180,
    "wires": []
  }
]
```

### Manual parsing snippet

If you prefer to write your own function node:

```javascript
const buf = msg.payload;
if (!Buffer.isBuffer(buf) || buf.length < 4) return null;
if (buf[0] !== 0x02) return null;  // 0x02 = doorbell event type

const ringCount = (buf[2] << 8) | buf[3];
msg.payload = {
    type:      'doorbell',
    ring:      buf[1] === 0x01,
    ringCount: ringCount,
    timestamp: new Date().toISOString()
};
return msg;
```

### Dashboard widgets

After importing the flow, open `http://<node-red-host>:1880/ui` to see:

- **Text widget** — running total of doorbell presses since boot
- **Toast / notification** — pops up "DING DONG!" on every ring event
- **Debug panel** — full JSON payload for each event

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
make -f Applications/Ble/ThreadBleDoorbell_DK/Makefile.ThreadBleDoorbell_DK_qpg6200
```

Output files are written to `Work/ThreadBleDoorbell_DK_qpg6200/`:

| File | Description |
|------|-------------|
| `ThreadBleDoorbell_DK_qpg6200.elf` | ELF debug image |
| `ThreadBleDoorbell_DK_qpg6200.hex` | Signed firmware merged with bootloader, ready to flash |
| `ThreadBleDoorbell_DK_qpg6200.map` | Linker map file |

---

## Flashing

Please refer to the [Installation Guide](../../../sphinx/docs/QUICKSTART.rst#install-the-sdk) to set up a build environment, and to [Building and flashing the example applications](../../../sphinx/docs/QUICKSTART.rst#working-with-examples) for programming instructions.

## SDK API Reference

See the [API Manual](../../../Documents/API Manuals/API_Manuals.rst)
