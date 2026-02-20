# BLE Doorbell Demo

## Introduction

The BLE Doorbell demo turns the QPG6200L Development Kit into a Bluetooth Low Energy (BLE)
peripheral that advertises as **"QPG Doorbell"**. Use **nRF Connect** or **Qorvo Connect**
to connect, subscribe to notifications, and interact with the doorbell remotely.

## Hardware Board

The app runs on the QPG6200L Development Kit.

## QPG6200 GPIO Configuration

| GPIO Name | Direction | Connected to   | Function                          |
|:---------:|:---------:|:--------------:|:----------------------------------|
| GPIO 0    | Input     | PB5            | Doorbell button / restart advertising |
| GPIO 1    | Output    | WHITE_COOL LED | BLE state indicator               |
| GPIO 12   | Output    | BLUE LED       | Ring event indicator              |
| GPIO 9    | Output    | J35 UART TX    | Serial log output                 |

## LED Behaviour

| LED         | Behaviour              | Meaning                    |
|-------------|------------------------|----------------------------|
| WHITE_COOL  | Blinking               | Advertising (waiting for connection) |
| WHITE_COOL  | Solid ON               | Phone connected            |
| WHITE_COOL  | OFF                    | Disconnected (not advertising) |
| BLUE        | Rapid blink            | Doorbell ring event        |

## Button Behaviour

| Button | Hold duration | Action                                  |
|--------|---------------|-----------------------------------------|
| PB5    | Short press   | Ring the doorbell + send BLE notification |
| PB5    | Hold ≥ 2 s   | Release to restart BLE advertising     |

## BLE GATT Services

### Battery Service (UUID 0x180F — standard)
Visible in nRF Connect as the Battery Service. Reports 100% for the demo.

### Doorbell Service (custom 128-bit UUID)
`D00RBELL-0000-1000-8000-00805F9B3400`

**Doorbell Ring Characteristic** (`...3401`):

| Operation | Value | Description                                    |
|-----------|-------|------------------------------------------------|
| Read      | 0x00  | Doorbell is idle                               |
| Read      | 0x01  | Doorbell is ringing                            |
| Notify    | 0x01  | Board → Phone: PB5 was pressed                 |
| Write     | 0x01  | Phone → Board: trigger a ring locally on the board |
| Write     | 0x00  | Phone → Board: reset the doorbell              |

## Usage with nRF Connect or Qorvo Connect

1. Power on the board — WHITE LED starts blinking (advertising)
2. Open **nRF Connect** (or **Qorvo Connect**) on your phone
3. Scan for **"QPG Doorbell"** and tap **Connect**
4. Browse to the **Doorbell Service** and find the **Doorbell Ring** characteristic
5. Enable notifications (tap the subscribe / bell icon)
6. Press **PB5** on the board — the phone receives a notification with value `0x01`
7. Write `0x01` from the app to the characteristic — the board rings locally (BLUE LED blinks)

## Serial Output

| Setting   | Value               |
|-----------|---------------------|
| UART TX   | GPIO 9              |
| Baud rate | 115200              |
| Format    | 8N1, no flow control|

### Example output

```
============================================
     QPG6200 BLE DOORBELL DEMO
============================================

Board  : QPG6200L Development Kit
Device : QPG Doorbell (BLE Peripheral)

--- LED Guide ---
  WHITE blinks = advertising
  WHITE solid  = connected to phone
  BLUE blinks  = doorbell ring event

--- Button Guide ---
  PB5 short press = ring doorbell
  PB5 hold 2s+    = restart advertising

--- App Guide ---
  1. Open nRF Connect or Qorvo Connect
  2. Scan and connect to 'QPG Doorbell'
  3. Find Doorbell Ring characteristic
  4. Enable notifications (subscribe)
  5. Press PB5 to ring! (notification=0x01)
  6. Write 0x01 to remotely ring the board

[BLE] Advertising started - scan for 'QPG Doorbell'

##############################################
#                                            #
#   ** DING DONG! **  Ring #1              #
#   Source: Local (PB5 button pressed)       #
#                                            #
##############################################

[BLE] Notification sent to phone (value=0x01)
[BLE] Phone connected! WHITE LED solid ON
```

## Build

```bash
make -f Makefile.BleDoorbell_qpg6200
```

Flash the resulting `Work/BleDoorbell_qpg6200/BleDoorbell_qpg6200.hex` using J-Link or
the QPG6200 programming tool.
