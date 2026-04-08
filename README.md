# QORVO\_RADIO2

IoT mesh network project built around the **Qorvo QPG6200L** multi-protocol SoC. The system combines custom PCB hardware, dual-stack BLE + OpenThread firmware, a Raspberry Pi MQTT gateway, and a Node-RED dashboard to demonstrate a complete end-to-end IoT product.

Developed as a Senior Design project at **UNC Charlotte — William States Lee College of Engineering**.

---

## System Architecture

```
┌─────────────────────────┐        BLE         ┌──────────────────────────┐
│  QPG6200L Dev Kit       │ ◄─────────────────►│  Raspberry Pi 4 Gateway  │
│  (Firmware application) │                    │  ble_mqtt_bridge.py      │
└─────────────────────────┘                    └────────────┬─────────────┘
        │  Thread/802.15.4 mesh                             │  MQTT (port 1883)
        ▼                                                   ▼
┌─────────────────────────┐                     ┌──────────────────────────┐
│  Other Thread nodes     │                     │  Node-RED + Dashboard    │
│  (chimes, sensors, …)   │                     │  (browser UI)            │
└─────────────────────────┘                     └──────────────────────────┘
```

The QPG6200L is a concurrent BLE 5.4 + IEEE 802.15.4 (OpenThread) radio SoC (ARM Cortex-M4F). Devices can be commissioned onto a Thread mesh network over BLE and then communicate ring/motion events across the mesh via UDP multicast.

---

## Repository Structure

```
QORVO_RADIO2/
├── Computer/
│   ├── Applications/
│   │   ├── Ble/
│   │   │   ├── BleIoTDemo/                    # BLE ↔ MQTT gateway demo
│   │   │   ├── Central/                       # BLE Central role example
│   │   │   ├── Peripheral/                    # BLE Peripheral role example
│   │   │   ├── ThreadBleDoorbell/             # Thread+BLE doorbell (carrier board)
│   │   │   ├── ThreadBleDoorbell_DK/          # Thread+BLE doorbell (standalone DK)
│   │   │   ├── ThreadBleDoorbell_DK_Analog/   # Doorbell with analog input
│   │   │   ├── ThreadBleMotionDetector_HCSR04/    # Motion detector — HC-SR04
│   │   │   ├── ThreadBleMotionDetector_MaxSonar/  # Motion detector — LV-MaxSonar
│   │   │   └── shared/                        # Shared BLE libraries
│   │   └── MovementDetector/                  # Movement detection application
│   ├── NodeRedDashboardUI/                    # Node-RED flow JSON exports
│   └── Software/Firmware/                     # Earlier firmware iterations
├── Electrical/
│   └── Hardware/
│       ├── KiCad/                         # Schematics and PCB layout (REV1, REV2)
│       ├── PCB/                           # PCB layout exports
│       ├── Gerbers/                       # Gerber files for manufacturing
│       └── Libraries/                     # Custom KiCad symbols and footprints
├── Mechanical/
│   ├── Doorbell/                          # Doorbell enclosure CAD + STL
│   └── MotionDetector/                    # Motion detector enclosure CAD + STL
├── docs/                                  # Technical documentation and datasheets
└── Tests/                                 # Test procedures
```

---

## Firmware Applications

| Application | Description |
|---|---|
| **ThreadBleDoorbell\_DK** | Smart doorbell on the QPG6200L DK. BLE commissioning, Thread mesh multicast of ring events, three input sources (button, BLE, Thread). |
| **ThreadBleMotionDetector\_MaxSonar** | Distance/motion sensor using LV-MaxSonar UART. Publishes motion events to Thread mesh and BLE GATT. |
| **ThreadBleMotionDetector\_HCSR04** | Same as above using HC-SR04 ultrasonic sensor. |
| **BleIoTDemo** | Full IoT gateway demo: QPG6200 BLE peripheral ↔ Raspberry Pi bridge ↔ MQTT ↔ Node-RED dashboard. |
| **Central / Peripheral** | Minimal BLE role examples for reference. |
| **ThreadBleDoorbell** | Doorbell variant requiring the QPG6200 carrier board. |

---

## Hardware

| Component | Details |
|---|---|
| SoC | Qorvo QPG6200L (ARM Cortex-M4F, 256 KB RAM, 1 MB Flash) |
| Radio | Concurrent BLE 5.4 + IEEE 802.15.4 (OpenThread FTD) |
| Dev Kit | QPG6200LDK-01 |
| Custom PCB | KiCad design (REV2) with inverted-F antenna, LED headers |
| Gateway | Raspberry Pi 4 running Python bridge + Mosquitto MQTT + Node-RED |
| Sensors | LV-MaxSonar EZ (UART) or HC-SR04 (GPIO) |

PCB schematics and Gerber files are in `Electrical/Hardware/`. Enclosure STL files for 3D printing are in `Mechanical/`.

---

## Getting Started

### Prerequisites

- Qorvo QPG6200 IoT SDK (provides the compiler toolchain, FreeRTOS, BLE stack, OpenThread libraries, and build system)
- GCC ARM toolchain (`arm-none-eabi-gcc`) — included in the SDK dev container
- SEGGER J-Link (built into the QPG6200LDK-01 board)
- Python 3.10+ and `bleak`, `paho-mqtt` (gateway only)

### Building the Doorbell firmware

1. Build the OpenThread FTD library first:
   ```bash
   make -f Computer/Applications/Concurrent/Light/Makefile.Light_Concurrent_qpg6200
   ```
2. Build the doorbell application:
   ```bash
   make -f Computer/Applications/Ble/ThreadBleDoorbell_DK/Makefile.ThreadBleDoorbell_DK_qpg6200
   ```
3. Flash the output hex to the dev kit:
   ```
   Work/ThreadBleDoorbell_DK_qpg6200/ThreadBleDoorbell_DK_qpg6200.hex
   ```

Refer to the per-application `README.md` files for application-specific build steps.

### Setting up the IoT gateway (BleIoTDemo)

See [`docs/QPG6200_BLE_Demo_Guide.md`](docs/QPG6200_BLE_Demo_Guide.md) for the full setup procedure including the Raspberry Pi bridge, MQTT broker, and Node-RED dashboard configuration.

---

## Documentation

| Document | Location |
|---|---|
| ThreadBleDoorbell\_DK — Technical Overview | [`docs/ThreadBleDoorbell_DK_Overview.md`](docs/ThreadBleDoorbell_DK_Overview.md) |
| ThreadBleDoorbell\_DK — Test Plan | [`docs/ThreadBleDoorbell_DK_Test_Plan.md`](docs/ThreadBleDoorbell_DK_Test_Plan.md) |
| ThreadBleMotionDetector (MaxSonar) — Overview | [`docs/ThreadBleMotionDetector_MaxSonar_Overview.md`](docs/ThreadBleMotionDetector_MaxSonar_Overview.md) |
| ThreadBleMotionDetector (MaxSonar) — Test Plan | [`docs/ThreadBleMotionDetector_MaxSonar_Test_Plan.md`](docs/ThreadBleMotionDetector_MaxSonar_Test_Plan.md) |
| BLE IoT Demo — System Guide | [`docs/QPG6200_BLE_Demo_Guide.md`](docs/QPG6200_BLE_Demo_Guide.md) |
| IoT Gateway — Documentation | [`docs/IoT_Gateway_Documentation.docx.md`](docs/IoT_Gateway_Documentation.docx.md) |
| Node-RED — Documentation | [`docs/NodeRED_Documentation.docx.md`](docs/NodeRED_Documentation.docx.md) |
| QPG6200L Datasheet | [`docs/qpg6200L_datasheet.pdf`](docs/qpg6200L_datasheet.pdf) |
| LV-MaxSonar Datasheet | [`docs/11832-LV-MaSonar-EZ_Datasheet.pub.pdf`](docs/11832-LV-MaSonar-EZ_Datasheet.pub.pdf) |

---

## License

MIT License — see [LICENSE](LICENSE) for details.
