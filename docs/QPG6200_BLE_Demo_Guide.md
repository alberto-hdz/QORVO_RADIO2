# QPG6200 BLE IoT Demo
## System Guide & Demo Procedures

**UNC Charlotte | William States Lee College of Engineering**

| Field | Value |
|---|---|
| Project | IoT Mesh Network Senior Design |
| Hardware | QPG6200LDK-01 Development Kit |
| Gateway | Raspberry Pi 4 (192.168.10.102) |
| Network | TRENDnet TEW-827DRU Private LAN |
| Dashboard URL | http://192.168.10.102:1880/dashboard/page1 |
| Document Version | 1.0 — March 2026 |

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [Hardware Components](#2-hardware-components)
3. [Software Stack](#3-software-stack)
4. [Startup Procedure](#4-startup-procedure)
5. [Shutdown Procedure](#5-shutdown-procedure)
6. [Demo Script](#6-demo-script)
7. [Troubleshooting](#7-troubleshooting)
8. [Quick Reference](#8-quick-reference)

---

## 1. System Overview

The QPG6200 BLE IoT Demo is an end-to-end IoT demonstration system that shows real-time data flowing from a wireless embedded device all the way to a web-based dashboard. The QPG6200L multi-protocol SoC on the development kit communicates over Bluetooth Low Energy (BLE) to a Raspberry Pi 4 gateway, which forwards the data via MQTT to a Node-RED dashboard.

### 1.1 Architecture

| Layer | Component / Description |
|---|---|
| Embedded Device | Qorvo QPG6200LDK-01 dev kit running BleIoTDemo firmware |
| Wireless Link | Bluetooth Low Energy (BLE) — ConcurrentConnect technology |
| Gateway | Raspberry Pi 4 running ble_mqtt_bridge.py (Python) |
| Message Broker | Eclipse Mosquitto MQTT broker on the Pi (port 1883) |
| Flow Engine | Node-RED v4.1.7 on the Pi (port 1880) |
| Dashboard | @flowfuse/node-red-dashboard v1.30.2 |
| Network | TRENDnet TEW-827DRU private LAN (192.168.10.x subnet) |
| Client Device | Any browser on the LAN — tablet, laptop, or phone |

### 1.2 Data Flow

1. QPG6200 dev kit generates events (button press, heartbeat) and sends them as BLE notifications.
2. `ble_mqtt_bridge.py` on the Pi receives the BLE notifications and publishes them to Mosquitto MQTT.
3. Node-RED subscribes to the MQTT topics and processes the messages.
4. The Node-RED dashboard widgets update in real time in any connected browser.
5. When a user toggles the Light Switch on the dashboard, the command flows back: Node-RED publishes to MQTT, the bridge reads it and sends a BLE write command to the dev kit, and the BLUE LED on the board responds.

### 1.3 MQTT Topic Map

| MQTT Topic | Description / Payload |
|---|---|
| `qorvo/demo/button` | Button press/release events. JSON: `{ state: 'pressed' \| 'released' }` |
| `qorvo/demo/heartbeat` | Alive counter every 5 seconds. JSON: `{ count: N }` |
| `qorvo/demo/status` | BLE connection status. JSON: `{ connected: true/false, device: 'QPG IoT Demo' }` |
| `qorvo/demo/led` | LED control command (subscribed on device). JSON: `{ led: 'on' \| 'off' }` |

---

## 2. Hardware Components

| Component | Details |
|---|---|
| QPG6200LDK-01 | Qorvo IoT Development Kit. ARM Cortex-M4 at up to 192 MHz, 2 MB NVM, 336 KB RAM, BLE 5.4 + IEEE 802.15.4, QFN32 package. |
| Raspberry Pi 4 | Gateway computer. Runs Raspberry Pi OS (64-bit). Connected to TRENDnet router. IP: 192.168.10.102. |
| TRENDnet TEW-827DRU | Private WiFi/Ethernet router. Admin at 192.168.10.1. Creates isolated LAN for demo — avoids university WiFi client isolation issues. |
| Dell Latitude 7280 | Ubuntu Linux laptop used to SSH into the Pi and manage the system. Username: qorvo-radio. IP: 192.168.10.101. |
| nRF52840 USB Dongle | Thread RCP for OTBR (used in the Thread mesh portion of the project, not active in this BLE demo). |
| J-Link Debugger | SEGGER J-Link used for flashing QPG6200 firmware via 10-pin SWD. Built into the dev kit board. |

---

## 3. Software Stack

### 3.1 Dev Kit Firmware — BleIoTDemo

The firmware running on the QPG6200 development kit is the BleIoTDemo application, built using the qpg6200-iot-sdk dev container on Windows. Key firmware behaviours:

- On boot, the board initialises but does **NOT** start BLE advertising automatically.
- A 6-second button hold triggers BLE advertising — the board then scans for a BLE central device (the Pi bridge).
- Once connected, a heartbeat notification is sent every 5 seconds with an incrementing counter.
- Physical button presses generate BLE notifications with state `pressed` or `released`.
- The board subscribes to LED commands via BLE and toggles the BLUE LED accordingly.

**Build command (inside SDK dev container on Windows):**
```
make -f Applications/Ble/BleIoTDemo/Makefile.BleIoTDemo_qpg6200 all
```

Output hex file: `Work/BleIoTDemo_qpg6200/BleIoTDemo_qpg6200.hex`

### 3.2 Gateway Bridge — ble_mqtt_bridge.py

A Python 3 script running on the Raspberry Pi as a systemd service. It uses the `bleak` library for BLE and `paho-mqtt` for MQTT. It:

- Scans for a BLE device named `QPG IoT Demo` every 10 seconds.
- When found, connects and subscribes to all BLE notification characteristics.
- Translates BLE notifications into MQTT publishes on the `qorvo/demo/*` topics.
- Subscribes to `qorvo/demo/led` and sends BLE write commands to control the dev kit LED.
- Automatically reconnects if the BLE connection drops.

**Service management commands:**
```bash
sudo systemctl status ble-mqtt-bridge      # check status
sudo systemctl restart ble-mqtt-bridge     # restart
journalctl -u ble-mqtt-bridge -f           # live logs
```

### 3.3 Node-RED Dashboard

Node-RED runs on the Pi and hosts the flow and dashboard. The dashboard page **Smart Door Devices** (path `/page1`) contains the following widgets:

| Widget | Function |
|---|---|
| Doorbell | Ring button + animation. Physical PB5 press triggers the same animation. |
| QPG6200 Device | Shows BLE connection status (Connected/Offline) and live heartbeat counter. |
| Movement Detector | Status indicator — turns GREEN when heartbeats arrive, showing device is alive. |
| Speaker/Mic | Shows QPG6200 Connected (📡) or QPG6200 Offline (🔌) based on BLE status. |
| Lighting System | Light Switch toggle controls the BLUE LED on the dev kit via MQTT. |

---

## 4. Startup Procedure

> ⚠️ **WARNING:** Always start the router before the Pi. Always start the Pi before the dev kit. Follow the order below exactly.

### Step 1 — Power on the TRENDnet Router

Plug the router's power cable into a wall outlet. Wait approximately **60 seconds** for it to fully boot. The front LEDs will stabilise when it is ready.

### Step 2 — Connect your laptop to the router

Connect your Linux laptop to the TRENDnet network via Ethernet cable or WiFi. Verify connectivity:
```bash
ping 192.168.10.1
```
You should see replies from the router. Press Ctrl+C to stop.

### Step 3 — Power on the Raspberry Pi

Plug the Pi's USB-C power cable in. Wait approximately **45 seconds** for it to fully boot. All services (Mosquitto, Node-RED, ble-mqtt-bridge) start automatically — no manual action needed.

### Step 4 — Confirm the Pi is reachable
```bash
nmap -sn 192.168.10.0/24
```
You should see `raspberrypi.lan (192.168.10.102)` in the output.

### Step 5 — Verify all services are running
```bash
ssh pi@192.168.10.102
sudo systemctl status mosquitto node-red ble-mqtt-bridge
```
All three should show `Active: active (running)` in green. If any show `failed`:
```bash
sudo systemctl restart <service-name>
```

### Step 6 — Power on the dev kit

Plug the USB-C cable into the QPG6200 development kit board. The board will boot — LEDs will flicker briefly. Wait 5 seconds.

### Step 7 — Start BLE advertising on the dev kit

> ⚠️ **WARNING:** BLE advertising does NOT start automatically on boot. You must trigger it manually every time.

1. Locate the user button (PB5) on the dev kit.
2. Hold it down for **6 full seconds**.
3. Release it.
4. The board will print `BLE advertising — waiting for gateway connection` in the serial monitor.
5. Within 15 seconds the Pi bridge will find the board and connect automatically.

Confirm in the bridge logs:
```bash
journalctl -u ble-mqtt-bridge -f
```
Look for: `BLE connected – gateway bridge is active`

### Step 8 — Open the dashboard

On any device connected to the TRENDnet router, open a browser and go to:
```
http://192.168.10.102:1880/dashboard/page1
```
The Smart Door Devices page will load. The heartbeat counter should be incrementing every 5 seconds.

---

## 5. Shutdown Procedure

> ⚠️ **WARNING:** Never just unplug the Raspberry Pi. Always run `sudo shutdown now` first to avoid SD card corruption.

| Order | Action |
|---|---|
| 1 | Close the dashboard browser tab on any connected devices. |
| 2 | SSH into the Pi: `ssh pi@192.168.10.102` |
| 3 | Shut down the Pi cleanly: `sudo shutdown now` |
| 4 | Wait 15 seconds until the Pi's green LED stops blinking and stays off. |
| 5 | Unplug the Pi's power cable. |
| 6 | Unplug the dev kit USB-C cable. |
| 7 | Unplug the TRENDnet router power cable. |

---

## 6. Demo Script

### 6.1 Setup Before the Audience Arrives

- Complete the full Startup Procedure (Section 4).
- Open the dashboard on the tablet or laptop that will face the audience: `http://192.168.10.102:1880/dashboard/page1`
- Confirm the heartbeat counter is incrementing — if it is, everything is working.
- Have the dev kit board visible and accessible for button presses.
- Have the serial monitor open on a laptop to show firmware output (optional but impressive).

### 6.2 Demo Actions

#### Demo 1 — Live Heartbeat (System is Alive)

Point to the QPG6200 Device panel on the dashboard.

| Action | Expected Result |
|---|---|
| Point to the Heartbeat counter | Counter increments by 1 every 5 seconds automatically. |
| Point to BLE Connection Status | Shows: ✅ Connected – QPG IoT Demo |
| Say to audience | "This heartbeat comes directly from the QPG6200 chip over BLE, through the Pi gateway, through MQTT, into Node-RED, and updates this counter in real time." |

#### Demo 2 — Physical Button Press (Doorbell)

| Action | Expected Result |
|---|---|
| Press the user button on the dev kit briefly | Doorbell animation plays on dashboard. Bell shakes. |
| | A notification popup appears bottom-right: 🔔 Door bell ringing |
| | BLUE LED on the dev kit flashes for 3 seconds. |
| Press the Ring Doorbell button on the dashboard | Same animation plays — bidirectional demo. |
| Say to audience | "A physical button press on the embedded device triggers a real-time animation on the dashboard. The same doorbell can also be triggered from the dashboard, and the LED on the board responds." |

#### Demo 3 — LED Control (Dashboard to Device)

| Action | Expected Result |
|---|---|
| Toggle the Light Switch ON on the dashboard | BLUE LED on the dev kit turns ON. |
| Toggle the Light Switch OFF on the dashboard | BLUE LED on the dev kit turns OFF. |
| Say to audience | "This shows bidirectional communication. The dashboard sends a command over MQTT, the Pi bridge receives it and sends a BLE write command to the chip, and the LED responds in under one second." |

#### Demo 4 — Device Offline Detection

| Action | Expected Result |
|---|---|
| Power off the dev kit (unplug USB-C) | Speaker/Mic panel changes to: 🔌 QPG6200 Offline |
| | Heartbeat counter stops incrementing. |
| | Movement Detector indicator turns RED. |
| Power the dev kit back on, hold button 6s | Within 15s: 📡 QPG6200 Connected reappears. |
| | Heartbeat counter resumes. |
| Say to audience | "The system detects when the IoT device goes offline and updates the dashboard status in real time. When the device reconnects it automatically resumes all data streams." |

---

## 7. Troubleshooting

### 7.1 Dashboard Not Loading

| Symptom | Fix |
|---|---|
| Browser shows "site can't be reached" | Confirm your device is on the TRENDnet network. Run `nmap -sn 192.168.10.0/24` to verify Pi is reachable. |
| Dashboard loads but is blank | Go directly to `http://192.168.10.102:1880/dashboard/page1` — this bypasses the login page. |
| Node-RED not responding | SSH into Pi and run: `sudo systemctl restart node-red` |

### 7.2 BLE Not Connecting

| Symptom | Fix |
|---|---|
| Bridge logs show "Device not found in scan" repeatedly | Dev kit is not advertising. Hold PB5 for 6 seconds to restart advertising. |
| Bridge connects then immediately disconnects | Move dev kit closer to Pi. Keep within 2 metres for reliable connection. |
| ble-mqtt-bridge service shows "failed" | Run: `sudo systemctl restart ble-mqtt-bridge` then: `journalctl -u ble-mqtt-bridge -f` |

### 7.3 Heartbeat Counter Not Moving

| Symptom | Fix |
|---|---|
| Counter is stuck at a number | BLE may have disconnected. Check bridge logs. Hold button 6s to reconnect. |
| Counter never appears | Refresh the browser. Go to `/dashboard/page1` directly. |
| MQTT messages not arriving | SSH into Pi and run: `mosquitto_sub -t 'qorvo/demo/#' -v` to verify MQTT is active. |

### 7.4 LED Not Responding to Dashboard Switch

| Symptom | Fix |
|---|---|
| Toggle switch does nothing to the LED | Confirm BLE is connected (heartbeat should be moving). If not, reconnect first. |
| LED turns on but not off (or vice versa) | Toggle the switch again. If still stuck, restart the bridge service. |

### 7.5 Pi Not Found on Network

| Symptom | Fix |
|---|---|
| nmap shows no raspberrypi.lan | Wait another 30 seconds — Pi may still be booting. Try again. |
| Pi visible but SSH refuses connection | Check that the Pi has fully booted. Try again after 30 more seconds. |
| Pi has different IP than 192.168.10.102 | Run `nmap -sn 192.168.10.0/24` to find the new IP. Update all commands accordingly. |

---

## 8. Quick Reference

### 8.1 Key IP Addresses & URLs

| Resource | Address / URL |
|---|---|
| TRENDnet Router Admin | http://192.168.10.1 (admin / admin) |
| Raspberry Pi SSH | `ssh pi@192.168.10.102` |
| Node-RED Editor | http://192.168.10.102:1880 |
| Dashboard (direct) | http://192.168.10.102:1880/dashboard/page1 |
| Linux Laptop | 192.168.10.101 (user: qorvo-radio) |
| MQTT Broker | 192.168.10.102:1883 |

### 8.2 Useful SSH Commands on the Pi

| Command | Purpose |
|---|---|
| `sudo systemctl status mosquitto node-red ble-mqtt-bridge` | Check all services at once |
| `sudo systemctl restart ble-mqtt-bridge` | Restart the BLE bridge |
| `journalctl -u ble-mqtt-bridge -f` | Watch live BLE bridge logs |
| `journalctl -u node-red -f` | Watch live Node-RED logs |
| `mosquitto_sub -t 'qorvo/demo/#' -v` | Watch all MQTT messages live |
| `sudo shutdown now` | Safe Pi shutdown |

### 8.3 Startup Checklist

| Order | Action |
|---|---|
| 1 | Plug in TRENDnet router — wait 60 seconds |
| 2 | Connect laptop to TRENDnet network |
| 3 | Plug in Raspberry Pi — wait 45 seconds |
| 4 | Run nmap to confirm Pi at 192.168.10.102 |
| 5 | SSH in and verify all 3 services are active |
| 6 | Plug in dev kit USB-C |
| 7 | Hold PB5 for 6 seconds to start BLE advertising |
| 8 | Open http://192.168.10.102:1880/dashboard/page1 |
| 9 | Confirm heartbeat counter is incrementing |

### 8.4 Shutdown Checklist

| Order | Action |
|---|---|
| 1 | Close dashboard browser tabs |
| 2 | SSH into Pi: `ssh pi@192.168.10.102` |
| 3 | Run: `sudo shutdown now` |
| 4 | Wait for Pi green LED to stop (about 15 seconds) |
| 5 | Unplug Pi power cable |
| 6 | Unplug dev kit USB-C cable |
| 7 | Unplug TRENDnet router power cable |

---

*QPG6200 BLE IoT Demo | UNC Charlotte Senior Design | March 2026*
