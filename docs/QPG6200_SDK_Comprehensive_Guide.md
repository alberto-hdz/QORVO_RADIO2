# QPG6200 IoT SDK — Comprehensive Developer Guide

> **SDK Version:** 2.0.1 (October 17, 2025)  
> **Target SoC:** Qorvo QPG6200 / QPG6200L  
> **Architecture:** ARM Cortex-M4F  

---

## Table of Contents

1. [SDK Overview](#1-sdk-overview)
2. [Hardware: QPG6200 SoC](#2-hardware-qpg6200-soc)
3. [Repository Structure](#3-repository-structure)
4. [Toolchain & Build Environment](#4-toolchain--build-environment)
5. [Build System Deep-Dive](#5-build-system-deep-dive)
6. [SDK Initialization & Boot Flow](#6-sdk-initialization--boot-flow)
7. [Memory Layout & Linker Scripts](#7-memory-layout--linker-scripts)
8. [FreeRTOS Integration](#8-freertos-integration)
9. [Hardware Abstraction Layer (HAL)](#9-hardware-abstraction-layer-hal)
10. [Peripheral Driver Layer](#10-peripheral-driver-layer)
11. [Bluetooth Low Energy (BLE) Stack](#11-bluetooth-low-energy-ble-stack)
12. [Thread / OpenThread Stack](#12-thread--openthread-stack)
13. [Zigbee Stack](#13-zigbee-stack)
14. [Matter (CHIP) Stack](#14-matter-chip-stack)
15. [ConcurrentConnect — Multi-Protocol Operation](#15-concurrentconnect--multi-protocol-operation)
16. [Application Architecture Patterns](#16-application-architecture-patterns)
17. [Example Applications](#17-example-applications)
18. [Configuration System](#18-configuration-system)
19. [Non-Volatile Memory (NVM)](#19-non-volatile-memory-nvm)
20. [Security Architecture](#20-security-architecture)
21. [Bootloader & OTA Updates](#21-bootloader--ota-updates)
22. [Power Management](#22-power-management)
23. [Logging & Debugging](#23-logging--debugging)
24. [Factory Data & Provisioning](#24-factory-data--provisioning)
25. [Certifications](#25-certifications)
26. [Quick-Start Walkthrough](#26-quick-start-walkthrough)

---

## 1. SDK Overview

The QPG6200 IoT Software Development Kit (SDK) is Qorvo's official production-ready firmware development environment for the QPG6200 multiprotocol System-on-Chip. It provides a complete vertical stack from bare-metal hardware drivers up through certified wireless protocol stacks (BLE 5.4, Thread 1.4, Zigbee R23, Matter 1.4) to reference application code.

### Key Capabilities

| Capability | Detail |
|---|---|
| Wireless Protocols | BLE 5.4, Thread 1.4 (MTD/FTD), Zigbee R23, Matter 1.4 |
| Multi-protocol | ConcurrentConnect — simultaneous operation of all protocols |
| RTOS | FreeRTOS 10.3.1 with tickless idle |
| Security | PSA Certified Level 2, Secure Boot, AES-GCM NVM encryption |
| OTA Updates | LZMA-compressed dual-image OTA |
| Programming Language | C (drivers/RTOS), C++ (Matter/application layer) |
| Build System | GNU Make + Python configuration generation |
| Compiler | arm-none-eabi-gcc (ARM GCC Embedded) |

### Unique Features

- **ConcurrentConnect**: Unlike time-division-multiplex solutions, QPG6200 can run BLE, Thread, and Zigbee simultaneously without dropping packets or introducing delays on any protocol.
- **Antenna Diversity**: Dual physical antennas with automatic selection for maximum range and reliability.
- **Competitive Power Efficiency**: Tickless idle, sleep modes with GPIO wakeup, ultra-low-power standby.

---

## 2. Hardware: QPG6200 SoC

### Processor Core

| Parameter | Value |
|---|---|
| CPU | ARM Cortex-M4F |
| Clock Frequency | 32 MHz (main) |
| FPU | Single-precision hardware floating point |
| ISA | ARMv7E-M (Thumb-2) |

### Memory

| Region | Base Address | Size | Purpose |
|---|---|---|---|
| NRT Flash (CODE_NRT) | `0x10013000` | ~1.8 MB (application space) | Application code + read-only data |
| UCRAM | `0x20000000` | 288 KB | User accessible RAM (stack, heap, BSS) |
| SYSRAM | `0x40000000` | 16 KB | System/radio RAM |
| AKRAM_NRT | `0x42038000` | 18 KB | AKU non-real-time RAM |

The first 0xC000 bytes of flash (`0x10000000`–`0x10013000`) are reserved for the bootloader and ROM code.

### Radio / RF

- **Frequency Band**: 2.4 GHz ISM
- **Supported Standards**: IEEE 802.15.4 (Thread/Zigbee) and Bluetooth LE 5.4
- **Antenna Diversity**: Two antenna ports with automatic switching
- **Multiprotocol Radio Sharing**: Hardware-level MAC dispatcher + RX arbiter

### Peripherals

GPIO, UART (multiple), SPI Master/Slave, I2C Master/Slave, General-Purpose ADC (GPADC), High-Resolution ADC (HRADC), PWM (PWMXL), I2S (audio), DMA, Timers, Watchdog, Low-Power Comparator, Keypad controller, IR receiver, LED PWM controller, Temperature sensor, Battery monitor.

### Integrated Secure Element

- PSA Certified Level 2
- AES hardware acceleration (offloaded from Cortex-M4)
- ECDSA P-256 key generation and signature verification
- Secure key storage — keys never leave the element in plaintext
- Secure Boot chain of trust anchored in ROM

---

## 3. Repository Structure

```
qpg6200-iot-sdk/
├── Applications/          # All reference and demo applications
│   ├── Ble/               # BLE-only applications
│   ├── Bootloader/        # Application bootloader source
│   ├── Combo/             # Combo protocol applications
│   ├── Concurrent/        # ConcurrentConnect applications (flagship)
│   ├── Doorbell/          # Legacy doorbell example
│   ├── HelloWorld/        # Minimal "blink + log" starter project
│   ├── Matter/            # Matter-only endpoint applications
│   ├── MovementDetector/  # Motion detection demo
│   ├── Peripherals/       # Driver usage examples
│   ├── PTC/               # Product Test Component (RF testing)
│   ├── Sleep/             # Power management demo
│   ├── shared/            # Shared application modules
│   └── Zigbee/            # Zigbee-only applications
│
├── Binaries/              # Pre-built application binaries (requires Git LFS)
│
├── Components/
│   ├── Qorvo/             # Qorvo proprietary components
│   │   ├── 802_15_4/      # IEEE 802.15.4 MAC layer
│   │   ├── BaseUtils/     # Core utilities (logging, assert, scheduler)
│   │   ├── BleController/ # BLE link-layer controller
│   │   ├── Bootloader/    # Bootloader components
│   │   ├── BSP/           # Board support package
│   │   ├── HAL_PLATFORM/  # Cortex-M4 platform HAL
│   │   ├── HAL_RF/        # RF hardware abstraction
│   │   ├── Matter/        # Qorvo Matter glue
│   │   ├── OpenThread/    # Qorvo OpenThread platform adaptation
│   │   ├── OS/            # OS abstraction layer
│   │   ├── Rt/            # Real-time components
│   │   ├── Test/          # Test infrastructure
│   │   ├── Zigbee/        # Zigbee application layer
│   │   └── gpPeripheral/  # Peripheral register maps and low-level drivers
│   │
│   └── ThirdParty/        # Open-source and licensed third-party software
│       ├── ARM/           # ARM CMSIS / Cortex-M headers
│       ├── BleHost/       # ARM Cordio BLE host stack
│       ├── CMSIS_5/       # ARM CMSIS-5
│       ├── FreeRTOS/      # FreeRTOS kernel 10.3.1
│       ├── Matter/        # CHIP/Matter SDK
│       ├── mera/          # Additional Matter dependencies
│       ├── OpenThread/    # OpenThread 1.4 stack
│       ├── secure_element/# Secure element firmware
│       └── Zigbee/        # Third-party Zigbee stack
│
├── Documents/             # PDF datasheets, API manuals, user journeys
│   ├── API Manuals/       # Qorvo component API reference PDFs
│   ├── User_Journey/      # Step-by-step guides (Security, Production)
│   └── User Manuals/      # Hardware and SDK user manuals
│
├── Files/                 # Project documentation and reference files
│
├── Libraries/             # Pre-built middleware libraries
│   └── Qorvo/
│       ├── FactoryBlock/  # Manufacturer data storage
│       ├── FactoryData/   # Device-specific provisioning data
│       ├── MatterQorvoGlue/ # Matter-to-Qorvo adapter
│       ├── MatterStack/   # Matter stack wrapper
│       ├── mbedtls_alt/   # mbedTLS with Secure Element offloading
│       └── PersistentStorageBlock/ # Runtime NVM persistence
│
├── make/                  # Shared Makefile rules and configurations
│   └── gpcommon.mk        # Master build rules
│
├── Scripts/               # Setup and utility scripts
│   ├── bootstrap.sh       # Environment setup script
│   └── setup_submodules.sh # Git submodule initialization
│
├── Tools/                 # Development and production tools
│   ├── Bootloader/        # Bootloader image tools
│   ├── DataBlock/         # Data block programming tools
│   ├── FactoryData/       # Factory data programming tools
│   ├── Hex/               # Hex file merging tools
│   ├── MatterControllers/ # Matter test controllers
│   ├── OTA/               # OTA image generation tools
│   ├── QorvoPlatformTools/# Qorvo programming/debug tools
│   ├── RadioControlConsole/ # RF test console
│   └── SecureBoot/        # Secure boot configuration tools
│
├── make/                  # Build system
├── sphinx/                # Sphinx documentation source
├── Makefile               # Root makefile
├── README.md
└── RELEASE_NOTES.md
```

---

## 4. Toolchain & Build Environment

### Required Tools

| Tool | Purpose | Notes |
|---|---|---|
| `arm-none-eabi-gcc` | C/C++ compiler | ARM GCC Embedded |
| `arm-none-eabi-ld` | Linker | Part of ARM GCC Embedded |
| `arm-none-eabi-objcopy` | Binary conversion (ELF → HEX) | |
| `arm-none-eabi-size` | Flash/RAM usage reporting | |
| `arm-none-eabi-nm` | Symbol table analysis | |
| Python 3 | Build configuration generation | Generates Makefiles and headers from `.py` config files |
| GNU Make | Build orchestration | |
| J-Link | Flash programming and debugging | Segger J-Link hardware required |
| Git LFS | Access pre-built binaries | Required for `Binaries/` directory |

### Optional Tools

| Tool | Purpose |
|---|---|
| `ccache` | Compiler cache to speed up rebuilds |
| ZAP (Zigbee/Matter Attribute Parser) | Generate Matter cluster attribute code |
| Spake2p | Matter commissioning credential generation |
| Qorvo Platform Tools | Secure boot key provisioning, factory data |

### DevContainer Environment

The SDK ships with a pre-configured VS Code DevContainer (`.devcontainer/`) that:
- Pre-installs all compiler toolchain and Python dependencies
- Automates `bootstrap.sh` execution
- Sets up Git submodules via `setup_submodules.sh`
- Provides a ready-to-build environment immediately upon container start

### Compiler Configuration

The build targets ARM Cortex-M4F with hardware FPU:

```
-mcpu=cortex-m4
-mthumb
-mfloat-abi=hard
-mfpu=fpv4-sp-d16
```

Optimization flags vary by build type:
- **Debug**: `-Og -g3` (optimized for debugging)
- **Release**: `-Os` (optimized for size)

---

## 5. Build System Deep-Dive

### Architecture

The build system is a layered Makefile system driven by Python-generated configuration:

```
Python Config (.py)  →  generates  →  Makefile variables + qorvo_config.h
                                              ↓
                            gpcommon.mk (master rules)
                                              ↓
                          Application Makefile (e.g., Makefile.HelloWorld_qpg6200)
                                              ↓
                                  arm-none-eabi-gcc
```

### Application Makefile

Each application has a dedicated Makefile named `Makefile.<AppName>_qpg6200`. Example for HelloWorld:

```makefile
# Makefile.HelloWorld_qpg6200
APP_NAME  = HelloWorld
TARGET    = qpg6200
WORK_DIR  = Work/$(APP_NAME)_$(TARGET)

# Component selections
GP_COMPONENTS += gpBaseComps gpCom gpLog gpSched gpHal
GP_COMPONENTS += FreeRTOS StatusLed qPinCfg

# Include master build rules
include $(SDK_ROOT)/make/gpcommon.mk
```

### Generated Files (per application)

The build generates several files in `Applications/<App>/gen/qpg6200/`:

| File | Purpose |
|---|---|
| `qorvo_config.h` | Feature flags, buffer sizes, component enables |
| `qorvo_internals.h` | Internal component defines |
| `<App>_qpg6200.ld` | Application-specific linker script |
| `gpHal_RtSystem.c` | Real-time system configuration |
| `esec_config.h` | Secure element configuration |
| `q_info_page.h` | Application info page (version, build metadata) |

### Build Outputs

Build artifacts are placed in `Work/<AppName>_qpg6200/`:

| File | Description |
|---|---|
| `<App>.elf` | Full ELF with debug symbols |
| `<App>.hex` | Intel HEX for programming |
| `<App>.map` | Linker map file (symbol addresses, section sizes) |
| `<App>.xml` | Component dependency graph |
| `<App>_merged.hex` | Final merged HEX (app + bootloader + factory data) |

### Post-Build Script

Each application has a `<App>_qpg6200_postbuild.sh` that:
1. Generates the merged HEX using `hexmerge`
2. Applies secure boot signatures (if configured)
3. Packages OTA image (LZMA compressed)

### Building an Application

```bash
# From the repository root
cd Applications/HelloWorld
make -f Makefile.HelloWorld_qpg6200

# Clean build
make -f Makefile.HelloWorld_qpg6200 clean

# Build with specific configuration
make -f Makefile.HelloWorld_qpg6200 DEBUG=1
```

---

## 6. SDK Initialization & Boot Flow

### Complete Boot Sequence

```
Power-on Reset
      │
      ▼
ROM Boot Code (Secure Element)
  • Verifies bootloader signature (ECDSA P-256)
  • Transfers execution to bootloader
      │
      ▼
Application Bootloader (optional)
  • Checks for OTA image in secondary slot
  • Validates image signatures
  • Selects primary or secondary firmware
  • Jumps to application start address
      │
      ▼
ARM Cortex-M4 Reset Handler
  • Copies .data section from flash to RAM
  • Zeroes .bss section
  • Calls C++ static constructors (.ctors)
  • Calls main()
      │
      ▼
main()
  ├── HAL_INITIALIZE_GLOBAL_INT()   // Disable global interrupts
  ├── HAL_INIT()                    // Initialize hardware peripherals
  ├── HAL_ENABLE_GLOBAL_INT()       // Re-enable interrupts
  ├── gpSched_Init()                // Initialize Qorvo scheduler task
  ├── gpSched_ScheduleEvent(0, Application_Init)  // Queue init work
  └── vTaskStartScheduler()         // Hand control to FreeRTOS
      │
      ▼
FreeRTOS Scheduler (running)
  │
  └── gpSched Task executes Application_Init()
        ├── gpHal_Set32kHzCrystalAvailable(false/true)  // Crystal config
        ├── gpBaseComps_StackInit()   // Core Qorvo components
        ├── gpCom_Init()              // Communication layer
        ├── gpLog_Init()              // Logging subsystem
        ├── qPinCfg_Init(NULL)        // GPIO pin configuration
        ├── [Protocol stack inits]    // BLE / Thread / Zigbee / Matter
        └── [Application tasks]       // xTaskCreateStatic(...)
```

### HAL_INIT() Breakdown

`HAL_INIT()` is a macro that calls the platform HAL initializer which:
1. Configures the PLL and system clocks
2. Initializes the flash controller
3. Sets up the interrupt controller (NVIC)
4. Initializes DMA
5. Configures the Secure Element interface
6. Sets up power management hardware

### gpSched: Qorvo Cooperative Scheduler

`gpSched` is Qorvo's lightweight cooperative event scheduler that runs as a FreeRTOS task. It provides:
- `gpSched_Init()` — Creates the scheduler FreeRTOS task
- `gpSched_ScheduleEvent(delay_us, callback)` — Schedule a callback
- `gpSched_ScheduleEventArg(delay_us, callback, arg)` — Schedule with argument

The scheduler task runs all its callbacks from a single FreeRTOS task with a dedicated (large) stack. This is intentional — the application initialization (which is stack-intensive) is dispatched through the scheduler to benefit from its larger stack compared to `main()`.

---

## 7. Memory Layout & Linker Scripts

### Flash Memory Map

```
0x10000000  ┌─────────────────────────────┐
            │  ROM / Secure Element Area  │  (not application-accessible)
0x10013000  ├─────────────────────────────┤
            │  Bootloader Header          │  (.bl_fw_header)
0x1001F000  ├─────────────────────────────┤
            │  App UC Firmware Header     │  (.appuc_fw_header)
0x1001F400  ├─────────────────────────────┤
            │  App Firmware Header        │
0x1001F600  ├─────────────────────────────┤
            │  ISR Vector Table           │  (.isr_vector, 256-byte aligned)
0x1001F800  ├─────────────────────────────┤
            │  RT Flash Data              │  (.rt_flash — radio calibration)
0x10020000  ├─────────────────────────────┤
            │                             │
            │  Application Code + Data    │  (.text, .rodata)
            │                             │
            │  (up to ~1.8 MB available)  │
            │                             │
0x1FD000    ├─────────────────────────────┤
            │  NVM / Factory Data Area    │
0x1FFFFF    └─────────────────────────────┘
```

### RAM Memory Map

```
0x20000000  ┌─────────────────────────────┐
            │  .data (initialized vars)   │
            ├─────────────────────────────┤
            │  .bss (zero-initialized)    │
            ├─────────────────────────────┤
            │  FreeRTOS Heap              │
            ├─────────────────────────────┤
            │  Task Stacks (static alloc) │
            ├─────────────────────────────┤
            │  Free RAM                   │
0x20048000  └─────────────────────────────┘  (288 KB total UCRAM)

0x40000000  ┌─────────────────────────────┐
            │  SYSRAM (16 KB)             │  Radio / system use
0x40004000  └─────────────────────────────┘

0x42038000  ┌─────────────────────────────┐
            │  AKRAM_NRT (18 KB)          │  AKU non-real-time
0x4203C800  └─────────────────────────────┘
```

### Linker Script Sections

The generated `.ld` file defines the following key sections:

| Section | Location | Purpose |
|---|---|---|
| `.bl_fw_header` | Flash @ 0x10013000 | Bootloader firmware header (magic, version, CRC) |
| `.appuc_fw_header` | Flash @ 0x1001F000 | Application upgrade header |
| `.isr_vector` | Flash @ 0x1001F600 | ARM vector table (256-byte aligned) |
| `.rt_flash` | Flash @ 0x1001F800 | Real-time radio calibration constants |
| `.text` | Flash (after vector table) | Application machine code |
| `.rodata` | Flash (after .text) | Read-only data, string literals |
| `.data` | RAM (copied from flash at startup) | Initialized global/static variables |
| `.bss` | RAM (zeroed at startup) | Uninitialized global/static variables |
| `.ctors` / `.dtors` | Flash | C++ constructor/destructor tables |
| `.heap` / `.stack` | RAM | FreeRTOS heap and system stack |

---

## 8. FreeRTOS Integration

### Version & Configuration

- **FreeRTOS Version**: 10.3.1 (Qorvo-customized)
- **Kernel Port**: ARM Cortex-M4F (with FPU support)
- **Config file**: `Applications/<App>/gen/qpg6200/FreeRTOSConfig.h` (or per-component override)

### Key FreeRTOSConfig.h Parameters

```c
#define configCPU_CLOCK_HZ              32000000UL    // 32 MHz
#define configTICK_RATE_HZ              1000          // 1 ms tick
#define configMAX_PRIORITIES            8             // 8 priority levels (0=idle, 7=highest)
#define configMINIMAL_STACK_SIZE        128           // Words (512 bytes minimum stack)
#define configTOTAL_HEAP_SIZE           2800          // Bytes (static heap for FreeRTOS objects)
#define configUSE_TICKLESS_IDLE         1             // Enable low-power tickless idle
#define configUSE_PREEMPTION            1             // Preemptive scheduling
#define configUSE_TIMERS                1             // Software timer support
#define configTIMER_TASK_PRIORITY       2
#define configUSE_STATIC_ALLOCATION     1             // Applications use static allocation
#define configUSE_DYNAMIC_ALLOCATION    1             // heap_3.c (malloc/free wrapper)
#define configCHECK_FOR_STACK_OVERFLOW  2             // Level 2 overflow detection
#define configUSE_MUTEXES               1
#define configUSE_RECURSIVE_MUTEXES     1
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_QUEUE_SETS            1
```

### Task Creation Pattern

All SDK applications use **static task allocation** to avoid runtime heap fragmentation and make memory usage predictable:

```c
// Static memory — declare at file scope
static StaticTask_t xMyTaskPCB;                            // Task control block
static StackType_t  xMyTaskStack[MY_TASK_STACK_SIZE];      // Task stack

// Create the task — no dynamic allocation
TaskHandle_t handle = xTaskCreateStatic(
    MyTask_Function,       // Task entry function
    "MyTask",              // Debug name (not used by kernel)
    MY_TASK_STACK_SIZE,    // Stack size in WORDS (not bytes)
    NULL,                  // pvParameters
    MY_TASK_PRIORITY,      // Priority level
    xMyTaskStack,          // Stack buffer
    &xMyTaskPCB            // TCB buffer
);
GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, handle != NULL);
```

### Standard Task Priorities

| Priority | Value | Usage |
|---|---|---|
| Idle | `tskIDLE_PRIORITY` (0) | FreeRTOS idle task |
| Application Low | `tskIDLE_PRIORITY + 1` | LED blink, logging, UI tasks |
| Application Normal | `tskIDLE_PRIORITY + 2` | Main application tasks |
| Timer | 2 | FreeRTOS software timers |
| Protocol Stacks | 3–5 | BLE, Thread, Zigbee, Matter tasks |
| gpSched | 5 | Qorvo event scheduler |
| Radio RT | Highest | Real-time radio tasks (interrupt context) |

### Inter-Task Communication

**Event Queues** (primary pattern in application code):

```c
// Create queue
static StaticQueue_t xQueueBuffer;
static uint8_t xQueueStorage[10 * sizeof(AppEvent_t)];
QueueHandle_t xAppEventQueue = xQueueCreateStatic(10, sizeof(AppEvent_t),
                                                   xQueueStorage, &xQueueBuffer);

// Send from ISR or other task
AppEvent_t event = { .type = APP_EVENT_BUTTON_PRESSED };
xQueueSendFromISR(xAppEventQueue, &event, NULL);

// Receive in task loop
AppEvent_t event;
if (xQueueReceive(xAppEventQueue, &event, portMAX_DELAY) == pdTRUE)
{
    AppManager_HandleEvent(&event);
}
```

**Other Primitives Used**:
- `SemaphoreHandle_t` / `xSemaphoreCreateBinaryStatic()` — for signaling
- `MutexHandle_t` / `xSemaphoreCreateMutexStatic()` — for shared resource protection
- `TimerHandle_t` / `xTimerCreateStatic()` — for periodic/one-shot callbacks
- `EventGroupHandle_t` / `xEventGroupCreateStatic()` — for multi-event synchronization

### Tickless Idle

With `configUSE_TICKLESS_IDLE = 1`, FreeRTOS suppresses the SysTick interrupt when all tasks are blocked. The Qorvo HAL provides a custom `vPortSuppressTicksAndSleep()` implementation that:
1. Calculates the expected idle time
2. Programs a hardware timer to wake the system
3. Enters the configured sleep mode
4. Restores tick count on wakeup

---

## 9. Hardware Abstraction Layer (HAL)

### HAL Structure

The HAL is split across two main components:

```
Components/Qorvo/HAL_PLATFORM/    — Cortex-M4 platform HAL
Components/Qorvo/HAL_RF/          — RF/radio HAL
```

### HAL_PLATFORM Components

| Component | Purpose |
|---|---|
| `halPlatform` | Top-level platform init, clock configuration |
| `halCortexM4` | ARM Cortex-M4 specific (NVIC, SysTick, FPU enable) |
| `hal_PowerMode` | Sleep state machine and power mode transitions |
| `halWatchdog` | Hardware watchdog configuration |

### Key HAL Macros

```c
HAL_INITIALIZE_GLOBAL_INT();   // Save + disable global interrupts (PRIMASK)
HAL_ENABLE_GLOBAL_INT();       // Restore/enable global interrupts
HAL_DISABLE_GLOBAL_INT();      // Disable global interrupts
HAL_INIT();                    // Full hardware initialization sequence
HAL_WAIT_FOR_EVENT();          // Enter WFE (Wait For Event) sleep
```

### HAL_RF Components

| Component | Purpose |
|---|---|
| `gpPad` | Packet buffer (PD) management for RF data |
| `gpPd` | Packet descriptor layer |
| `gpRadio` | Radio control (channel, TX power, RX enable) |
| `gpRtIl` | Real-time interface layer (interrupt-driven radio) |
| `gpTxMonitor` | TX activity monitoring |

### gpHal API

`gpHal` (General Purpose Hardware Abstraction Layer) provides:

```c
// Flash operations
gpHal_FlashRead(address, length, buffer);
gpHal_FlashProgramSector(address, length, data);  // Word-aligned writes
gpHal_FlashEraseSector(address);

// System
gpHal_Set32kHzCrystalAvailable(bool available);   // Must call before gpBaseComps_StackInit
gpHal_GetChipId(chipId);                           // Read unique device ID

// Interrupt management
gpHal_EnableInterrupt(irqNumber);
gpHal_DisableInterrupt(irqNumber);
```

---

## 10. Peripheral Driver Layer

All peripheral drivers follow a consistent naming convention: `qDrv<PeripheralName>` for the driver, and `qDrv<PeripheralName>.h` for the public API.

### GPIO (`qDrvGPIO.h`)

```c
// Configure pin as output
qDrvGPIO_SetPinDirection(pinNumber, QDRVGPIO_OUTPUT);
qDrvGPIO_SetPinOutput(pinNumber, true);   // Drive HIGH
qDrvGPIO_SetPinOutput(pinNumber, false);  // Drive LOW

// Configure pin as input with interrupt
qDrvGPIO_SetPinDirection(pinNumber, QDRVGPIO_INPUT);
qDrvGPIO_SetPinPullup(pinNumber, QDRVGPIO_PULLUP_ENABLED);
qDrvGPIO_RegisterInterrupt(pinNumber, QDRVGPIO_EDGE_FALLING, myCallback);
qDrvGPIO_EnableInterrupt(pinNumber);
```

### Pin Configuration (`qPinCfg`)

`qPinCfg` is the SDK's board-level pin assignment system. Applications define their GPIO map in `inc/qPinCfg.h`:

```c
// Example qPinCfg.h defines
#define QPINCFG_STATUS_LED     { GPIO_PIN_2, GPIO_PIN_3 }   // LED GPIO list
#define QPINCFG_BUTTON_1       GPIO_PIN_10                   // Button 1
#define QPINCFG_UART_TX        GPIO_PIN_6
#define QPINCFG_UART_RX        GPIO_PIN_7
```

The `qPinCfg_Init(NULL)` call applies these assignments to the hardware.

### UART (`qDrvUART.h`)

```c
// Default configuration: 115200 baud, 8N1
qDrvUART_Config_t uartConfig = {
    .baudRate   = 115200,
    .dataBits   = QDRVUART_DATABITS_8,
    .stopBits   = QDRVUART_STOPBITS_1,
    .parity     = QDRVUART_PARITY_NONE,
    .flowControl = QDRVUART_FLOWCONTROL_NONE,
    .useDMA     = true,   // DMA-driven TX (required to avoid hang)
};
qDrvUART_Init(UART0, &uartConfig, rxBuffer, RX_BUF_SIZE);
qDrvUART_Write(UART0, txData, txLength);
```

**Important**: In release builds, UART TX must use DMA mode (`useDMA = true`) to avoid a known hang under high-frequency transmission. (Fixed in SDK 2.0.1 via `qRegUART_IRQUnmaskedTxNotBusyGet`.)

### SPI Master (`qDrvSPIM.h`)

```c
qDrvSPIM_Config_t spiConfig = {
    .clockFrequency = 4000000,              // 4 MHz
    .clockPhase     = QDRVSPIM_CPHA_0,
    .clockPolarity  = QDRVSPIM_CPOL_0,
    .bitOrder       = QDRVSPIM_MSB_FIRST,
};
qDrvSPIM_Init(SPI0, &spiConfig);
qDrvSPIM_Transfer(SPI0, txBuf, rxBuf, length);
```

### I2C Master (`qDrvTWIM.h`)

```c
qDrvTWIM_Config_t i2cConfig = {
    .clockFrequency = QDRVTWIM_FREQ_400K,   // 400 kHz fast mode
};
qDrvTWIM_Init(TWI0, &i2cConfig);
qDrvTWIM_Write(TWI0, deviceAddr, regAddr, data, length);
qDrvTWIM_Read(TWI0, deviceAddr, regAddr, buffer, length);
```

### ADC — General Purpose (`qDrvGPADC.h`)

```c
qDrvGPADC_Config_t adcConfig = {
    .reference  = QDRVGPADC_REF_VDD,
    .resolution = QDRVGPADC_RES_12BIT,
};
qDrvGPADC_Init(&adcConfig);
uint16_t rawValue = qDrvGPADC_Sample(QDRVGPADC_INPUT_AIN0);
uint32_t millivolts = qDrvGPADC_ConvertToMillivolt(rawValue);
```

### ADC — High Resolution (`qDrvHRADC.h`)

Higher-precision ADC for sensitive measurements (temperature, battery, etc.).

### PWM (`qDrvPWMXL.h`)

```c
qDrvPWMXL_Config_t pwmConfig = {
    .frequency  = 1000,    // 1 kHz
    .resolution = 256,     // 8-bit duty cycle
};
qDrvPWMXL_Init(PWM0, &pwmConfig);
qDrvPWMXL_SetDutyCycle(PWM0, 128);   // 50% duty cycle
qDrvPWMXL_Enable(PWM0, true);
```

### Battery Monitor (`gpBatteryMonitor`)

```c
gpBatteryMonitor_Init();
uint16_t batteryMv = gpBatteryMonitor_GetBatteryVoltage_mV();
uint8_t  batteryPct = gpBatteryMonitor_GetBatteryPercentage();
```

### StatusLed (Application-Level Abstraction)

`StatusLed` is a simple application-level LED driver found in `Applications/shared/`:

```c
// Initialize with list of LED GPIO pins
const uint8_t leds[] = QPINCFG_STATUS_LED;
StatusLed_Init(leds, Q_ARRAY_SIZE(leds), true);   // last param = active high

// Control LEDs
StatusLed_SetLed(GREEN_LED_GPIO_PIN, true);    // Turn on
StatusLed_SetLed(RED_LED_GPIO_PIN, false);     // Turn off
StatusLed_BlinkLed(GREEN_LED_GPIO_PIN, 500);   // Blink every 500ms
```

### DMA (`qDrvDMA.h`)

Direct memory access for UART, SPI, I2S transfers — reduces CPU load for high-throughput operations.

### I2S Audio (`qDrvI2S.h`)

Used in the `MicTest` and `SpeakerTest` peripheral examples for audio streaming.

### Watchdog (`qDrvWatchdog.h`)

```c
qDrvWatchdog_Init(WATCHDOG_TIMEOUT_MS);   // Initialize with timeout in ms
qDrvWatchdog_Enable();
// Application must call periodically:
qDrvWatchdog_Kick();                       // Reset watchdog timer
```

---

## 11. Bluetooth Low Energy (BLE) Stack

### Architecture Overview

```
Application Layer (C/C++)
        │
        ▼
ARM Cordio BLE Host Stack           (Components/ThirdParty/BleHost/)
  ├── ATT (Attribute Protocol)
  ├── GATT (Generic Attribute Profile)
  ├── GAP (Generic Access Profile)
  ├── SMP (Security Manager Protocol)
  └── L2CAP (Logical Link Control)
        │
        ▼  (HCI — Host Controller Interface)
        │
Qorvo BLE Controller                (Components/Qorvo/BleController/)
  ├── gpHci         — HCI transport layer
  ├── gpBle         — Core BLE functionality
  ├── gpBleAdvertiser — Advertisement management
  ├── gpBleConnectionManager — Connection lifecycle
  ├── gpBleInitiator — Central/Initiator role
  ├── gpBleDataRx   — RX data path
  ├── gpBleDataTx   — TX data path
  ├── gpBleLlcp     — Link Layer Control Protocol
  ├── gpBleLlcpProcedures — Parameter updates, encryption setup
  ├── gpBleChannelMapManager — Adaptive frequency hopping
  ├── gpBleAddressResolver — Resolvable Private Addresses
  ├── gpBleActivityManager — Scheduling / coexistence
  └── gpECC         — Elliptic Curve Cryptography (ECDH)
        │
        ▼
HAL_RF / Radio Hardware
```

### BLE Controller Key Configuration (`gpBleConfig.h`)

```c
#define GP_BLE_NR_OF_CONNECTIONS           1     // Max simultaneous connections
#define GP_BLE_FILTER_ACCEPT_LIST_SIZE     3     // Accept list entries
#define GP_BLE_HCI_RX_BUFFER_SIZE         255    // HCI RX buffer (bytes)
#define GP_BLE_HCI_TX_BUFFER_SIZE         255    // HCI TX buffer (bytes)
#define GP_BLE_PREAMBLE_DETECTION_THRESHOLD 150  // Optimized for RPi4 interop
```

### BLE Specification Support

- **BLE Version**: 5.4
- **Declaration ID**: D068807
- **QDID**: 203579
- Extended Advertising
- Periodic Advertising
- LE Audio foundations
- LLCP procedures: Connection Parameter Update, Encryption, PHY Update, Data Length Extension

### BLE Application Flow (Peripheral Role)

```
Application_Init()
  └── BleInit()
        ├── WsfOsInit()                 // Initialize Cordio OS
        ├── DmDevReset()                // Initialize DM (Device Manager)
        ├── AttServerInit()             // Initialize ATT server
        ├── GattInit()                  // Initialize GATT
        ├── RegisterGattServices()      // Register application GATT services
        └── BleStart()
              └── DmAdvStart(...)       // Start advertising

// Advertising callback registered via DmRegister():
static void BleHandleEvent(dmEvt_t* pMsg)
{
    switch(pMsg->hdr.event)
    {
        case DM_CONN_OPEN_IND:    // Connection established
        case DM_CONN_CLOSE_IND:   // Disconnected
        case ATTS_HANDLE_VALUE_CNF:  // Notification confirmed
    }
}
```

### BLE GATT Service Definition

```c
// Service UUID declaration
static const uint8_t svcUuid[] = { 0x..., ... };  // 128-bit custom UUID

// Characteristic definition
static const attsAttr_t svcAttrList[] = {
    { attPrimSvcUuid, (uint8_t*)svcUuid, &svcLen, sizeof(svcUuid),
      ATTS_PERMIT_READ, 0 },
    { attChUuid, (uint8_t*)myChValue, &myChLen, MY_MAX_LEN,
      ATTS_PERMIT_READ | ATTS_PERMIT_WRITE, ATTS_SET_WRITE_CBACK },
    // ... more attributes
};
```

---

## 12. Thread / OpenThread Stack

### Architecture Overview

```
Application (C/C++)
        │
        ▼  OpenThread C API (otXxx functions)
OpenThread Stack               (Components/ThirdParty/OpenThread/)
  ├── Network layer (IPv6, routing)
  ├── MLE (Mesh Link Establishment)
  ├── CoAP / DNS / SRP
  └── 802.15.4 MAC Interface
        │
        ▼  Qorvo Platform Adaptation Layer
qvOT Platform Abstraction       (Components/Qorvo/OpenThread/qvOT/)
  ├── platform_qorvo.h  — Platform init
  ├── radio_qorvo.h     — Radio (TX/RX, energy scan, channel)
  ├── alarm_qorvo.h     — Millisecond/microsecond timers
  ├── uart_qorvo.h      — CLI UART interface
  ├── random_qorvo.h    — Hardware RNG
  └── settings_qorvo.h  — NVM-backed settings storage
        │
        ▼
Qorvo 802.15.4 MAC + Radio Hardware
```

### Thread Configuration

| Parameter | Value |
|---|---|
| Thread Version | 1.4 (updated in SDK 2.0.1) |
| Device Roles | MTD (Minimal Thread Device) and FTD (Full Thread Device) |
| Certifications | Thread 1.3 MTD and FTD+MTD (Apr 2025); Thread 1.4 in progress |
| Security | DTLS 1.3 (TLS 1.3 via mbedTLS for commissioning) |
| Features | CoAP, CoAP blockwise, CoAP observe, DNS-SD, SRP client |

### Thread Stack Initialization (in application)

```c
// Typically called from Matter or Thread task
void Thread_Init(void)
{
    otInstance* instance = otInstanceInitSingle();  // Create OT instance
    
    // Configure dataset
    otOperationalDataset dataset;
    otDatasetCreateNewNetwork(instance, &dataset);
    
    // Set network key, PAN ID, channel
    otDatasetSetActive(instance, &dataset);
    
    // Start Thread
    otThreadSetEnabled(instance, true);
    otIp6SetEnabled(instance, true);
}
```

### NVM-Backed Settings

Thread network credentials (network key, PAN ID, extended PAN ID, active timestamp) are stored in Qorvo's NVM via the `settings_qorvo` platform adaptation:

```c
// Platform abstraction used internally by OpenThread
otError platformSettingsGet(otInstance*, uint16_t key, int index,
                             uint8_t* value, uint16_t* length);
otError platformSettingsSet(otInstance*, uint16_t key,
                             const uint8_t* value, uint16_t length);
otError platformSettingsDelete(otInstance*, uint16_t key, int index);
```

---

## 13. Zigbee Stack

### Architecture Overview

```
Application (C/C++)  — Zigbee endpoint handlers
        │
        ▼
Qorvo Zigbee Application Layer     (Components/Qorvo/Zigbee/)
  ├── Cluster handlers (OnOff, Level, Color, Occupancy, ...)
  ├── Zigbee endpoint registration
  └── Touchlink commissioning support
        │
        ▼
Third-Party Zigbee Stack            (Components/ThirdParty/Zigbee/)
  ├── Application Support Sublayer (APS)
  ├── Network Layer (NWK)
  ├── Security
  └── Zigbee Cluster Library (ZCL)
        │
        ▼
Qorvo 802.15.4 MAC Dispatcher      (Components/Qorvo/802_15_4/)
  ├── gpMacCore       — IEEE 802.15.4 MAC core
  ├── gpMacDispatcher — Shares MAC between Thread/Zigbee
  └── gpRxArbiter     — Routes received frames to correct stack
```

### Zigbee Certification

| Certification | Date | Level |
|---|---|---|
| Zigbee R23 Platform Certification | Oct 11, 2024 | Platform |

### Zigbee Application Endpoint

```c
// Register a Zigbee endpoint (e.g., OnOff Light)
zigbeeEndpointConfig_t endpointConfig = {
    .endpoint = 1,
    .profileId = ZB_PROFILE_HA,
    .deviceId  = ZB_DEVICE_COLOR_DIMMABLE_LIGHT,
};
ZigbeeEndpoint_Init(&endpointConfig);

// Handle cluster attribute callbacks
void zclOnOffClusterCallback(uint8_t endpoint, bool onOff)
{
    // Update hardware (LED, etc.)
    StatusLed_SetLed(LED_GPIO, onOff);
}
```

### ZAP (Zigbee/Matter Attribute Parser)

The SDK uses ZAP for both Zigbee and Matter cluster definitions. ZAP processes `.zap` files to generate:
- Attribute storage arrays
- Cluster handler stubs
- ZCL command dispatch tables

Located in `Tools/Zap/`. The generated output goes into `Applications/<App>/gen/`.

### Touchlink Commissioning

The SDK includes `ColorLightApplicationTouchLink` for Zigbee touchlink support. Touchlink allows proximity-based commissioning without a coordinator.

**Important Bug Fix (SDK 2.0.1)**: After touchlink timeout, the device now properly resets NLME and APSME to allow sleep entry. Prior versions left these in an active state, preventing sleep mode.

---

## 14. Matter (CHIP) Stack

### Architecture Overview

```
Application (C++)
  └── AppManager.cpp — Application logic, attribute callbacks
        │
        ▼
Matter SDK (CHIP)              (Components/ThirdParty/Matter/)
  ├── Server / Commissioning
  ├── Data Model (clusters, attributes, commands)
  ├── Interaction Model (reads, writes, subscriptions)
  ├── Secure Channel (CASE, PASE, SigmaR1/R2)
  ├── Discovery (mDNS, DNS-SD)
  └── Transport (UDP/IPv6 over Thread, BLE for commissioning)
        │
        ▼
Qorvo Matter Glue              (Libraries/Qorvo/MatterQorvoGlue/)
  ├── Platform adaptation for QPG6200
  ├── NVM integration for fabric + credential storage
  └── OTA requestor integration
        │
        ▼
OpenThread (for IPv6 transport)
BLE Host (for BLE commissioning)
```

### Matter Certifications

| Certification | Matter Version | Date |
|---|---|---|
| Combo Switch | Matter 1.3 | Sep 17, 2024 |
| Thermostatic Radiator Valve | Matter 1.4.0 | Aug 22, 2025 |

### Matter Stack Initialization

```cpp
// In MatterTask.cpp (runs on dedicated FreeRTOS task)
void MatterTask_Init(void)
{
    // Initialize Matter platform layer
    chip::Platform::MemoryInit();
    chip::DeviceLayer::PlatformMgr().InitChipStack();
    
    // Configure Thread as transport
    chip::DeviceLayer::ThreadStackMgr().InitThreadStack();
    chip::DeviceLayer::ConnectivityMgr().SetThreadDeviceType(
        chip::DeviceLayer::ConnectivityManager::kThreadDeviceType_Router);
    
    // Start Matter server
    chip::Server::GetInstance().Init();
    
    // Set device attestation credentials
    chip::Credentials::SetDeviceAttestationCredentialsProvider(
        chip::Credentials::Examples::GetExampleDACProvider());
    
    // Start stack (blocking — runs Matter event loop)
    chip::DeviceLayer::PlatformMgr().RunEventLoop();
}
```

### Matter Commissioning Flow

1. Device starts advertising over BLE (using Qorvo BLE stack)
2. Commissioner scans and connects over BLE
3. PASE (Password Authenticated Session Establishment) handshake using passcode
4. Operational credentials are provisioned (certificate chain)
5. Thread network dataset is delivered over secure BLE channel
6. Device joins Thread network
7. CASE session established over Thread/IPv6
8. Matter fabric is joined — device now controllable via Matter

### Matter Cluster Implementation

```cpp
// In AppManager.cpp — attribute write callback
void MatterPostAttributeChangeCallback(
    const chip::app::ConcreteAttributePath& attributePath,
    uint8_t type,
    uint16_t size,
    uint8_t* value)
{
    using namespace chip::app::Clusters;
    
    if (attributePath.mClusterId == OnOff::Id)
    {
        if (attributePath.mAttributeId == OnOff::Attributes::OnOff::Id)
        {
            bool onOff = *value != 0;
            LightingManager::GetInstance().SetOnOff(onOff);
        }
    }
}
```

---

## 15. ConcurrentConnect — Multi-Protocol Operation

### What It Is

ConcurrentConnect is Qorvo's patented technology for **true simultaneous multiprotocol operation**. Unlike software time-division approaches, QPG6200 uses hardware-level MAC scheduling to run BLE, Thread (IEEE 802.15.4), and Zigbee (IEEE 802.15.4) concurrently on the same 2.4 GHz radio — without starvation or dropped packets on any protocol.

### Enabling Components

| Component | Purpose |
|---|---|
| `gpMacDispatcher` | Routes outgoing frames to the correct protocol's MAC |
| `gpRxArbiter` | Routes incoming 802.15.4 frames between Thread and Zigbee |
| `gpBleActivityManager` | Schedules BLE events around 802.15.4 windows |
| `gpMacCore` | Shared 802.15.4 MAC core |

### Flagship Application: Concurrent Light

`Applications/Concurrent/Light/` implements a light endpoint that simultaneously:
- Runs as a **Matter** light (controllable via Google Home, Apple Home, SmartThings)
- Runs as a **Zigbee** color light (controllable via Zigbee coordinator/gateway)
- Accepts **BLE** connections for Matter commissioning and diagnostics

All three protocol stacks run concurrently on the same device/radio with no manual switching.

### Protocol Stack Co-existence Architecture

```
Applications/Concurrent/Light/src/
  ├── main.c           — Single main(), starts all stacks
  ├── AppTask.cpp      — Central event queue, distributes events
  ├── AppManager.cpp   — Application logic (light state machine)
  ├── MatterTask.cpp   — Matter event loop (dedicated FreeRTOS task)
  ├── ZigbeeTask.cpp   — Zigbee event loop (dedicated FreeRTOS task)
  └── AppButtons.cpp   — Physical button → event queue
```

### Radio Sharing Behavior

- **BLE** uses dedicated BLE controller time slots
- **Thread and Zigbee** share the 802.15.4 MAC via `gpMacDispatcher`
- `gpRxArbiter` examines each received 802.15.4 frame's PAN ID and routing key to determine whether it belongs to Thread or Zigbee
- The `gpBleActivityManager` (v2.0.1 fix: assert 156) checks context allocation before updating activity entries to prevent race conditions in noisy RF environments

---

## 16. Application Architecture Patterns

### Standard Multi-Protocol Application Structure

```
inc/
  AppEvent.h         — Application event type definitions
  AppTask.h          — Task + queue declarations
  AppManager.h       — Application logic interface
  AppButtons.h       — Button event declarations
  StatusLed.h        — LED control interface
  MatterTask.h       — Matter task interface
  ZigbeeEndpoint.h   — Zigbee endpoint interface
  qPinCfg.h          — Board-level GPIO pin assignments
  CHIPProjectConfig.h — Matter configuration overrides

src/
  AppTask.cpp        — FreeRTOS task, event queue, dispatcher
  AppManager.cpp     — State machine, attribute/event handling
  AppButtons.cpp     — GPIO interrupt → queue post
  MatterTask.cpp     — Matter stack init + event loop
  ZigbeeTask.cpp     — Zigbee stack init + event processing

gen/qpg6200/
  qorvo_config.h     — Generated feature flags
  <App>_qpg6200.ld   — Generated linker script
  gpHal_RtSystem.c   — Generated RT system config
```

### Event-Driven Pattern

```cpp
// AppEvent.h
typedef enum {
    APP_EVENT_BUTTON_1_SHORT_PRESS,
    APP_EVENT_BUTTON_2_SHORT_PRESS,
    APP_EVENT_BUTTON_FACTORY_RESET,
    APP_EVENT_MATTER_CONNECTED,
    APP_EVENT_ZIGBEE_JOINED,
    APP_EVENT_OTA_START,
} AppEventType_t;

typedef struct {
    AppEventType_t type;
    union {
        bool       onOff;
        uint8_t    level;
        uint32_t   color;
    } data;
} AppEvent_t;

// AppTask.cpp — Central dispatcher
void AppTask_EventLoop(void* pvParameters)
{
    AppEvent_t event;
    while (true)
    {
        if (xQueueReceive(xAppEventQueue, &event, portMAX_DELAY) == pdTRUE)
        {
            AppManager_HandleEvent(&event);
        }
    }
}
```

### AppButtons Module

Physical buttons generate hardware GPIO interrupts → debounced → converted to `AppEvent_t` → posted to `xAppEventQueue`. No button logic lives in interrupt context.

### ResetCount Module

Implements factory reset via counting rapid button presses:

```c
// Hold button for N seconds → trigger factory reset
ResetCount_Init(APP_FACTORY_RESET_BUTTON, FACTORY_RESET_TIMEOUT_MS);
// Registers a callback when the threshold is reached
```

### State LED Module

Encodes device state as LED blinking patterns:

| State | LED Pattern |
|---|---|
| Unprovisioned | Slow blink (500ms) |
| Commissioning | Fast blink (100ms) |
| Network joined | Solid ON |
| OTA in progress | Double-blink |
| Factory reset | Fast red blink |

---

## 17. Example Applications

### HelloWorld

**Path**: `Applications/HelloWorld/`  
**Purpose**: Minimal starter project — demonstrates FreeRTOS task creation, LED blinking, and UART logging.

**What it does**:
- Creates two FreeRTOS tasks:
  - `ledToggle_Task`: Blinks green LED every 500 ms
  - `helloWorld_Task`: Prints "Hello world" via UART every 1000 ms

**Key files**: `src/main.c`

---

### BLE Peripheral

**Path**: `Applications/Ble/Peripheral/`  
**Purpose**: Demonstrates BLE peripheral role with a custom GATT service for LED control.

---

### BLE Central

**Path**: `Applications/Ble/Central/`  
**Purpose**: Demonstrates BLE central role — scans and connects to BLE peripherals.

---

### BLE IoT Demo

**Path**: `Applications/Ble/BleIoTDemo/`  
**Purpose**: Full IoT demo over BLE — sensor data streaming to a central gateway.

---

### ThreadBleDoorbell

**Path**: `Applications/Ble/ThreadBleDoorbell/`  
**Purpose**: Doorbell system combining Thread for IP connectivity and BLE for local notifications.

**Architecture**:
- Thread: Sends doorbell events to a Thread border router (and onward to cloud/app)
- BLE: Direct push notification to a nearby smartphone

---

### ThreadBleMotionDetector (MaxSonar)

**Path**: `Applications/Ble/ThreadBleMotionDetector_MaxSonar/`  
**Purpose**: Ultrasonic motion/distance detection using MaxSonar sensor over UART, reporting via Thread + BLE.

---

### Concurrent Light (Flagship)

**Path**: `Applications/Concurrent/Light/`  
**Purpose**: Production-grade reference for Matter + Zigbee + BLE concurrent operation.

**Features**:
- OnOff, Level Control, Color Control (Matter + Zigbee clusters)
- Matter commissioning via BLE
- Zigbee touchlink commissioning
- Factory reset via 5-press button sequence
- OTA update support (Matter OTA Requestor)
- Finding and Binding target (Zigbee)

---

### Matter Applications

| Path | Endpoint Type |
|---|---|
| `Applications/Matter/Light/` | Dimmable/Color Light |
| `Applications/Matter/Switch/` | Combo Switch |
| `Applications/Matter/MotionSensor/` | Occupancy Sensor |
| `Applications/Matter/DoorWindow/` | Door/Window Sensor |

---

### Zigbee Applications

| Path | Endpoint Type |
|---|---|
| `Applications/Zigbee/Light/` | Color Dimmable Light |
| `Applications/Zigbee/Switch/` | On/Off Switch |
| `Applications/Zigbee/MotionSensor/` | Occupancy Sensor |
| `Applications/Zigbee/DoorWindow/` | IAS Zone (Contact Sensor) |

---

### Peripheral Examples

| Path | Demonstrates |
|---|---|
| `Applications/Peripherals/Batterymonitor/` | Battery voltage + percentage via GPADC |
| `Applications/Peripherals/Gpadc/` | General Purpose ADC raw sampling |
| `Applications/Peripherals/Hradc/` | High-Resolution ADC |
| `Applications/Peripherals/Uart/` | UART TX/RX with DMA |
| `Applications/Peripherals/Mtwi/` | I2C master read/write |
| `Applications/Peripherals/MicTest/` | I2S microphone input |
| `Applications/Peripherals/SpeakerTest/` | I2S audio output |

---

### PTC (Product Test Component)

**Path**: `Applications/PTC/`  
**Purpose**: RF production testing — enables continuous TX, RX sensitivity tests, RSSI measurement. Required for FCC/CE compliance testing.

---

### Sleep Application

**Path**: `Applications/Sleep/`  
**Purpose**: Demonstrates power management modes — deep sleep, wake on GPIO, sleep current measurement.

---

### Bootloader

**Path**: `Applications/Bootloader/`  
**Purpose**: Application-level bootloader with OTA support. Two modes:
- **Single-mode**: Always loads one application from fixed address
- **Dual-mode**: Manages two application slots, enables fail-safe OTA

---

## 18. Configuration System

### Three-Layer Configuration

#### Layer 1: Python Configuration File (`QPG6200DK.py`)

Each application's build configuration is defined in a Python file. This generates the Makefile and all generated header files. Parameters include:
- Component selection (which Qorvo and third-party components to include)
- GPIO pin mapping (`qPinCfg`)
- Memory allocation sizes
- Feature flags (enable/disable protocols, features)
- Build type (debug/release/production)

#### Layer 2: Generated `qorvo_config.h`

Auto-generated per application, per target. Contains `#define` flags for:

```c
// Component enables
#define GP_DIVERSITY_BLE                    1
#define GP_DIVERSITY_THREAD                 1
#define GP_DIVERSITY_ZIGBEE                 1

// Memory / buffer sizes
#define GP_BLE_NR_OF_CONNECTIONS            1
#define GP_SCHED_EVENT_LIST_SIZE            92    // Increased in 2.0.1
#define GPHAL_RT_UNTIMED_TIMED_ENTRIES_MAX  GP_HAL_PBM_TYPE1_AMOUNT

// Feature flags
#define GP_APP_DIVERSITY_FINDING_AND_BINDING_TARGET  1
#define GP_APP_DIVERSITY_OTA_REQUESTOR               1
```

#### Layer 3: Application `CHIPProjectConfig.h` (Matter apps)

Overrides defaults from the Matter SDK for the specific application:

```cpp
#define CHIP_DEVICE_CONFIG_DEVICE_VENDOR_ID     0x10D0   // Qorvo vendor ID
#define CHIP_DEVICE_CONFIG_DEVICE_PRODUCT_ID    0x0001
#define CHIP_DEVICE_CONFIG_DEVICE_HARDWARE_VERSION 1
#define CHIP_DEVICE_CONFIG_DEFAULT_SIO_LOG_LEVEL 1       // Log level
#define CONFIG_ENABLE_EXTENDED_DISCOVERY        1
```

### Build-Time Feature Flags Summary

| Flag | Purpose |
|---|---|
| `GP_DIVERSITY_BLE` | Enable BLE stack |
| `GP_DIVERSITY_THREAD` | Enable Thread/OpenThread |
| `GP_DIVERSITY_ZIGBEE` | Enable Zigbee stack |
| `GP_DIVERSITY_MATTER` | Enable Matter stack |
| `GP_APP_DIVERSITY_OTA_REQUESTOR` | Enable Matter OTA updates |
| `GP_APP_DIVERSITY_FINDING_AND_BINDING_TARGET` | Enable Zigbee F&B |
| `GP_DIVERSITY_LOG` | Enable debug logging |
| `GP_DIVERSITY_ASSERT` | Enable assert checks |
| `GP_DIVERSITY_SECURE_BOOT` | Enable secure boot verification |

---

## 19. Non-Volatile Memory (NVM)

### NVM Architecture

The SDK uses a key-value NVM system backed by on-chip flash with wear-leveling and error recovery:

```
gpNvm API (application-facing)
        │
        ▼
gpNvm_RW_Kx_SubpagedFlash_v2  — Sub-paged flash driver with defragmentation
        │
        ▼
gpHal_FlashProgramSector / gpHal_FlashEraseSector  — Raw flash access
```

### NVM API

```c
// Register NVM element
gpNvm_RegisterElements(elementsArray, numElements);

// Read from NVM
gpNvm_Result_t result = gpNvm_ReadNext(token, &length, data);

// Write to NVM
result = gpNvm_Write(token, length, data);

// Backup/Restore (used during OTA)
gpNvm_Backup();
gpNvm_Restore();
```

### Protected NVM API (v2.0.1+)

To prevent Assert 1155 during concurrent NVM access, the SDK now requires **protected NVM API** calls:

```c
// Use protected versions (acquire mutex internally):
gpNvm_BuildLookup_Protected();
gpNvm_ReadNext_Protected(token, &length, data);
gpNvm_Write_Protected(token, length, data);
```

### NVM Encryption (v2.0.1+)

Sensitive NVM data (credentials, keys) is now encrypted using **AES-GCM** with a hardware-stored 16-byte key:
- The encryption key never leaves the Secure Element in plaintext
- Provides transition path from unencrypted legacy NVM
- Tamper detection: failed decryption triggers secure erasure

### NVM Regions

| Region | Purpose |
|---|---|
| Factory Block | Manufacturer-set: device certificates, attestation |
| Factory Data | Device-specific: serial number, calibration |
| Persistent Storage Block | Runtime: Matter fabrics, Thread credentials, app state |
| OTA Block | Pending OTA image metadata |

---

## 20. Security Architecture

### Chain of Trust

```
Qorvo Root of Trust (in ROM)
        │  ECDSA P-256 verification
        ▼
Secure Element Firmware (SEFW 4.0.12)
        │  ECDSA P-256 verification
        ▼
Application Bootloader
        │  ECDSA P-256 verification
        ▼
Application Firmware
```

Every stage verifies the digital signature of the next stage before granting execution.

### Secure Element Features

- **PSA Certified Level 2** (certified May 21, 2025)
- AES hardware acceleration (offloads ~10.5 KB of flash from mbedTLS)
- ECDSA P-256: key generation, signing, verification
- SHA-256 hardware hash
- True Random Number Generator (TRNG)
- Secure key storage (keys stored in tamper-resistant element)
- Secure Debug: JTAG can be locked until unlocked with a challenge-response

### Secure Boot Verification

```c
// esec_config.h (generated per application)
#define ESEC_MANUFACTURER_PUBLIC_KEY  { /* 64-byte P-256 public key */ }
#define ESEC_SIGNATURE_ALGORITHM      ESEC_SIG_ALGO_ECDSA_P256_SHA256

// Bootloader verifies application using:
// 1. SHA-256 hash of application image
// 2. ECDSA P-256 signature verification against manufacturer public key
```

### NVM Security

- Credential storage uses AES-GCM encryption
- Matter operational certificates stored in `PersistentStorageBlock`
- Thread network keys stored encrypted in NVM
- Factory certificates stored in `FactoryBlock` (locked at manufacturing)

### Mutex Protection

Several critical sections are protected by mutexes (v2.0.1 fixes):
- Flash write operations: mutex prevents concurrent Secure Element access
- NVM defragmentation: critical section prevents Assert 764
- BLE activity manager: checks context allocation before update (Assert 156 fix)

---

## 21. Bootloader & OTA Updates

### Bootloader Modes

**Single-Image Bootloader**:
- Simple: always boots from fixed application address
- Validates signature before jumping
- No dual-slot support

**Dual-Image Bootloader (recommended for production)**:
- Slot A: Primary application
- Slot B: OTA download target
- On boot: validates both slots, boots highest valid version
- On failed slot B: falls back to slot A
- On failed slot A: can attempt slot B as emergency recovery

### OTA Image Format

1. Application is compiled to ELF/HEX
2. Post-build script compresses with **LZMA** compression
3. OTA package is created with:
   - Qorvo OTA header (magic, version, size, hash)
   - ECDSA P-256 signature
   - LZMA-compressed payload
4. Matter OTA Requestor downloads and validates the package

### OTA via Matter

```cpp
// Matter OTA Requestor is automatically enabled with GP_APP_DIVERSITY_OTA_REQUESTOR
// It handles:
// 1. Querying Matter OTA Provider nodes
// 2. Downloading image over BDX (Bulk Data Transfer) protocol
// 3. Validating image signature
// 4. Writing to flash slot B
// 5. Signaling bootloader to switch slots on next reboot
```

### Bootloader NVM Fix (v2.0.1)

The `gpUpgrade_StoreWakeUpAddress` function was fixed to use `gpHal_FlashProgramSector` (word-addressed) instead of `gpHal_FlashWrite` (byte-addressed). The incorrect length parameter was causing NVM corruption in release builds.

---

## 22. Power Management

### Sleep Modes

| Mode | Wake Sources | Current | Use Case |
|---|---|---|---|
| Active | N/A | ~10 mA (TX) | Radio active |
| Idle (WFE) | Any interrupt | ~1 mA | CPU idle, radio on |
| Tickless Idle | Timer, GPIO | ~100 µA | FreeRTOS idle, radio off |
| Deep Sleep | GPIO only | ~10 µA | Long sleep periods |

### FreeRTOS Tickless Idle

Enabled via `configUSE_TICKLESS_IDLE = 1`. The Qorvo port hooks into `vPortSuppressTicksAndSleep()`:

```c
void vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime)
{
    // Convert FreeRTOS ticks to hardware timer units
    uint32_t sleepMs = xExpectedIdleTime * portTICK_PERIOD_MS;
    
    // Program wake-up timer
    hal_SleepSetWakeUpTime(sleepMs);
    
    // Enter sleep mode
    hal_SleepStart();
    
    // After wake: compensate tick count
    vTaskStepTick(actualSleepTicks);
}
```

### Sleep Application

`Applications/Sleep/` demonstrates:
1. Configuring GPIO wakeup pins
2. Transitioning between sleep modes
3. Measuring sleep current via Battery Monitor
4. Verifying correct crystal configuration (`gpHal_Set32kHzCrystalAvailable`)

### 32 kHz Crystal Configuration

**Critical**: `gpHal_Set32kHzCrystalAvailable()` must be called **before** `gpBaseComps_StackInit()`. Incorrect ordering causes Assert 521 (hal_ES) when entering sleep mode.

```c
void Application_Init(void)
{
    gpHal_Set32kHzCrystalAvailable(false);  // MUST be first — no 32kHz crystal on DK
    gpBaseComps_StackInit();                 // Then initialize stack
    // ...
}
```

---

## 23. Logging & Debugging

### gpLog Logging System

```c
#include "gpLog.h"

// Set component ID for log filtering
#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

// Log a formatted string (printf-style, limited format support)
GP_LOG_SYSTEM_PRINTF("Sensor value: %d mV", 0, millivolts);

// Log with different severity levels
GP_LOG_PRINTF("Debug info", 0);
```

**Note**: `GP_LOG_SYSTEM_PRINTF` uses a compact format — the second parameter is a bitmask for format specifiers, not a standard variadic. Follow existing patterns in the codebase.

### Assert System

```c
#include "gpAssert.h"

// Assert with system-level severity (triggers reset on failure in release)
GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_SYSTEM, condition);

// Assert with component-level severity
GP_ASSERT(GP_DIVERSITY_ASSERT_LEVEL_COMP, condition);

// Qorvo-style assert (shorthand)
Q_ASSERT(condition);
```

### UART Debug Output

All `GP_LOG_SYSTEM_PRINTF` output routes to UART (115200 8N1 by default). Connect a USB-to-UART adapter to the DK's debug UART pins.

### J-Link Debugging

- SWD interface for GDB debugging
- Real-time J-Link RTT for log output without UART
- J-Link Ozone for graphical debugging

### Component ID System

Each component registers a numeric ID (`GP_COMPONENT_ID`) used to:
- Filter log output per component
- Tag assert messages with origin
- Route gpSched events

---

## 24. Factory Data & Provisioning

### Factory Block

Written at manufacturing time — immutable in production:

| Data | Purpose |
|---|---|
| Device Attestation Certificate (DAC) | Matter device identity |
| Product Attestation Intermediate (PAI) | Certificate chain |
| Certificate Declaration | Matter product category |
| Passcode / Discriminator | Matter commissioning |
| Device Serial Number | Unique device ID |
| RF Calibration Data | Per-unit radio tuning |

### Factory Data Tools

`Tools/FactoryData/` — Scripts to:
1. Generate device certificates (for development/test)
2. Program factory data to flash
3. Verify factory data integrity

### Qorvo Platform Tools

`Tools/QorvoPlatformTools/` — Production tools:
- Program factory data to secure element
- Set up secure boot keys
- Verify SEFW version
- Configure antenna diversity

### QR Code / Manual Pairing Code

For Matter commissioning, each device has a QR code and manual pairing code derived from:
- Passcode (randomly generated per device)
- Discriminator (randomly generated per device)
- VID/PID
- Setup PIN flags

---

## 25. Certifications

| Certification | Version | Date | Authority |
|---|---|---|---|
| Bluetooth LE | 5.4 | Jun 12, 2024 | Bluetooth SIG (QDID: 203579) |
| Bluetooth LE Design Reassessed | 5.4 | Dec 19, 2024 | Bluetooth SIG (DN: Q338888) |
| Thread MTD | 1.3 | Apr 28, 2025 | Thread Group |
| Thread FTD+MTD | 1.3 | Apr 28, 2025 | Thread Group |
| Zigbee Platform | R23 | Oct 11, 2024 | CSA (csa-iot.org) |
| Matter Combo Switch | 1.3 | Sep 17, 2024 | CSA |
| Matter TRV | 1.4.0 | Aug 22, 2025 | CSA |
| PSA Security | Level 2 | May 21, 2025 | TrustCB |

---

## 26. Quick-Start Walkthrough

### Step 1: Environment Setup

```bash
# Clone repository (with LFS for binaries)
git lfs install
git clone <repo-url> qpg6200-iot-sdk
cd qpg6200-iot-sdk

# Initialize git submodules (OpenThread, Matter, FreeRTOS, etc.)
./Scripts/setup_submodules.sh

# Set up toolchain and Python dependencies
./Scripts/bootstrap.sh

# OR: open in VS Code with DevContainer for automatic setup
code .  # Then: "Reopen in Container"
```

### Step 2: Build HelloWorld

```bash
cd Applications/HelloWorld
make -f Makefile.HelloWorld_qpg6200

# Output: Work/HelloWorld_qpg6200/HelloWorld.hex
```

### Step 3: Flash the Device

```bash
# Using J-Link Commander
JLinkExe -device QPG6200 -if SWD -speed 4000 -autoconnect 1
J-Link> loadfile Work/HelloWorld_qpg6200/HelloWorld_merged.hex
J-Link> r
J-Link> g
J-Link> exit

# OR using Qorvo Platform Tools
python3 Tools/QorvoPlatformTools/program.py --hex Work/HelloWorld_qpg6200/HelloWorld_merged.hex
```

### Step 4: View Debug Output

```bash
# Connect USB-UART adapter to DK debug pins
# Open serial terminal at 115200 baud
minicom -D /dev/ttyUSB0 -b 115200
# OR
screen /dev/ttyUSB0 115200
```

Expected output:
```
Hello world
Hello world
Hello world
```

### Step 5: Build a Protocol Application

```bash
# Matter Light
cd Applications/Matter/Light
make -f Makefile.Light_Matter_qpg6200

# Zigbee Light
cd Applications/Zigbee/Light
make -f Makefile.Light_Zigbee_qpg6200

# Concurrent Light (Matter + Zigbee + BLE)
cd Applications/Concurrent/Light
make -f Makefile.Light_Concurrent_qpg6200
```

### Step 6: Commission Matter Device

1. Flash the Matter Light application
2. Install a Matter controller app (Apple Home / Google Home / chip-tool)
3. Scan the QR code printed during boot (or use `chip-tool` with passcode)
4. Follow commissioning wizard — device joins Thread network
5. Control light via Matter app

### Step 7: Create a New Application

1. Copy `Applications/HelloWorld/` to `Applications/MyApp/`
2. Update `Makefile.MyApp_qpg6200` (rename, adjust components)
3. Edit `inc/qPinCfg.h` with your GPIO assignments
4. Implement `Application_Init()` in `src/main.c`
5. Add FreeRTOS tasks for your application logic
6. `make -f Makefile.MyApp_qpg6200`

---

## Appendix A: Common Assert Codes

| Assert Code | Location | Cause | Fix |
|---|---|---|---|
| 156 | `gpBleActivityManager_Simple` | Context freed before update | Use v2.0.1+ |
| 176 | `gpHal_Ipc` | IPC buffer overflow | Increase `GPHAL_RT_UNTIMED_TIMED_ENTRIES_MAX` |
| 315 | `gpSched_rom` | Scheduler queue overflow | Increase `GP_SCHED_EVENT_LIST_SIZE` |
| 521 | `hal_ES` | Wrong crystal config before stack init | Call `gpHal_Set32kHzCrystalAvailable()` first |
| 764 | `gpNvm_RW_Kx_SubpagedFlash_v2` | NVM defrag race condition | Use protected NVM API |
| 1155 | `gpNvm_RW_Kx_SubpagedFlash_v2` | Concurrent NVM access | Use `_Protected` NVM functions |

---

## Appendix B: Key API Headers

| Header | Location | Purpose |
|---|---|---|
| `hal.h` | `Components/Qorvo/HAL_PLATFORM/` | Main HAL entry point |
| `gpHal_DEFS.h` | `Components/Qorvo/HAL_PLATFORM/` | HAL definitions |
| `gpBaseComps.h` | `Components/Qorvo/BaseUtils/` | Base component init |
| `gpSched.h` | `Components/Qorvo/BaseUtils/` | Event scheduler |
| `gpLog.h` | `Components/Qorvo/BaseUtils/` | Logging |
| `gpAssert.h` | `Components/Qorvo/BaseUtils/` | Assert macros |
| `gpCom.h` | `Components/Qorvo/BaseUtils/` | Communication layer |
| `gpNvm.h` | `Components/Qorvo/BaseUtils/` | Non-volatile memory |
| `gpBatteryMonitor.h` | `Components/Qorvo/` | Battery monitor |
| `qDrvGPIO.h` | `Components/Qorvo/gpPeripheral/` | GPIO driver |
| `qDrvUART.h` | `Components/Qorvo/gpPeripheral/` | UART driver |
| `qDrvSPIM.h` | `Components/Qorvo/gpPeripheral/` | SPI Master driver |
| `qDrvTWIM.h` | `Components/Qorvo/gpPeripheral/` | I2C Master driver |
| `qDrvGPADC.h` | `Components/Qorvo/gpPeripheral/` | GP ADC driver |
| `qDrvPWMXL.h` | `Components/Qorvo/gpPeripheral/` | PWM driver |
| `FreeRTOS.h` | `Components/ThirdParty/FreeRTOS/` | FreeRTOS kernel |
| `task.h` | `Components/ThirdParty/FreeRTOS/` | FreeRTOS task API |
| `queue.h` | `Components/ThirdParty/FreeRTOS/` | FreeRTOS queue API |

---

## Appendix C: IPC Buffer Sizing (v2.0.1)

| Parameter | Old Value | New Value (v2.0.1) | Effect |
|---|---|---|---|
| `GPHAL_RT_UNTIMED_TIMED_ENTRIES_MAX` | 7 → 14 | `GP_HAL_PBM_TYPE1_AMOUNT` | Prevents IPC buffer overflow in 200-node networks |
| `GP_SCHED_EVENT_LIST_SIZE` | 64 | 92 | Prevents scheduler queue overflow |
| BLE Preamble Threshold | 203 | 150 | Improved RPi4 Bluetooth interoperability |

---

*This document covers QPG6200 IoT SDK version 2.0.1. For the latest information, refer to the official Qorvo documentation portal.*
