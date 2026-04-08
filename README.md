# QORVO\_RADIO2

[![License](https://img.shields.io/github/license/alberto-hdz/QORVO_RADIO2)](LICENSE)
![Platform](https://img.shields.io/badge/SoC-QPG6200L-blue)
![School](https://img.shields.io/badge/UNC%20Charlotte-Senior%20Design-green)

A complete end-to-end IoT mesh network security system built around the **Qorvo QPG6200L**
multi-protocol SoC. The system consists of four custom 6-layer PCBs (motion detector, doorbell,
speaker, and microphone), a Raspberry Pi 4 gateway running OpenThread Border Router + Mosquitto
MQTT + Node-RED, and a live web dashboard — demonstrating a production-grade IoT product from
embedded firmware to cloud-style UI.

Developed as a Senior Design project at **UNC Charlotte — William States Lee College of Engineering**
in partnership with **Qorvo**.

---

## System Architecture

```
  ┌─────────────────────────────────────────────────────────────────────┐
  │                    Custom QPG6200L PCB Devices                      │
  │                                                                     │
  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │
  │  │   Motion    │  │   Doorbell  │  │   Speaker   │  │    Mic     │  │
  │  │  Detector   │  │  GPIO+GPADC │  │  SPI DAC+   │  │  PDM MEMS  │  │
  │  │ UART MaxSon │  │   GPIO28/5  │  │   Amp       │  │ GPIO11/12  │  │
  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └─────┬──────┘  │
  └─────────┼────────────────┼────────────────┼───────────────┼─────────┘
            │                │                │               │
            └────────────────┴────────────────┴───────────────┘
                         │  Thread mesh (IEEE 802.15.4)
                         │  UDP multicast → ff03::1 port 5683
                         ▼
              ┌───────────────────────┐
              │  nRF52840 USB Dongle  │  ← Thread RCP (Radio Co-Processor)
              │  Spinel / ttyACM0     │    plugged into Raspberry Pi 4
              └──────────┬────────────┘
                         │
                         ▼
  ┌─────────────────────────────────────────────────────────────────────┐
  │                    Raspberry Pi 4 Gateway  (192.168.10.102)         │
  │                                                                     │
  │   ┌─────────────┐    ┌───────────────┐    ┌────────────────────┐    │
  │   │    OTBR     │───►│   Mosquitto   │───►│   Node-RED 4.1.7   │    │
  │   │ (wpan0 ↔IP) │    │  MQTT :1883   │    │  Dashboard :1880   │    │
  │   └─────────────┘    └───────────────┘    └────────────────────┘    │
  │                             ▲                                       │
  │   ┌─────────────────────────┘                                       │
  │   │  ble_mqtt_bridge.py  (BLE → MQTT, systemd service)              │
  │   │  bleak + paho-mqtt, async BLE scan/connect                      │
  │   └─────────────────────────────────────────────────────────────────┤
  └─────────────────────────────────────────────────────────────────────┘
            │  BLE 5.4 (commissioning + GATT notifications)
            ▲
  QPG6200L devices ── BLE used once on first boot to push Thread credentials
                       then devices auto-rejoin Thread mesh on every reboot

                         │  HTTP / WebSocket
                         ▼
              ┌───────────────────────┐
              │    User device        │  ← Live dashboard display
              │    Node-RED UI        │    http://192.168.10.102:1880
              └───────────────────────┘
```

The QPG6200L is a concurrent BLE 5.4 + IEEE 802.15.4 (OpenThread) SoC (ARM Cortex-M4F, up to
192 MHz). On first boot, each device is commissioned onto the Thread mesh via BLE. After that,
all sensor/event data travels over the Thread mesh as UDP multicast packets. The nRF52840 USB
dongle acts as the Thread Radio Co-Processor (RCP) for the Pi, bridging 802.15.4 radio traffic
into the Pi's IP stack via OTBR.

---

## Repository Structure

```
QORVO_RADIO2/
├── Computer/
│   ├── Applications/
│   │   ├── Ble/
│   │   │   ├── BleIoTDemo/                        # BLE ↔ MQTT gateway demo
│   │   │   ├── Central/                           # BLE Central role reference
│   │   │   ├── Peripheral/                        # BLE Peripheral role reference
│   │   │   ├── ThreadBleDoorbell/                 # Doorbell — carrier board variant
│   │   │   ├── ThreadBleDoorbell_DK/              # Doorbell — standalone DK (primary)
│   │   │   ├── ThreadBleDoorbell_DK_Analog/       # Doorbell — GPADC analog input
│   │   │   ├── ThreadBleMotionDetector_HCSR04/    # Motion detector — HC-SR04
│   │   │   ├── ThreadBleMotionDetector_MaxSonar/  # Motion detector — LV-MaxSonar (primary)
│   │   │   └── shared/                            # Shared BLE/Thread libraries
│   │   └── MovementDetector/                      # Standalone movement detection app
│   ├── NodeRedDashboardUI/                        # Node-RED flow JSON exports
│   └── Software/Firmware/                         # Earlier firmware iterations
├── Electrical/
│   └── Hardware/
│       ├── KiCad/                     # Schematics and PCB layout (REV1, REV2)
│       ├── PCB/                       # PCB layout exports
│       ├── Gerbers/                   # Gerber files for JLCPCB manufacturing
│       └── Libraries/                 # Custom KiCad symbols and footprints
├── Mechanical/
│   ├── Doorbell/                      # Doorbell enclosure CAD + STL
│   └── MotionDetector/                # Motion detector enclosure CAD + STL
├── docs/                              # Technical documentation and datasheets
└── Tests/                             # Test procedures and results
```

---

## Custom PCB Devices

Four custom 6-layer PCBs were designed in KiCad and fabricated at JLCPCB. Each device is built
around the QPG6200L SoC.

| Device | Peripheral | Interface | GPIO Pins | Firmware |
|---|---|---|---|---|
| **Motion Detector** | LV-MaxSonar EZ (MB1000) ultrasonic | UART, 9600 baud | TX: GPIO0, RX: GPIO9 | `ThreadBleMotionDetector_MaxSonar` |
| **Doorbell** | Push button + GPADC | ANIO0/GPIO28 (analog), GPIO5 (button) | GPIO28, GPIO5 | `ThreadBleDoorbell_DK` |
| **Speaker** | DAC6551A SPI DAC → LM48511 amplifier | SPI | MOSI: GPIO5, SCLK: GPIO6, SYNC: GPIO7 | — |
| **Microphone** | IM69D127 PDM MEMS microphone | PDM | CLK: GPIO12, DATA: GPIO11 | — |

---

## Firmware Applications

### Custom PCB applications (primary deliverables)

| Application | Description |
|---|---|
| **ThreadBleDoorbell\_DK** | Smart doorbell on QPG6200L. BLE commissioning on first boot, Thread credentials persisted to NVM, auto-rejoin on reboot. Ring events sent as UDP multicast (`ff03::1:5683`) and BLE GATT notifications. 3-sample GPIO debounce on button input. |
| **ThreadBleMotionDetector\_MaxSonar** | Motion detector using LV-MaxSonar EZ over UART at 9600 baud. State-change-only reporting (only transmits when distance changes by more than 2 cm). Motion threshold: 200 cm. |

### Dev kit reference applications

| Application | Description |
|---|---|
| **ThreadBleMotionDetector\_HCSR04** | Same motion detector logic using HC-SR04 ultrasonic sensor via GPIO trigger/echo. |
| **BleIoTDemo** | Full IoT gateway demo: QPG6200L BLE peripheral ↔ Raspberry Pi bridge ↔ MQTT ↔ Node-RED. |
| **Central / Peripheral** | Minimal BLE role examples for reference. |
| **ThreadBleDoorbell** | Doorbell variant requiring the QPG6200 carrier board (not standalone DK). |

---

## Hardware

| Component | Details |
|---|---|
| SoC | Qorvo QPG6200L — ARM Cortex-M4F @ up to 192 MHz, 336 KB RAM, 2 MB NVM, QFN32 |
| Radio | Concurrent BLE 5.4 + IEEE 802.15.4 (OpenThread FTD), 10 dBm TX power |
| Dev Kit | QPG6200LDK-01 |
| Custom PCBs | 4× 6-layer boards (KiCad REV2), inverted-F antenna, fabricated at JLCPCB |
| Thread RCP | nRF52840 USB dongle — RCP firmware, Spinel protocol, `/dev/ttyACM0` |
| Gateway | Raspberry Pi 4 (192.168.10.102) — OTBR + Mosquitto + Node-RED |
| Dashboard | User device running Node-RED web UI |
| Router | TRENDnet TEW-827DRU — private LAN 192.168.10.x (avoids university WiFi isolation) |
| Sensors | LV-MaxSonar EZ MB1000 (UART), IM69D127 PDM mic, DAC6551A SPI DAC |
| Debugger | SEGGER J-Link via 10-pin SWD (built into QPG6200LDK-01) |

PCB schematics and Gerber files are in `Electrical/Hardware/`. Enclosure STL files are in
`Mechanical/`.

---

## Getting Started

### Prerequisites

- **Qorvo QPG6200 IoT SDK** — provides the compiler toolchain, FreeRTOS, BLE host stack
  (Cordio ARM), OpenThread libraries, and build system. Clone it and open the project in the
  provided VS Code dev container (Docker must be running).
- **GCC ARM toolchain** (`arm-none-eabi-gcc`) — included inside the SDK dev container; no
  separate install needed if using the container.
- **SEGGER J-Link** — built into the QPG6200LDK-01 board. Used for both flashing and
  live debugging over SWD. No separate SPI programming mode needed during development.
- **Python 3.10+** with `bleak` and `paho-mqtt` — gateway only, runs on the Raspberry Pi.

### Step 0 — Open the SDK dev container (firmware builds only)

All firmware must be built inside the Qorvo SDK VS Code dev container. From inside the container:

```bash
# Verify the toolchain is available
arm-none-eabi-gcc --version
```

### Building the Doorbell firmware

1. Build the OpenThread FTD library:
   ```bash
   make -f Makefile.OpenThread_FTD_qpg6200
   ```
2. Build the doorbell application:
   ```bash
   make -f Computer/Applications/Ble/ThreadBleDoorbell_DK/Makefile.ThreadBleDoorbell_DK_qpg6200
   ```
3. Flash the output `.hex` via J-Link SWD:
   ```
   Work/ThreadBleDoorbell_DK_qpg6200/ThreadBleDoorbell_DK_qpg6200.hex
   ```

Refer to the per-application `README.md` files for application-specific build steps.

### BLE commissioning (first boot on any device)

After flashing, Thread credentials must be provisioned over BLE before the device joins the mesh:

1. Power on the device.
2. Hold **PB5** for **6 seconds** — the device begins BLE advertising.
3. Use the Raspberry Pi gateway (or a BLE tool) to connect and write Thread network credentials.
4. Credentials are persisted to NVM. The device auto-rejoins on every subsequent reboot — no
   repeat commissioning needed.

### Setting up the IoT gateway

See [`docs/IoT_Gateway_Documentation.docx.md`](docs/IoT_Gateway_Documentation.docx.md) for the
full Raspberry Pi setup (OTBR, Mosquitto, Node-RED, and `ble_mqtt_bridge.py`).

The Node-RED dashboard is accessible at:
```
http://192.168.10.102:1880/dashboard/page1
```
> **Note:** Navigate directly to `/dashboard/page1`. The login page has a known Vue rendering
> incompatibility and will not display correctly.

---

## Documentation

| Document | Location |
|---|---|
| ThreadBleDoorbell\_DK — Technical Overview | [`docs/ThreadBleDoorbell_DK_Overview.md`](docs/ThreadBleDoorbell_DK_Overview.md) |
| ThreadBleDoorbell\_DK — Test Plan | [`docs/ThreadBleDoorbell_DK_Test_Plan.md`](docs/ThreadBleDoorbell_DK_Test_Plan.md) |
| ThreadBleMotionDetector (MaxSonar) — Overview | [`docs/ThreadBleMotionDetector_MaxSonar_Overview.md`](docs/ThreadBleMotionDetector_MaxSonar_Overview.md) |
| ThreadBleMotionDetector (MaxSonar) — Test Plan | [`docs/ThreadBleMotionDetector_MaxSonar_Test_Plan.md`](docs/ThreadBleMotionDetector_MaxSonar_Test_Plan.md) |
| BLE IoT Demo — System Guide | [`docs/QPG6200_BLE_Demo_Guide.md`](docs/QPG6200_BLE_Demo_Guide.md) |
| IoT Gateway — Setup and Configuration | [`docs/IoT_Gateway_Documentation.docx.md`](docs/IoT_Gateway_Documentation.docx.md) |
| Node-RED Dashboard — Documentation | [`docs/NodeRED_Documentation.docx.md`](docs/NodeRED_Documentation.docx.md) |
| QPG6200L Datasheet | [`docs/qpg6200L_datasheet.pdf`](docs/qpg6200L_datasheet.pdf) |
| LV-MaxSonar EZ Datasheet | [`docs/11832-LV-MaSonar-EZ_Datasheet.pub.pdf`](docs/11832-LV-MaSonar-EZ_Datasheet.pub.pdf) |

---

## License

See [LICENSE](LICENSE) for terms. This project was developed using the Qorvo QPG6200 IoT SDK,
which is subject to Qorvo's proprietary license terms.
