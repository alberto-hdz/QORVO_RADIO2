# ThreadBleMotionDetector_MaxSonar

QPG6200L Development Kit application: BLE-commissioned Thread motion detector using the
LV-MaxSonar EZ series ultrasonic distance sensor.

## Overview

This application runs on the QPG6200L Development Kit and provides proximity-based motion
detection using an LV-MaxSonar EZ ultrasonic sensor connected via UART. The QPG6200
simultaneously runs BLE (for initial commissioning) and OpenThread (for sensor data delivery
on a mesh network).

Key features:
- BLE advertising on boot: "QPG MaxSonar Motion"
- Phone provisions Thread network credentials via BLE GATT
- Sensor reads distance at 9600 baud; motion declared when object <= 200 cm
- Motion events published to Thread mesh via UDP multicast (ff03::1, port 5683)
- BLE GATT notifications for motion status and raw distance
- Blue LED indicates active motion detection

## Hardware Requirements

- QPG6200L Development Kit (QFN32 variant with DK LED/PB board)
- LV-MaxSonar EZ series sensor (MB1010, MB1020, MB1030, or compatible 11832-series)
- 5V power supply for sensor (DK 5V header or external)
- Three short wires for GPIO connections

## Wiring

Connect the LV-MaxSonar EZ sensor to the QPG6200L DK as follows:

```
LV-MaxSonar EZ          QPG6200L DK
------------------      ------------------
GND          -->        GND
+5V          -->        5V header
TX (pin 5)   -->        GPIO 8  (UART1 RX)
RX (pin 4)   -->        GPIO 9  (UART1 TX)
```

The sensor TX pin outputs ASCII serial data at 9600 baud, 8N1.
Frame format: 'R' + 3 decimal digits + CR  (example: "R079\r" = 79 inches = 200 cm)

GPIO 9 (UART1 TX) is held HIGH by the UART peripheral, which enables the sensor's
free-running continuous ranging mode. No external pull-up is required.

## Building

Prerequisites: build the Concurrent Light application first to produce the OpenThread
and related libraries.

```sh
# From the SDK root
make -f Applications/Concurrent/Light/Makefile.Light_Concurrent_qpg6200

# Then build this application
make -f Applications/Ble/ThreadBleMotionDetector_MaxSonar/Makefile.ThreadBleMotionDetector_MaxSonar_qpg6200
```

The signed, merged firmware image will be written to:
`Work/ThreadBleMotionDetector_MaxSonar_qpg6200/ThreadBleMotionDetector_MaxSonar_qpg6200.hex`

Flash with J-Link:
```sh
make -f Applications/Ble/ThreadBleMotionDetector_MaxSonar/Makefile.ThreadBleMotionDetector_MaxSonar_qpg6200 flash
```

## BLE Commissioning

The device must be commissioned with Thread network credentials before it can send
motion events over the mesh. Use nRF Connect (iOS/Android) or any BLE GATT client.

1. Power on the board. The WHITE LED blinks, indicating BLE advertising.
2. Scan and connect to "QPG MaxSonar Motion".
3. Write the Thread Network Name (UTF-8, up to 16 bytes) to handle 0x4002.
4. Write the Thread Network Key (16 bytes, binary) to handle 0x4004.
5. Optionally write the Channel (1 byte, 11-26) to handle 0x4006.
6. Optionally write the PAN ID (2 bytes, little-endian) to handle 0x4008.
7. Subscribe to Thread Status notifications on handle 0x400D.
8. Write 0x01 to the Join characteristic at handle 0x400A.
9. The GREEN LED begins blinking while the device joins the network.
10. Once joined, the GREEN LED turns solid and a Thread Status notification is sent.

Credentials are stored in NVM. On subsequent power cycles, the device rejoins
automatically without requiring BLE commissioning again.

To clear credentials and restart commissioning: hold PB1 for 5 seconds.

## Thread Network Operation

Once attached to a Thread network the device:

- Sends a 4-byte UDP multicast packet to ff03::1 port 5683 on every motion state change.
- Receives matching packets from other nodes on the same mesh (relay display).

### UDP Payload Format

```
Byte 0 : Message type  0x01 = motion event
Byte 1 : Motion state  0x00 = cleared, 0x01 = detected
Byte 2 : Distance (cm) high byte
Byte 3 : Distance (cm) low byte
```

Example: object at 150 cm triggers detection
```
01 01 00 96
```

Example: object moves beyond 2 m, motion cleared
```
01 00 01 90   (400 cm = 0x0190)
```

## GATT Service Layout

### Battery Service (0x180F)

| Handle | Type             | Properties  | Description            |
|--------|------------------|-------------|------------------------|
| 0x2000 | Primary Service  | -           | Service declaration    |
| 0x2001 | Characteristic   | -           | Battery Level decl.    |
| 0x2002 | Battery Level    | Read        | Current level (0-100)  |
| 0x2003 | CCC              | Read/Write  | Notification config    |

### Motion Detection Service (custom 128-bit UUID)

Base UUID: `4D4F4E00-0000-1080-8000-005F9B340000`

| Handle | Type             | Properties         | Description                     |
|--------|------------------|--------------------|---------------------------------|
| 0x3000 | Primary Service  | -                  | Service declaration             |
| 0x3001 | Characteristic   | -                  | Motion Status decl.             |
| 0x3002 | Motion Status    | Read/Write/Notify  | 0x00=none, 0x01=detected        |
| 0x3003 | CCC              | Read/Write         | Notification config             |
| 0x3004 | Characteristic   | -                  | Distance decl.                  |
| 0x3005 | Distance         | Read/Notify        | 2 bytes BE, centimetres         |
| 0x3006 | CCC              | Read/Write         | Notification config             |

### Thread Config Service (custom 128-bit UUID)

| Handle | Type             | Properties  | Description                     |
|--------|------------------|-------------|---------------------------------|
| 0x4000 | Primary Service  | -           | Service declaration             |
| 0x4002 | Network Name     | Read/Write  | Up to 16 bytes UTF-8            |
| 0x4004 | Network Key      | Write       | 16-byte master key (write-only) |
| 0x4006 | Channel          | Read/Write  | 1 byte, 11-26                   |
| 0x4008 | PAN ID           | Read/Write  | 2 bytes little-endian           |
| 0x400A | Join             | Write       | Write 0x01 to join network      |
| 0x400C | Thread Status    | Read/Notify | 0x00=disabled ... 0x04=leader   |
| 0x400D | CCC              | Read/Write  | Notification config             |

## LED Status Guide

| LED         | State    | Meaning                          |
|-------------|----------|----------------------------------|
| WHITE (BLE) | Blinking | BLE advertising                  |
| WHITE (BLE) | Solid ON | Phone connected via BLE          |
| WHITE (BLE) | OFF      | BLE disconnected                 |
| GREEN       | Blinking | Thread joining network           |
| GREEN       | Solid ON | Thread attached                  |
| GREEN       | OFF      | Thread detached                  |
| BLUE        | Solid ON | Motion detected (object <= 2 m)  |
| BLUE        | OFF      | No motion (object > 2 m or none) |

## Button Functions (PB1)

| Hold Duration | Action                                     |
|---------------|--------------------------------------------|
| 2-4 seconds   | Restart BLE advertising                    |
| >= 5 seconds  | Factory reset: clear Thread credentials    |

## Node-RED Integration

To receive motion events in Node-RED, add a UDP input node bound to port 5683,
then parse the 4-byte payload:

```javascript
// Parse motion UDP payload
const buf = msg.payload;
if (buf.length >= 4 && buf[0] === 0x01) {
    msg.motion    = buf[1] === 0x01;
    msg.distanceCm = (buf[2] << 8) | buf[3];
    msg.payload   = {
        motion:     msg.motion,
        distanceCm: msg.distanceCm
    };
    return msg;
}
return null;
```

The UDP source address will be the sensor node's Thread mesh-local IPv6 address.
