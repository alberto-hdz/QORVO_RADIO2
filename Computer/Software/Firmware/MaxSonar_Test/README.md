# MaxSonar_Test

Minimal sensor-validation app for the **QPG6200L Development Kit**. Reads an
LV-MaxSonar EZ ultrasonic sensor over UART and prints each distance sample to
the debug console. No LEDs, no heartbeat, no wireless — just sensor in, serial
out.

---

## How It Works

The LV-MaxSonar EZ continuously streams range frames over a 9600-baud UART
whenever its `RX` pin is held HIGH. Each frame is:

```
'R' + 3 ASCII digits (inches, 0–255) + '\r'    e.g. "R079\r" = 79 inches
```

On the QPG6200L:

1. `qDrvUART` is configured on UART0 at 9600 8N1. The UART TX line idles HIGH,
   which is exactly what the sensor needs on its `RX` pin to free-run.
2. A single FreeRTOS task polls the UART RX FIFO every 50 ms, parses the
   `R<ddd>\r` frames with a three-state state machine, converts inches to
   centimetres (`cm = in * 254 / 100`), and logs the result via `gpLog`.
3. `gpLog` is routed over UART1 (GPIO 8/9) by the SDK — that is what the
   J-Link virtual COM port exposes to the host.

That's the entire application.

---

## Hardware Wiring

| Sensor pin      | QPG6200L DK     | GPIO        | Notes                              |
|-----------------|-----------------|-------------|------------------------------------|
| GND             | GND             | —           | Common ground                      |
| +5 V (Vcc)      | 5 V header      | —           | Sensor requires 5 V supply         |
| TX (serial out) | UART0 RX        | **GPIO 11** | Sensor streams frames into the MCU |
| RX (ranging en) | UART0 TX        | **GPIO 10** | Held HIGH → continuous ranging     |

UART1 (GPIO 8/9) is reserved for the debug console. The sensor's 3.3 V logic
level on `TX` is compatible with the QPG6200 directly.

```
LV-MaxSonar EZ         QPG6200L DK
┌──────────────┐       ┌─────────────────────┐
│        GND   ├───────┤ GND                 │
│        +5V   ├───────┤ 5 V header          │
│        TX  ──┼─────▶─┤ GPIO 11  (UART0 RX) │
│        RX  ◀─┼───────┤ GPIO 10  (UART0 TX) │
└──────────────┘       └─────────────────────┘
```

---

## Using It

1. **Wire the sensor** per the table above and power the DK.
2. **Open a serial monitor** on the J-Link virtual COM port:
   - **115200 baud, 8N1** (this is the *debug console*, not the sensor UART).
3. **Reset the board.** You should see:

```
=== QPG6200L - MaxSonar EZ Test ===
Sensor UART: GPIO11 RX / GPIO10 TX @ 9600 baud
[Sensor] polling started
[Sensor] 79 in  (200 cm)
[Sensor] 78 in  (198 cm)
[Sensor] 77 in  (195 cm)
...
```

Each line is one sensor reading (~10 Hz).

---

## Building

From the SDK root:

```bash
cd /workspaces/qpg6200-iot-sdk
make -f Applications/Peripherals/MaxSonar_Test/Makefile.MaxSonar_Test_qpg6200
```

Output hex (signed + merged with the bootloader):

```
Work/MaxSonar_Test_qpg6200/MaxSonar_Test_qpg6200.hex
```

---

## Flashing

Via J-Link Commander:

```bash
JLinkExe -device XP4002 -if SWD -speed 1000 -CommanderScript flash.jlink
```

where `flash.jlink` contains:

```
erase
loadfile Work/MaxSonar_Test_qpg6200/MaxSonar_Test_qpg6200.hex
r
g
exit
```

---

## Troubleshooting

| Symptom                         | Likely cause                  | Fix                                            |
|---------------------------------|-------------------------------|------------------------------------------------|
| No `[Sensor]` lines after boot  | UART RX not wired correctly   | Verify sensor `TX` → GPIO 11                   |
| Reads stuck near 6 in           | Object too close at power-up  | Clear the field of view, reset the board       |
| No debug console output at all  | Wrong port/baud               | Open UART1 (GPIO 8/9) at 115200 baud           |
| Very erratic readings           | No target / out of range      | Aim the sensor at a flat wall 1–3 m away       |
| Banner prints, no readings      | Sensor not powered            | Confirm 5 V supply — 3.3 V is not enough       |
