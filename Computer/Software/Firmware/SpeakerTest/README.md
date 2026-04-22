# SpeakerTest

Simple test application for the **QPG6200L Development Kit** (QPG6200LDK-01) that drives the **CSS-66668N 8Ω/3W speaker** via the **TPA2034D1 Class-D amplifier**.

## How It Works

The QPG6200L has a hardware PWMXL peripheral. This application configures PWMXL channel 4 on **GPIO10** to output a continuous **500 Hz square wave** at 50% duty cycle.

```
QPG6200L GPIO10  ──[1kΩ]──► TPA2034D1 IN+
GND              ──────────► TPA2034D1 IN−
3.3V             ──────────► TPA2034D1 VDD + SHUTDOWN (enable)
TPA2034D1 OUT+/OUT−  ──────► CSS-66668N speaker (8Ω)
```

The TPA2034D1 is a 2.75W Class-D mono amplifier with differential analog input. The PWM square wave acts as a low-frequency audio signal that the amplifier passes to the speaker at audible volume. A 500 Hz tone is clearly audible as a continuous beep.

The firmware clock prescaler and period are calculated automatically using `qDrvPWMXL_FrequencyCalculate()`, which selects the optimal settings from the 64 MHz system clock.

Debug output is sent over **UART1** (GPIO8 RX / GPIO9 TX) at **115200 baud** and reports the computed prescaler and tick count on startup.

## Hardware Required

| Part | Digikey PN | Description |
|------|-----------|-------------|
| CSS-66668N | 102-3861-ND | 8Ω 3W speaker |
| TPA2034D1YZFT | — | 2.75W Class-D mono amplifier |

## Wiring

Connect to the Arduino Uno R3 compatible headers on the QPG6200LDK-01 carrier board:

| DK Header Signal | GPIO | Connection |
|-----------------|------|-----------|
| GPIO10 | 10 | 1 kΩ resistor → TPA2034D1 **IN+** |
| GND | — | TPA2034D1 **IN−**, **GND** |
| 3.3V | — | TPA2034D1 **VDD** and **SHUTDOWN** pin |
| — | — | TPA2034D1 **OUT+** / **OUT−** → speaker |

> **Important:** The TPA2034D1 SHUTDOWN pin is active-high. Tie it to 3.3V to enable the amplifier. If left floating, the amplifier stays off.

## Building

```bash
make -f Applications/Peripherals/SpeakerTest/Makefile.speakertest_QPG6200DK
```

Output: `Work/speakertest_QPG6200DK/speakertest_QPG6200DK.hex`

Flash with SEGGER J-Link (on-board on QPG6200LDK-01):
```bash
JLinkExe -device QPG6200 -if SWD -speed 4000 -CommandFile flash.jlink
```

## Expected Behavior

- On power-up, a **500 Hz tone** immediately begins playing from the speaker.
- UART log output at startup:
  ```
  Speaker test: 500Hz tone on GPIO10 via PWMXL_4
  Tone running. Prescaler=X Ticks=XXXXX
  ```
- The tone plays continuously until the board is reset or powered off.
- No user interaction is required.
