# BLE Gateway Doorbell – QPG6200 Development Kit

A complete end-to-end smart doorbell demo using **Bluetooth Low Energy (BLE)**
as the sole wireless protocol.  The QPG6200 development board acts as a BLE
peripheral that sends doorbell ring events to a Raspberry Pi gateway, which
forwards them to an MQTT broker and a Node-RED dashboard.

---

## Table of Contents

1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [RF Protocol – BLE Only](#rf-protocol--ble-only)
4. [GATT Service Layout](#gatt-service-layout)
5. [MQTT Topic Reference](#mqtt-topic-reference)
6. [Prerequisites](#prerequisites)
7. [Repository Layout](#repository-layout)
8. [Step 1 – Build and Flash the Firmware](#step-1--build-and-flash-the-firmware)
9. [Step 2 – Deploy the Gateway Bridge](#step-2--deploy-the-gateway-bridge)
10. [Step 3 – Import the Node-RED Flow](#step-3--import-the-node-red-flow)
11. [Step 4 – Log In and Use the Dashboard](#step-4--log-in-and-use-the-dashboard)
12. [LED and Button Reference](#led-and-button-reference)
13. [How It Works End-to-End](#how-it-works-end-to-end)
14. [Troubleshooting](#troubleshooting)
15. [Testing Without Hardware](#testing-without-hardware)

---

## Overview

| Component | Role |
|---|---|
| **QPG6200 DK** (firmware) | BLE peripheral – sends doorbell ring events and a periodic heartbeat; accepts LED control commands |
| **Raspberry Pi** (gateway bridge) | BLE central – connects to the QPG6200, bridges BLE notifications to MQTT, and forwards MQTT LED commands back via BLE |
| **Node-RED** (dashboard) | Visualises doorbell events with animation and chime audio; controls the BLUE LED; shows device heartbeat and connection state |

---

## System Architecture

```
  ┌──────────────────────────────┐
  │   QPG6200 Development Kit    │
  │                              │
  │  [PB5 Button] [WHITE] [BLUE] │
  │       │            ▲         │
  │  BLE GATT Peripheral         │
  │  Name: "QPG Doorbell GW"     │
  └──────────┬───────────────────┘
             │  2.4 GHz BLE (RF)
             │
  ┌──────────▼───────────────────┐
  │   Raspberry Pi Gateway       │  192.168.10.102
  │                              │
  │  ble_mqtt_bridge.py          │  ← this repo
  │  (BLE Central ↔ MQTT)        │
  │                              │
  │  Mosquitto MQTT Broker :1883 │
  │  Node-RED             :1880  │
  └──────────┬───────────────────┘
             │  WiFi / Ethernet
             │
  ┌──────────▼───────────────────┐
  │   Browser (tablet / laptop)  │
  │   http://192.168.10.102:1880 │
  │         /dashboard           │
  │                              │
  │   Login: admin / Qorvo       │
  └──────────────────────────────┘
```

### Data flow summary

```
Button press  → BLE notify → MQTT qorvo/demo/button    → Doorbell animation + chime
Button release→ BLE notify → MQTT qorvo/demo/button    → Animation resets
Heartbeat     → BLE notify → MQTT qorvo/demo/heartbeat → Movement Detector widget
BLE connect   →            → MQTT qorvo/demo/status    → Connection indicator

Dashboard LED toggle → MQTT qorvo/demo/led → BLE write → BLUE LED on QPG6200
```

---

## RF Protocol – BLE Only

This application uses **Bluetooth Low Energy (BLE) 5.0** as the sole wireless
protocol.  No Thread, Wi-Fi, or Zigbee is required.

| Parameter | Value |
|---|---|
| Protocol | Bluetooth Low Energy (BLE) 5.0 |
| Role | Peripheral (GATT server) |
| Advertising name | `QPG Doorbell GW` |
| Advertising interval | 20 ms – 60 ms |
| Advertising duration | ~60 seconds per cycle |
| Frequency band | 2.4 GHz ISM |
| Typical indoor range | 10–30 m |
| Gateway role | Central (BLE client) via `ble_mqtt_bridge.py` |

BLE was chosen because both the QPG6200 SoC and the Raspberry Pi 4 have
integrated BLE radios, requiring no additional hardware.

---

## GATT Service Layout

### Service 1 – Battery Service (SIG standard, UUID `0x180F`)

| Attribute | Handle | UUID | Properties |
|---|---|---|---|
| Battery Service | 0x2000 | 0x180F | – |
| Battery Level | 0x2002 | 0x2A19 | Read / Notify (fixed at 100 %) |

### Service 2 – Doorbell Gateway Service (custom 128-bit UUID)

Service UUID: `C000C001-0000-1000-8000-00805F9B3400`

| Attribute | Handle | UUID (last 4 hex) | Properties | Description |
|---|---|---|---|---|
| Doorbell Service | 0x3000 | …C001 | – | Service declaration |
| Doorbell Ring | 0x3002 | …C002 | Read / Notify | `0x00` = idle, `0x01` = ringing |
| LED Control | 0x3005 | …C003 | Write | `0x00` = LED off, `0x01` = LED on |
| Heartbeat | 0x3007 | …C004 | Read / Notify | 1-byte counter (0–255, wraps) |

> **Testing with nRF Connect**
> Connect to `QPG Doorbell GW`, find the custom service, and:
> - Enable notifications on the Doorbell Ring characteristic, then press PB5.
> - Write `0x01` to LED Control to turn on the BLUE LED.
> - Watch the Heartbeat value increment every 5 seconds.

---

## MQTT Topic Reference

All topics use the `qorvo/demo/` prefix and carry JSON payloads.

### Inbound (QPG6200 → MQTT broker)

#### `qorvo/demo/button`
Published when PB5 is pressed or released.
```json
{ "state": "pressed",  "raw": 1, "timestamp": "2026-04-16T10:00:00.000Z" }
{ "state": "released", "raw": 0, "timestamp": "2026-04-16T10:00:01.200Z" }
```

#### `qorvo/demo/heartbeat`
Published every 5 seconds to confirm the board is alive.
```json
{ "count": 42, "timestamp": "2026-04-16T10:00:05.000Z" }
```
The `count` field wraps from 255 back to 0.

#### `qorvo/demo/status`
Published by the gateway bridge on BLE connection state change.
```json
{ "connected": true,  "device": "QPG Doorbell GW", "timestamp": "..." }
{ "connected": false, "device": "QPG Doorbell GW", "timestamp": "..." }
```

### Outbound (MQTT broker → QPG6200)

#### `qorvo/demo/led`
Publish to control the BLUE LED on the development board.
```json
{ "led": "on"  }
{ "led": "off" }
```
Plain text `on` / `off` is also accepted (useful for testing with `mosquitto_pub`).

---

## Prerequisites

### Hardware

| Item | Notes |
|---|---|
| QPG6200 Development Kit | Qorvo QPG6200L-based board |
| Raspberry Pi 4 | Gateway – running Raspberry Pi OS (Bookworm / Bullseye) |
| TRENDnet TEW-827DRU router | Or any router; Pi at `192.168.10.102` |
| USB programming cable | For flashing firmware |
| J-Link debug probe | Or other SWD-compatible programmer |

### Gateway software (see `IoT_Gateway_Documentation.docx.md`)

The following must already be installed and running on the Raspberry Pi:
- Mosquitto MQTT broker on port `1883`
- Node-RED on port `1880` with `@flowfuse/node-red-dashboard` (Dashboard 2.0)
- Python 3.8+ (`python3 --version`)
- Bluetooth enabled: `hciconfig` shows `hci0 UP RUNNING`

### Build host software

- Qorvo IoT SDK (this repository)
- ARM GCC Embedded (12.2.mpacbti-rel1.1-p345-matter)
- GNU Make

---

## Repository Layout

```
Applications/Ble/BleGatewayDoorbell/
├── inc/
│   ├── AppEvent.h               # Application event types (button, BLE, heartbeat)
│   ├── AppManager.h             # Application manager class interface
│   ├── AppTask.h                # FreeRTOS task and event queue interface
│   ├── BleDoorbellGW_Config.h   # GATT handle definitions and doorbell constants
│   ├── Ble_Peripheral_Config.h  # Shim header required by BleIf.c
│   └── qPinCfg.h                # GPIO pin assignments for the DK
│
├── src/
│   ├── AppManager.cpp           # Central application logic
│   ├── AppTask.cpp              # FreeRTOS task and event queue
│   └── BleDoorbellGW_Config.c   # GATT attribute database and advertising data
│
├── gateway/
│   ├── ble_mqtt_bridge.py       # Python BLE-to-MQTT bridge (runs on Pi)
│   ├── requirements.txt         # Python dependencies (bleak, paho-mqtt)
│   └── install.sh               # Automated installer / systemd service setup
│
├── nodered/
│   └── flows.json               # Node-RED flow export (import via UI)
│
├── Makefile.BleDoorbellGW_qpg6200    # SDK build system Makefile
├── BleDoorbellGW_qpg6200_postbuild.sh
└── README.md                         # This file
```

---

## Step 1 – Build and Flash the Firmware

### 1.1 Copy the generated configuration directory

The build system needs a `gen/` directory with SDK-generated headers and a
linker script.  Reuse the Peripheral BLE example's generated directory:

```bash
# From the SDK root
cp -r Applications/Ble/Peripheral/gen/Ble_qpg6200 \
      Applications/Ble/Peripheral/gen/Ble_qpg6200
```

The `Makefile.BleDoorbellGW_qpg6200` already points to
`Applications/Ble/Peripheral/gen/Ble_qpg6200` for the config headers and
linker script, so no changes are needed if that directory exists.

### 1.2 Build the firmware

```bash
# From the SDK root directory
make -f Applications/Ble/BleGatewayDoorbell/Makefile.BleDoorbellGW_qpg6200
```

A successful build prints a memory summary and places the output at:
```
Work/BleDoorbellGW_qpg6200/BleDoorbellGW_qpg6200.hex
```

### 1.3 Flash the firmware

```bash
# With J-Link Commander:
JLinkExe -device XP4002 -if SWD -speed 4000 -autoconnect 1 \
  -CommandFile flash_commands.jlink
```

Where `flash_commands.jlink` contains:
```
loadfile Work/BleDoorbellGW_qpg6200/BleDoorbellGW_qpg6200.hex
r
g
exit
```

After flashing, the WHITE_COOL LED blinks at 500 ms indicating that BLE
advertising is active and the board is waiting for the gateway to connect.

Open a serial terminal (115200 baud) to confirm the boot log:
```
==============================================
   QPG6200 BLE GATEWAY DOORBELL DEMO
==============================================
RF Protocol : Bluetooth Low Energy (BLE) 5.0
Device name : QPG Doorbell GW
...
[BLE] Advertising started – scan for 'QPG Doorbell GW'
```

---

## Step 2 – Deploy the Gateway Bridge

### 2.1 Copy files to the Raspberry Pi

```bash
scp -r Applications/Ble/BleGatewayDoorbell/gateway/ pi@192.168.10.102:~/doorbell-bridge/
```

### 2.2 Install dependencies and service

```bash
ssh pi@192.168.10.102
cd ~/doorbell-bridge
chmod +x install.sh
./install.sh
```

The installer:
1. Installs Python dependencies (`bleak`, `paho-mqtt`) via pip.
2. Creates a systemd service (`ble-doorbell-bridge`) that starts on boot.
3. Starts the service immediately.

### 2.3 Verify the bridge is running

```bash
sudo systemctl status ble-doorbell-bridge
journalctl -u ble-doorbell-bridge -f
```

Expected log once the QPG6200 is advertising:
```
[INFO] Scanning for 'QPG Doorbell GW' (timeout=10s) …
[INFO] Found device: QPG Doorbell GW [AA:BB:CC:DD:EE:FF]
[INFO] BLE connected to QPG Doorbell GW
[INFO] Subscribed to Doorbell Ring notifications
[INFO] Subscribed to Heartbeat notifications
[INFO] Bridge active:
[INFO]   Doorbell ring  → qorvo/demo/button
[INFO]   Heartbeat      → qorvo/demo/heartbeat
[INFO]   LED commands   ← qorvo/demo/led
```

### 2.4 Run the bridge manually (optional)

```bash
# Default: connects to 'QPG Doorbell GW' and MQTT on localhost
python3 ~/doorbell-bridge/ble_mqtt_bridge.py

# Specify options explicitly
python3 ~/doorbell-bridge/ble_mqtt_bridge.py \
    --device "QPG Doorbell GW" \
    --broker 192.168.10.102 \
    --port 1883 \
    --debug
```

---

## Step 3 – Import the Node-RED Flow

The `nodered/flows.json` file is the complete **QPG IoT Demo** dashboard flow.
It contains:
- A login page (credentials: **admin** / **Qorvo**)
- A Smart Door Devices page with Doorbell, Movement Detector, and BLE status widgets
- MQTT nodes already wired to the correct topics and the Pi broker

### 3.1 Import steps

1. Open the Node-RED editor: `http://192.168.10.102:1880`
2. Click the **hamburger menu** (☰) → **Import**
3. Click **select a file to import**
4. Select `Applications/Ble/BleGatewayDoorbell/nodered/flows.json`
5. Choose **Import to new tabs** when prompted
6. Click **Import**, then click the red **Deploy** button

After deploying, all MQTT nodes should show a **green dot** (connected to
Mosquitto).  If they show red, check that Mosquitto is running:
```bash
sudo systemctl status mosquitto
```

### 3.2 Dashboard structure

Navigate to `http://192.168.10.102:1880/dashboard`:

| Page | Path | Purpose |
|---|---|---|
| Login | `/dashboard/page3` | Enter credentials before accessing the main page |
| Smart Door Devices | `/dashboard/page1` | Live doorbell and device status |

**Login credentials:** username `admin`, password `Qorvo`

### 3.3 Widget groups on the Smart Door Devices page

| Group | What it shows | Driven by |
|---|---|---|
| **Doorbell** | Animated doorbell with chime audio on ring event | `qorvo/demo/button` (`state: pressed`) |
| **Movement Detector** | Live status indicator; shows "Motion DETECTED" when heartbeat arrives | `qorvo/demo/heartbeat` |
| **Speaker/Mic** | BLE connection indicator (📡 Connected / 🔌 Offline) | `qorvo/demo/status` |
| **QPG6200 Device** | Heartbeat counter and connection text | `qorvo/demo/heartbeat` + `qorvo/demo/status` |
| **Lighting System** | Toggle switch to control the BLUE LED on the board | Publishes to `qorvo/demo/led` |

---

## Step 4 – Log In and Use the Dashboard

1. Navigate to `http://192.168.10.102:1880/dashboard` on any device connected
   to the TRENDnet `TRENDnet827_2.4GHz_QMGR` WiFi network.
2. The **Login** page appears first.  Enter `admin` / `Qorvo` and click LOGIN.
3. You are redirected to the **Smart Door Devices** page.

### Ring the doorbell

1. Press the **PB5** button on the QPG6200 development kit.
2. The board sends a BLE notification (Doorbell Ring = `0x01`).
3. The gateway bridge publishes `{"state":"pressed"}` to `qorvo/demo/button`.
4. The **Doorbell** widget animates and plays the chime audio on the dashboard.
5. Release PB5 — the bridge publishes `{"state":"released"}` and the animation resets.

### Monitor the heartbeat

Every 5 seconds:
- The **Movement Detector** indicator pulses with "Motion DETECTED".
- The **QPG6200 Device** counter increments (0–255, wrapping).

This confirms the board is powered, the BLE link is active, and the bridge is
forwarding data to MQTT.

### Control the BLUE LED

1. Toggle the **Light Switch** in the Lighting System group.
2. Node-RED publishes `{"led":"on"}` or `{"led":"off"}` to `qorvo/demo/led`.
3. The gateway bridge writes `0x01` or `0x00` to the LED Control characteristic.
4. The BLUE LED on the QPG6200 board turns on or off.

### BLE connection status

The **Speaker/Mic** panel updates immediately when the BLE link state changes:
- 📡 **QPG6200 Connected** – bridge is active and forwarding data.
- 🔌 **QPG6200 Offline** – bridge lost the BLE link (board powered off or
  out of range).

---

## LED and Button Reference

| LED / Button | GPIO | Function |
|---|---|---|
| WHITE_COOL LED | GPIO (see qPinCfg.h) | **BLE status** – blinks 500 ms while advertising, solid ON when connected |
| BLUE LED | GPIO (see qPinCfg.h) | **App LED** – remote-controlled by Node-RED dashboard via LED Control characteristic |
| PB5 button | GPIO (see qPinCfg.h) | **Doorbell trigger** – short press sends ring event; hold ≥ 5 s restarts advertising |

---

## How It Works End-to-End

```
1.  Board boots → starts BLE advertising as "QPG Doorbell GW"
    WHITE_COOL LED blinks at 500 ms

2.  ble_mqtt_bridge.py scans → finds "QPG Doorbell GW" → connects
    WHITE_COOL LED goes solid ON
    Bridge publishes {"connected": true} to qorvo/demo/status
    Node-RED Speaker/Mic panel shows 📡 Connected

3.  User presses PB5:
    Board → BLE Notify (Doorbell Ring = 0x01)
    Bridge → MQTT qorvo/demo/button {"state":"pressed"}
    Node-RED → doorbell animation plays + chime audio fires

4.  User releases PB5:
    Board → BLE Notify (Doorbell Ring = 0x00)
    Bridge → MQTT qorvo/demo/button {"state":"released"}
    Node-RED → animation resets to idle

5.  Every 5 seconds:
    Board → BLE Notify (Heartbeat counter++)
    Bridge → MQTT qorvo/demo/heartbeat {"count": N}
    Node-RED → Movement Detector shows "Motion DETECTED" + counter updates

6.  Dashboard Light Switch toggled ON:
    Node-RED → MQTT qorvo/demo/led {"led":"on"}
    Bridge → BLE Write (LED Control = 0x01)
    Board → BLUE LED turns ON

7.  Board moves out of range or is powered off:
    BLE connection drops
    Bridge → MQTT qorvo/demo/status {"connected": false}
    Node-RED → Speaker/Mic panel shows 🔌 Offline
    Bridge waits RECONNECT_DELAY_SEC (5 s) then retries scan
```

---

## Troubleshooting

### Board not advertising

- Check that firmware flashed successfully (no errors from the flash tool).
- Open a serial terminal (115200 baud) and verify the boot log shows
  `[BLE] Advertising started`.
- Hold PB5 for **5+ seconds** to restart BLE advertising manually.

### Gateway bridge cannot find the device

- Verify Bluetooth is enabled: `hciconfig hci0` should show `UP RUNNING`.
- If not: `sudo hciconfig hci0 reset`
- Confirm the WHITE_COOL LED is blinking (advertising mode).
- Check the device name: the bridge searches for `QPG Doorbell GW`
  (case-insensitive substring match).

### MQTT messages not arriving in Node-RED

- Check Mosquitto: `sudo systemctl status mosquitto`
- Test manually: `mosquitto_sub -h localhost -t "qorvo/demo/#" -v`
- Check bridge log: `journalctl -u ble-doorbell-bridge -f`

### Dashboard not loading

- Confirm Node-RED is running: `sudo systemctl status nodered`
- Try the editor first: `http://192.168.10.102:1880`
- If the editor loads but the dashboard does not, check that
  `@flowfuse/node-red-dashboard` is installed in Manage Palette.

### MQTT nodes show red dot (disconnected) in Node-RED

- Confirm the MQTT broker config node shows host `192.168.10.102`, port `1883`.
- Restart Mosquitto: `sudo systemctl restart mosquitto`
- Re-deploy the flow.

### LED switch has no effect

- Confirm the bridge is connected (BLE status = Connected on dashboard).
- Subscribe to the LED topic and toggle the switch:
  ```bash
  mosquitto_sub -h localhost -t "qorvo/demo/led" -v
  ```
  You should see `{"led":"on"}` or `{"led":"off"}` arrive.
- Check the bridge log for `BLE write: LED Control → ON/OFF`.

---

## Testing Without Hardware

You can simulate all device events using `mosquitto_pub` on the Pi.

```bash
# Simulate a doorbell press
mosquitto_pub -h localhost -t qorvo/demo/button \
    -m '{"state":"pressed","raw":1,"timestamp":"2026-04-16T10:00:00Z"}'

# Simulate doorbell release (3 seconds later)
mosquitto_pub -h localhost -t qorvo/demo/button \
    -m '{"state":"released","raw":0,"timestamp":"2026-04-16T10:00:03Z"}'

# Simulate a heartbeat tick
mosquitto_pub -h localhost -t qorvo/demo/heartbeat \
    -m '{"count":42,"timestamp":"2026-04-16T10:00:05Z"}'

# Simulate device connected
mosquitto_pub -h localhost -t qorvo/demo/status \
    -m '{"connected":true,"device":"QPG Doorbell GW","timestamp":"2026-04-16T10:00:00Z"}'

# Control the LED from the command line (bypassing the dashboard)
mosquitto_pub -h localhost -t qorvo/demo/led -m '{"led":"on"}'
mosquitto_pub -h localhost -t qorvo/demo/led -m '{"led":"off"}'
```

All expected dashboard effects:
- Button press → Doorbell animates and chime plays
- Heartbeat → Movement Detector pulses
- Status connected → Speaker/Mic shows 📡 Connected
- LED on/off → Node-RED forwards to the board's LED Control characteristic
