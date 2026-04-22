# MicTest

Simple test application for the **QPG6200L Development Kit** (QPG6200LDK-01) that reads the **SPG08P4HM4H-1 PDM MEMS microphone** using the I2S peripheral and reports sound level over UART.

## How It Works

The QPG6200L has no dedicated PDM driver, but its **I2S peripheral** can function as a PDM clock master. In this configuration:

- The I2S Master clock output (**BCLK / SCK**) drives the microphone's **CLK** input at ~1 MHz
- The microphone outputs a 1-bit PDM bitstream on its **DATA** pin, which is captured by the I2S **SDI** input
- Each 16-bit I2S word received contains **16 consecutive PDM bits**

```
QPG6200L I2S_0 SCK (GPIO3) ──► Mic CLK
QPG6200L I2S_0 SDI (GPIO5) ◄── Mic DATA
QPG6200L I2S_0 WS  (GPIO2) ──► Mic L/R SELECT → GND (left channel)
3.3V                        ──► Mic VDD
GND                         ──► Mic GND
```

### PDM Bit Density

PDM audio encodes amplitude as the density of '1' bits in the bitstream:
- **~50% ones** → silence (no signal)
- **above/below 50%** → sound detected; the further from 50%, the louder the sound

The firmware collects 128 bytes (1024 PDM bits) per polling cycle using I2S polling mode, counts the '1' bits with the `__builtin_popcount` instruction, and outputs:

```
PDM density:52% level:2 ones:533/1024
PDM density:63% level:13 ones:645/1024
```

- **density**: percentage of '1' bits in the last capture (50% = silence)
- **level**: deviation from 50% baseline (0 = silence, higher = louder sound)
- **ones/total**: raw bit counts

### Clock Configuration

| Parameter | Value |
|-----------|-------|
| System clock (F_CLK) | 64 MHz |
| I2S prescaler | 31 |
| I2S SCK frequency | 64 MHz / (2 × 32) = **1 MHz** |
| Bits per capture | 1024 |
| Capture time | ~1 ms |

## Hardware Required

| Part | Digikey PN | Description |
|------|-----------|-------------|
| SPG08P4HM4H-1 | 6017-SPG08P4HM4H-1CT-ND | PDM MEMS microphone |

## Wiring

Connect to the Arduino Uno R3 compatible headers on the QPG6200LDK-01 carrier board:

| DK Header Signal | GPIO | Connection |
|-----------------|------|-----------|
| GPIO3 | 3 | Mic **CLK** pin |
| GPIO5 | 5 | Mic **DATA** pin |
| GPIO2 | 2 | Mic **L/R SELECT** → GND (or VCC for right channel) |
| 3.3V | — | Mic **VDD** |
| GND | — | Mic **GND** |

> **Mic L/R SELECT:** Tie to GND for left channel or 3.3V for right channel. This pin determines on which WS phase the mic outputs data. The firmware selects the left channel (WS low).

## Building

```bash
make -f Applications/Peripherals/MicTest/Makefile.mictest_QPG6200DK
```

Output: `Work/mictest_QPG6200DK/mictest_QPG6200DK.hex`

Flash with SEGGER J-Link (on-board on QPG6200LDK-01):
```bash
JLinkExe -device QPG6200 -if SWD -speed 4000 -CommandFile flash.jlink
```

## Monitoring Output

Connect a USB-UART adapter to **GPIO9 (TX)** / **GPIO8 (RX)** at **115200 baud, 8N1**.

Open a terminal:
```bash
minicom -D /dev/ttyUSB0 -b 115200
# or
screen /dev/ttyUSB0 115200
```

## Expected Behavior

- On startup: `Microphone test: PDM via I2S_0 Master RX`
- Continuous readings printed several times per second
- **Silence:** density ≈ 50%, level ≈ 0
- **Speaking/clapping:** density deviates from 50%, level increases (5–25+ typical)
- If the mic is not connected: density will be 0% or 100% (all zeros or all ones on the bus)
