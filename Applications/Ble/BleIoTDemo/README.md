# BLE IoT Demo – QPG6200 Development Kit

A simple end-to-end demonstration showing the QPG6200 development board
communicating via **Bluetooth Low Energy (BLE)** with a Raspberry Pi IoT
gateway, an MQTT broker, and a Node-RED dashboard.

---

## Table of Contents

1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [RF Protocol – Bluetooth Low Energy](#rf-protocol--bluetooth-low-energy)
4. [GATT Service Layout](#gatt-service-layout)
5. [MQTT Topic Reference](#mqtt-topic-reference)
6. [Prerequisites](#prerequisites)
7. [Repository Layout](#repository-layout)
8. [Step 1 – Build and Flash the Firmware](#step-1--build-and-flash-the-firmware)
9. [Step 2 – Deploy the Gateway Bridge](#step-2--deploy-the-gateway-bridge)
10. [Step 3 – Import the Node-RED Flow](#step-3--import-the-node-red-flow)
    - [Dashboard pages](#32-dashboard-pages)
    - [MQTT-to-UI data mapping](#33-mqtt-to-ui-data-mapping)
11. [Using the Demo](#using-the-demo)
12. [LED and Button Reference](#led-and-button-reference)
13. [Troubleshooting](#troubleshooting)
14. [Architecture Notes](#architecture-notes)

---

## Overview

This demo ties together three components:

| Component | Role |
|---|---|
| **QPG6200 DK** (firmware) | BLE peripheral – sends button events and a periodic heartbeat; accepts LED control commands |
| **Raspberry Pi** (gateway bridge) | BLE central – connects to the QPG6200, forwards notifications to MQTT and writes LED commands back |
| **Node-RED** (dashboard) | Visualises button events, heartbeat, and connection status; provides a switch to control the LED |

The demo is intentionally minimal so that every link in the chain is easy to
inspect and extend.

---

## System Architecture

```
  ┌──────────────────────────────┐
  │   QPG6200 Development Kit    │
  │                              │
  │  [PB5 Button]  [LEDs]        │
  │       │            ▲         │
  │  BLE GATT Peripheral         │
  │  Device name: "QPG IoT Demo" │
  └──────────┬───────────────────┘
             │  2.4 GHz BLE (RF)
             │
  ┌──────────▼───────────────────┐
  │   Raspberry Pi Gateway       │  192.168.10.102
  │                              │
  │  ble_mqtt_bridge.py          │
  │  (BLE Central ↔ MQTT client) │
  │                              │
  │  Mosquitto MQTT Broker :1883 │
  │  Node-RED             :1880  │
  └──────────┬───────────────────┘
             │  WiFi / Ethernet (TCP/IP)
             │
  ┌──────────▼───────────────────┐
  │   Browser (any device)       │
  │   http://192.168.10.102:1880 │
  │         /dashboard           │
  └──────────────────────────────┘
```

### Data flow summary

```
Button press  → BLE notify → MQTT qorvo/demo/button    → Node-RED widget
Heartbeat     → BLE notify → MQTT qorvo/demo/heartbeat → Node-RED widget
BLE connect   →            → MQTT qorvo/demo/status    → Node-RED widget

Dashboard LED toggle → MQTT qorvo/demo/led → BLE write → LED on QPG6200
```

---

## RF Protocol – Bluetooth Low Energy

The QPG6200 uses **BLE 5.0** as the radio frequency (RF) protocol for all
wireless communication in this demo.

| Parameter | Value |
|---|---|
| Protocol | Bluetooth Low Energy (BLE) 5.0 |
| Role | Peripheral (GATT server) |
| Advertising name | `QPG IoT Demo` |
| Advertising interval | 20 ms – 60 ms |
| Advertising duration | ~60 seconds per cycle |
| Frequency band | 2.4 GHz ISM |
| Operating range | ~10–30 m (indoor, typical) |
| Connection role | Gateway bridge acts as Central |

BLE was selected because:
- It is the primary radio in the QPG6200 SoC.
- The Raspberry Pi 4 has a built-in BLE adapter (no additional hardware).
- BLE GATT provides a clean, standards-based interface for reading and
  writing device state.
- Power consumption is low, making the technique suitable for battery-powered
  IoT deployments.

---

## GATT Service Layout

### Service 1 – Battery Service (SIG standard, UUID `0x180F`)

| Attribute | Handle | UUID | Properties | Description |
|---|---|---|---|---|
| Battery Service | 0x2000 | 0x180F | – | Service declaration |
| Battery Level | 0x2002 | 0x2A19 | Read / Notify | Fixed at 100 % |

### Service 2 – IoT Demo Service (custom 128-bit UUID)

Service UUID: `A1B2C3D4-E5F6-7890-ABCD-EF0123456789`

| Attribute | Handle | UUID (last 4 hex digits) | Properties | Description |
|---|---|---|---|---|
| IoT Demo Service | 0x3000 | …6789 | – | Service declaration |
| Button State | 0x3002 | …6790 | Read / Notify | `0x00` = released, `0x01` = pressed |
| LED Control | 0x3005 | …6791 | Write | `0x00` = LED off, `0x01` = LED on |
| Heartbeat | 0x3007 | …6792 | Read / Notify | 1-byte counter, increments every 5 s |

> **Testing with nRF Connect**
> You can verify the GATT database using the nRF Connect app on a phone.
> Connect to `QPG IoT Demo`, navigate to the custom service, and:
> - Write `0x01` to the LED Control characteristic to turn the LED on.
> - Subscribe to Button State notifications and press PB5.

---

## MQTT Topic Reference

All topics use the `qorvo/demo/` prefix and carry JSON payloads.

### Inbound topics (QPG6200 → broker)

#### `qorvo/demo/button`

Published whenever the PB5 button is pressed or released.

```json
{ "state": "pressed",  "raw": 1, "timestamp": "2026-03-19T10:00:00.000Z" }
{ "state": "released", "raw": 0, "timestamp": "2026-03-19T10:00:01.200Z" }
```

#### `qorvo/demo/heartbeat`

Published every 5 seconds to confirm the board is alive.

```json
{ "count": 42, "timestamp": "2026-03-19T10:00:05.000Z" }
```

The `count` field is a 1-byte value (0–255) that wraps back to 0 after 255.

#### `qorvo/demo/status`

Published by the gateway bridge when the BLE connection state changes.

```json
{ "connected": true,  "device": "QPG IoT Demo", "timestamp": "..." }
{ "connected": false, "device": "QPG IoT Demo", "timestamp": "..." }
```

### Outbound topic (broker → QPG6200)

#### `qorvo/demo/led`

Publish to this topic to control the BLUE LED on the development board.

```json
{ "led": "on"  }
{ "led": "off" }
```

Plain text `on` / `off` is also accepted for easy testing with `mosquitto_pub`.

---

## Prerequisites

### Hardware

| Item | Notes |
|---|---|
| QPG6200 Development Kit | Qorvo QPG6200L-based board |
| Raspberry Pi 4 | Gateway – running Raspberry Pi OS |
| TRENDnet TEW-827DRU router | Or any router; Pi at 192.168.10.102 |
| USB programming cable | For flashing firmware |
| J-Link debug probe | Or other SWD programmer compatible with the SDK |

### Gateway software (already installed per IoT_Gateway_Documentation)

- Mosquitto MQTT broker on port 1883
- Node-RED on port 1880 with `@flowfuse/node-red-dashboard` (Dashboard 2.0)
- Python 3.8+ (`python3 --version`)
- Bluetooth enabled: `hciconfig` shows `hci0`

### Build host software

- Qorvo IoT SDK (this repository)
- ARM GCC Embedded (12.2.mpacbti-rel1.1-p345-matter)
- GNU Make

---

## Repository Layout

```
Applications/Ble/BleIoTDemo/
├── inc/
│   ├── AppEvent.h              # Application event type definitions
│   ├── AppManager.h            # Application manager interface
│   ├── AppTask.h               # FreeRTOS task interface
│   ├── BleIoTDemo_Config.h     # GATT handle and service definitions
│   ├── Ble_Peripheral_Config.h # Shim header required by BleIf.c
│   └── qPinCfg.h               # GPIO pin assignments
│
├── src/
│   ├── AppManager.cpp          # Central application logic
│   ├── AppTask.cpp             # FreeRTOS task and event queue
│   └── BleIoTDemo_Config.c     # GATT attribute database and advertising data
│
├── gateway/
│   ├── ble_mqtt_bridge.py      # Python BLE-to-MQTT bridge (runs on Pi)
│   ├── requirements.txt        # Python dependencies (bleak, paho-mqtt)
│   └── install.sh              # Automated installer / systemd service setup
│
├── nodered/
│   └── flows.json              # Node-RED flow export (import via UI)
│
├── Makefile.BleIoTDemo_qpg6200 # SDK build system Makefile
├── BleIoTDemo_qpg6200_postbuild.sh
└── README.md                   # This file
```

---

## Step 1 – Build and Flash the Firmware

### 1.1 Copy the generated config directory

The build system requires a `gen/` directory containing SDK-generated
configuration headers and a linker script. For BleIoTDemo the hardware
configuration is identical to the Peripheral BLE example, so you can
reuse its generated directory:

```bash
cp -r Applications/Ble/Peripheral/gen/Ble_qpg6200 \
      Applications/Ble/BleIoTDemo/gen/BleIoTDemo_qpg6200
```

Then update the `CONFIG_HEADER`, `INTERNALS_HEADER`, and `PREINCLUDE_HEADER`
variables in `Makefile.BleIoTDemo_qpg6200` to point to the copied directory
if you have renamed it, or leave them pointing to `Peripheral/gen/Ble_qpg6200`
as-is (both approaches work because the hardware config is identical).

### 1.2 Build

```bash
# From the SDK root directory
make -f Applications/Ble/BleIoTDemo/Makefile.BleIoTDemo_qpg6200
```

A successful build prints a memory summary similar to:

```
+----------------+------------------------------+
|                | BleIoTDemo_qpg6200.map       |
+----------------+------------------------------+
| Flash          |  258534/2011136 -    12.86 % |
| Flash+NVM+OTA  |  274918/2011136 -    13.67 % |
| Ram            |   49042/ 329728 -    14.87 % |
| Heap           |  280174/ 329728 -    84.97 % |
+----------------+------------------------------+
```

The output firmware is placed at:

```
Work/BleIoTDemo_qpg6200/BleIoTDemo_qpg6200.hex
```

### 1.3 Flash

Use J-Link or the Qorvo flash tool:

```bash
# With J-Link Commander (example):
JLinkExe -device XP4002 -if SWD -speed 4000 -autoconnect 1 \
  -CommandFile flash_commands.jlink
```

Where `flash_commands.jlink` contains:
```
loadfile Work/BleIoTDemo_qpg6200/BleIoTDemo_qpg6200.hex
r
g
exit
```

After flashing, the WHITE_COOL LED will start blinking at 500 ms,
indicating that BLE advertising has begun.

---

## Step 2 – Deploy the Gateway Bridge

### 2.1 Copy files to the Raspberry Pi

```bash
scp -r Applications/Ble/BleIoTDemo/gateway/ pi@192.168.10.102:~/ble-demo/
```

### 2.2 Install dependencies and service

```bash
ssh pi@192.168.10.102
cd ~/ble-demo
chmod +x install.sh
./install.sh
```

The installer:
1. Installs Python dependencies (`bleak`, `paho-mqtt`) via pip.
2. Creates a systemd service (`ble-mqtt-bridge`) that starts automatically on boot.
3. Starts the service immediately.

### 2.3 Verify the bridge is running

```bash
# Check service status
sudo systemctl status ble-mqtt-bridge

# Follow live log output
journalctl -u ble-mqtt-bridge -f
```

Expected log output once the QPG6200 is advertising:

```
[INFO] Scanning for 'QPG IoT Demo' (timeout=10s) …
[INFO] Found device: QPG IoT Demo [AA:BB:CC:DD:EE:FF]
[INFO] BLE connected to QPG IoT Demo
[INFO] Subscribed to Button State notifications
[INFO] Subscribed to Heartbeat notifications
[INFO] Bridge active – forwarding BLE events to MQTT and vice-versa
```

### 2.4 Test MQTT manually (optional)

On the Pi, verify messages are arriving:

```bash
# Subscribe to all demo topics
mosquitto_sub -h localhost -t "qorvo/demo/#" -v

# Send a manual LED command
mosquitto_pub -h localhost -t "qorvo/demo/led" -m '{"led":"on"}'
```

---

## Step 3 – Import the Node-RED Flow

The `nodered/flows.json` file is a **fully integrated merge** of the production
Smart Door Devices dashboard (`Files/NodeRedUI.json`) with the BleIoTDemo MQTT
data.  Importing it gives you the complete multi-page UI with real QPG6200
hardware wired in — no manual node editing required.

### 3.1 Import the flow

1. Open the Node-RED editor: `http://192.168.10.102:1880`
2. Click the **hamburger menu** (☰) → **Import**
3. Click **select a file to import**
4. Select `Applications/Ble/BleIoTDemo/nodered/flows.json`
5. Choose **Import to new tabs** when prompted
6. Click **Import** → **Deploy** (red button, top right)

### 3.2 Dashboard pages

The dashboard has two pages accessible at `http://192.168.10.102:1880/dashboard`:

#### Login page (`/dashboard/page3`)

| Field | Value |
|---|---|
| Username | `admin` |
| Password | `1234` |
| Session timeout | 5 minutes (auto-logout) |

The login page uses the UNCC + Qorvo branded layout with a green animated
background.  Enter the credentials above to proceed to the main dashboard.

#### Smart Door Devices page (`/dashboard/page1`)

This page contains four widget groups.  Three existing groups have been
extended with live QPG6200 BLE data, and one new group was added:

| Group | Original UI purpose | QPG6200 integration |
|---|---|---|
| **Doorbell** | Animated doorbell widget with audio playback | QPG6200 button press (PB5) triggers the doorbell animation and DING audio |
| **Lighting System** | Colour/brightness controls for smart lights | Light switch also publishes `qorvo/demo/led` to control the BLUE LED on the QPG6200 |
| **Movement Detector** | PIR-style motion indicator | Repurposed as **heartbeat monitor** – updates every 5 s; shows "Motion DETECTED" while board is alive |
| **QPG6200 Device** *(new)* | – | BLE connection status indicator; heartbeat counter; connection timestamp |

The **Speaker / Mic** panel (present in the original UI) is repurposed as the
QPG6200 BLE connection indicator:  📡 = Connected, 🔌 = Offline.

### 3.3 MQTT-to-UI data mapping

```
qorvo/demo/button   ──► func_btn_door ──► Doorbell animation + audio + LED flash (3 s)
qorvo/demo/heartbeat──► func_hb_disp  ──► Movement Detector indicator + heartbeat counter
qorvo/demo/status   ──► func_stat_disp──► Speaker panel (connection icon + text)
                                      ──► QPG6200 Device group (connection text)

Dashboard Light Switch ──► func_light_led ──► qorvo/demo/led  ──► QPG6200 BLUE LED
```

### 3.4 Verify Node-RED connectivity

In the editor, all MQTT nodes should show a **green dot** (connected) after
Deploy.  If they show red/disconnected:

- Confirm Mosquitto is running: `sudo systemctl status mosquitto`
- Check the MQTT broker config node (`ble_demo_broker`) shows host
  `192.168.10.102`, port `1883`.

---

## Using the Demo

Once all three components are running:

### Log in to the dashboard

Navigate to `http://192.168.10.102:1880/dashboard`, enter `admin` / `1234`,
and you will be taken to the **Smart Door Devices** page.

### Press the button (PB5)

1. The QPG6200 sends a BLE notification.
2. The gateway bridge publishes `{"state":"pressed", ...}` to `qorvo/demo/button`.
3. The Node-RED **Doorbell** group plays the DING animation and audio.
4. The QPG6200's BLUE LED flashes for 3 seconds (automatic, wired in the flow).

### Release the button

A second notification with `state: released` is published; the doorbell
animation stops.

### Control the LED from the dashboard

1. Toggle the **Light Switch** in the Lighting System group.
2. Node-RED publishes `{"led":"on"}` or `{"led":"off"}` to `qorvo/demo/led`.
3. The gateway bridge writes to the LED Control GATT characteristic.
4. The BLUE LED on the QPG6200 turns on or off.

### Monitor the heartbeat

Every 5 seconds:
- The **Movement Detector** indicator pulses ("Motion DETECTED").
- The **QPG6200 Device** group shows the current heartbeat counter (0–255,
  wrapping).

### BLE connection status

The **Speaker / Mic** panel updates immediately when the BLE connection
state changes:
- 📡 **QPG6200 Connected** – bridge is connected and forwarding data.
- 🔌 **QPG6200 Offline** – bridge lost the BLE link (board powered off or
  out of range).

---

## LED and Button Reference

| LED / Button | GPIO | Function |
|---|---|---|
| WHITE_COOL LED | GPIO 1 | **BLE status** – blinks while advertising, solid ON when connected |
| BLUE LED | GPIO 12 | **Application LED** – controlled via LED Control characteristic / dashboard |
| PB5 button | GPIO | **Multi-function button** – short press sends BLE notification; hold ≥5 s restarts advertising |

---

## Troubleshooting

### QPG6200 not advertising

- Check that the firmware flashed successfully (no errors from the flash tool).
- Open a serial terminal (115200 baud) and verify the boot log shows
  `BLE advertising started`.
- Press and hold PB5 for 5+ seconds to restart BLE advertising.

### Gateway bridge cannot find the device

- Verify Bluetooth is enabled on the Pi: `hciconfig hci0` should show `UP RUNNING`.
- Reset the Bluetooth adapter: `sudo hciconfig hci0 reset`
- Check the device name matches: the bridge looks for `QPG IoT Demo` (case-insensitive substring).
- Confirm the QPG6200 WHITE_COOL LED is blinking (advertising mode).

### MQTT messages not appearing

- Confirm Mosquitto is running: `sudo systemctl status mosquitto`
- Test with `mosquitto_sub -h localhost -t "qorvo/demo/#" -v`
- Check the bridge log: `journalctl -u ble-mqtt-bridge -f`

### Node-RED dashboard not updating

- Verify the MQTT broker address in the `mqtt-broker` config node matches
  your Pi's IP (default: `192.168.10.102`).
- Re-deploy the flow after any changes.
- Check the Node-RED debug panel for errors.

### LED switch in dashboard has no effect

- Confirm the bridge is connected (BLE connection state = CONNECTED in dashboard).
- Subscribe to `qorvo/demo/led` with `mosquitto_sub` and toggle the switch –
  you should see the JSON message arrive.
- Check the bridge log for `BLE write: LED Control → ON/OFF`.

---

## Architecture Notes

### Why BLE?

Bluetooth Low Energy was chosen as the RF protocol because:

1. **Built-in hardware** – The QPG6200 SoC and Raspberry Pi 4 both have
   integrated BLE radios; no additional hardware is required.
2. **Standardised interface** – GATT provides a well-defined, interoperable
   way to model device attributes as characteristics.
3. **Low power** – BLE advertising consumes very little energy, making this
   pattern suitable for battery-powered sensor nodes.
4. **Range** – 10–30 m indoor range is appropriate for most room/office
   deployments without infrastructure (no Thread border router needed).

### Gateway bridge design

The bridge (`ble_mqtt_bridge.py`) uses:

- **bleak** – A modern, async BLE library for Python. It uses the OS BLE
  stack (BlueZ on Linux) and is compatible with Raspberry Pi OS.
- **paho-mqtt** – The Eclipse Paho MQTT client, which runs on a separate
  thread and communicates with the asyncio event loop via
  `loop.call_soon_threadsafe()`.
- **asyncio** – The bridge's BLE scanning and connection management run in
  an async event loop, allowing the main thread to remain responsive while
  waiting for BLE events.
- **systemd** – The install script creates a systemd service for automatic
  startup on boot and automatic restart on failure.

### Extending the demo

| Goal | What to change |
|---|---|
| Add a temperature sensor | Add a characteristic to `BleIoTDemo_Config.c`, read the sensor in `AppManager.cpp`, add a topic and widget to the Node-RED flow |
| Send JSON from the board | Change the BLE payload format in `AppManager.cpp` and update the Python parser in `ble_mqtt_bridge.py` |
| Use Thread instead of BLE | See the `ThreadBleDoorbell` application for the Thread + UDP multicast pattern |
| Add authentication | Enable BLE pairing (edit `BleIf.c` security settings) and MQTT username/password (edit Mosquitto config and bridge script) |
