# ThreadBleMotionDetector_HCSR04

## Overview

ThreadBleMotionDetector_HCSR04 is a BLE + Thread motion detection application for the **QPG6200L Development Kit**. It uses an HC-SR04 ultrasonic distance sensor connected via GPIO to detect objects within a configurable range (default: 200 cm / 2 m). Detection events are reported over Bluetooth LE (GATT notifications) and Thread (UDP multicast).

A smartphone acts as BLE Commissioner: it connects to the device over BLE, provisions Thread network credentials via a custom GATT service, and receives real-time motion alerts as GATT notifications. Once commissioned, the device joins the Thread mesh and multicasts motion events to all Thread nodes at `ff03::1:5683` (CoAP-style port).

---

## Hardware Requirements

| Item | Details |
|------|---------|
| Board | QPG6200L Development Kit |
| Sensor | HC-SR04 ultrasonic distance sensor |
| Trig pin | GPIO 28 (ANIO0) — 3.3 V digital output |
| Echo pin | GPIO 29 (ANIO1) — **see voltage divider note below** |
| Power | HC-SR04 VCC = 5 V; GND shared with board GND |

### Echo Line Voltage Divider (Required)

The HC-SR04 Echo output is a 5 V signal. The QPG6200 GPIO inputs are **3.3 V tolerant only**. A simple resistor voltage divider must be placed between the sensor Echo pin and GPIO 29:

```
HC-SR04 Echo (5V) ──┬── 1kΩ ──── GPIO 29 (QPG6200)
                    │
                   2kΩ
                    │
                   GND
```

This divides 5 V → ~3.3 V at the GPIO input. Any equivalent divider with a ratio of ~0.66 and total impedance < 10 kΩ is acceptable.

---

## Wiring Summary

| HC-SR04 Pin | QPG6200L DK |
|------------|------------|
| VCC | 5 V header |
| GND | GND |
| Trig | GPIO 28 / ANIO0 (3.3 V output, direct connection) |
| Echo | GPIO 29 / ANIO1 (via voltage divider, see above) |

---

## LED Behaviour

| LED | Colour | Meaning |
|-----|--------|---------|
| LD1 | WHITE (solid) | BLE advertising / connected |
| LD2 | GREEN (solid) | Thread joined |
| LD3 | BLUE (solid) | Motion detected (object ≤ 200 cm) |
| LD3 | Off | No motion / object > 200 cm |

---

## Distance & Detection Logic

- Trigger: 10 µs HIGH pulse on GPIO 28
- Echo: pulse width (µs) on GPIO 29 is proportional to round-trip distance
- Formula: `distance_cm = echo_pulse_us / 58`
- Motion detected when: `0 < distance_cm ≤ 200`
- Measurement interval: 100 ms
- Timeout (no echo): 25 ms → reported as out-of-range (no motion)

---

## Build Instructions

### Prerequisites

- QPG6200 IoT SDK toolchain installed (arm-none-eabi-gcc, make)
- `appuc-firmware-packer` on PATH
- Python 3

### Build

```sh
cd Applications/Ble/ThreadBleMotionDetector_HCSR04
make -f Makefile.ThreadBleMotionDetector_HCSR04_qpg6200
```

The output hex file (with bootloader merged) is placed at:

```
Work/ThreadBleMotionDetector_HCSR04_qpg6200/ThreadBleMotionDetector_HCSR04_qpg6200.hex
```

### Flash

Use the Qorvo programming tool or J-Flash to flash the merged hex to the QPG6200L DK.

---

## BLE Commissioning

1. Power on the board. LED1 (WHITE) begins advertising as **"QPG HC-SR04 Motion"**.
2. Open a BLE commissioning app (e.g., the Qorvo Thread Commissioning App or nRF Connect).
3. Connect to **QPG HC-SR04 Motion**.
4. Write Thread credentials to the Thread Configuration GATT service (handles `0x4000`–`0x400D`):
   - Network Name
   - Master Key (Network Key)
   - PAN ID
   - Channel
   - Extended PAN ID
5. Write `0x01` to the **Join** characteristic (`0x400D`) to commit credentials and start Thread.
6. LED2 (GREEN) illuminates when the device successfully joins the Thread network.

---

## Thread Network

Once commissioned, the device:

- Joins as a **Full Thread Device**
- Sends UDP multicast on `ff03::1` port `5683` when motion state changes
- Payload format (4 bytes):

| Byte | Value | Description |
|------|-------|-------------|
| 0 | `0x01` | Message type: motion event |
| 1 | `0x00` / `0x01` | Motion state (0 = clear, 1 = detected) |
| 2 | distance high byte | Distance in cm (big-endian) |
| 3 | distance low byte | Distance in cm (big-endian) |

Other Thread nodes in the network will receive these multicast packets on UDP port 5683.

---

## GATT Services

### Battery Service (0x2000–0x2003)

| Handle | UUID | Properties | Description |
|--------|------|-----------|-------------|
| 0x2001 | 0x2A19 | Read, Notify | Battery Level (%) |
| 0x2002 | 0x2902 | Read, Write | Battery Level CCC |

### Motion Detection Service (0x3000–0x3007)

Base UUID: `4D4F544E-0000-1000-8000-00805F9B34FB`

| Handle | UUID (short) | Properties | Description |
|--------|-------------|-----------|-------------|
| 0x3002 | `...4E` | Read, Write, Notify | Motion Status (0=clear, 1=detected) |
| 0x3003 | 0x2902 | Read, Write | Motion Status CCC |
| 0x3005 | `...4F` | Read, Notify | Distance (cm, 2 bytes big-endian) |
| 0x3006 | 0x2902 | Read, Write | Distance CCC |

### Thread Configuration Service (0x4000–0x400D)

| Handle | Description |
|--------|-------------|
| 0x4002 | Network Name (write) |
| 0x4004 | Master Key (write) |
| 0x4006 | PAN ID (write) |
| 0x4008 | Channel (write) |
| 0x400A | Extended PAN ID (write) |
| 0x400C | Join trigger (write 0x01 to join) |
| 0x400D | Thread Status (read, notify) |

---

## Node-RED Integration

A Node-RED flow can listen for UDP packets on port 5683 from `ff03::1` on a Thread border router to receive motion events. Example flow:

1. **UDP input** node: bind to port 5683
2. **Function** node to parse the 4-byte payload:
   ```javascript
   const buf = msg.payload;
   if (buf[0] === 0x01) {
       msg.motion  = buf[1] === 0x01 ? "DETECTED" : "CLEAR";
       msg.distance = (buf[2] << 8) | buf[3];
   }
   return msg;
   ```
3. **Debug** / **Dashboard** node to display motion state and distance

---

## Factory Reset

Hold **BTN1** for 5 seconds to erase Thread credentials and restart BLE advertising.

---

## License

Copyright (c) 2024-2025, Qorvo Inc. SPDX-License-Identifier: LicenseRef-Qorvo-1
