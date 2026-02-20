# Standalone Doorbell Demo

## Introduction

The Standalone Doorbell demo simulates a doorbell device on the QPG6200L Development Kit.
No BLE or wireless stack is required — it runs entirely standalone using buttons, LEDs, and
serial logging.

## Hardware Board

![QPG6200 Carrier Board](.images/iot_carrier_board.png)

## QPG6200 GPIO Configuration

| GPIO Name | Direction | Connected to | Function              |
|:---------:|:---------:|:------------:|:----------------------|
| GPIO 0    | Input     | PB5          | Doorbell button (ring / dismiss) |
| GPIO 3    | Input     | PB1          | Dismiss button        |
| GPIO 1    | Output    | WHITE_COOL LED | Status LED (heartbeat / ringing) |
| GPIO 11   | Output    | GREEN LED    | Ready indicator       |
| GPIO 9    | Output    | J35 UART TX  | Serial log output     |

## How It Works

### IDLE state
- **WHITE LED**: slow heartbeat flash (100 ms ON / 1900 ms OFF)
- **GREEN LED**: ON — device is ready
- **Serial**: startup banner then silent

### RINGING state (triggered by pressing PB5)
- **WHITE LED**: rapid blink (150 ms ON / 150 ms OFF)
- **GREEN LED**: OFF
- **Serial**: "** DING DONG! **" message printed every 2 seconds
- Auto-dismisses after **30 seconds** if not manually cancelled

### Dismiss (press PB5 again or press PB1)
- Returns to IDLE state
- **Serial**: shows total ring count

## Buttons

| Button | GPIO | Behaviour                                  |
|--------|------|--------------------------------------------|
| PB5    | 0    | Ring the doorbell; press again to dismiss  |
| PB1    | 3    | Always silences the doorbell               |

## Serial Output

| Setting   | Value               |
|-----------|---------------------|
| UART TX   | GPIO 9              |
| Baud rate | 115200              |
| Format    | 8N1, no flow control|

Connect any serial terminal (PuTTY, CoolTerm, `screen`, etc.) to see the doorbell log.

### Example output

```
========================================
  QPG6200 STANDALONE DOORBELL DEMO
========================================

Board  : QPG6200L Development Kit
Serial : GPIO9 TX, 115200 baud, 8N1

--- LED Guide ---
  WHITE slow flash = idle / waiting
  WHITE fast blink = RINGING!
  GREEN ON         = ready for a ring
  GREEN OFF        = currently ringing

--- Button Guide ---
  PB5 = Ring the doorbell (or dismiss)
  PB1 = Dismiss / silence

Ready! Press PB5 to ring the doorbell.

========================================
  ** DING DONG! **  Doorbell Ring #1
========================================
  Press PB5 or PB1 to dismiss.
  Auto-dismiss in 30 seconds.

----------------------------------------
  Doorbell dismissed.
  Total rings so far: 1
  WHITE LED: slow heartbeat
  GREEN LED: ON - ready for next ring
----------------------------------------
```

## Build

```bash
make -f Makefile.Doorbell_qpg6200
```

The postbuild script signs the firmware with the developer key and merges it with the
bootloader. Flash the resulting `Work/Doorbell_qpg6200/Doorbell_qpg6200.hex` using
J-Link or the QPG6200 programming tool.
