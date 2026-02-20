# BLE Peripheral

## Introduction

Qorvo&reg; QPG6200 BLE Peripheral is an example application of controlling LED by Bluetooth&reg; LE characteristic write operation. This example can be used as a reference for creating your own BLE application.

## Hardware Setup

![QPG6200 Carrier Board, Zigbee and Matter&trade;](.images/ble_peripheral_example_io.png)

## QPG6200 GPIO Configurations

| GPIO Name| Direction| Connected To| Comments|
|:----------:|:----------:|:----------:|:---------|
| RESETN| Input| SW1| Reset Button|
| GPIO0| Input (Pull-up)| SW9| BLE start advertising button|
| GPIO1| Output| LD6| Dimmable LED (White cool), used as status LED |
| GPIO11| Output | D8| Dimmable Color LED (Blue), used as BLE connection status|

## Application Usage

| Item                                   | Action |Result                                                                     |
| -------------------------------------- | ---------------------------------------------------------------------------------- |---------------------------------------------------------------------------------- |
| Start up | Power On Device |The application restarts and two LEDs: status and connection are off|
| Connect   | Hold SW9 button for 2 seconds | BLE advertising.|

To test BLE peripheral application example use any BLE enabled utility (Qorvo Connect [iOS](https://apps.apple.com/pl/app/qorvo-connect/id1588263604)/[Android](https://play.google.com/store/apps/details?id=com.qorvo.ble&pcampaignid=web_share) app recommended). Set QPG6200 to BLE advertising mode and connect. <br>
During advertising connect to the device and write to LED control characteristic to control the cool white LED (LD6).

## Services and characteristics

```
Device: qBLE peripheral
└── Services
    ├── Generic Access Service (0x1800)
    │   ├── Device Name (0x2A00)
    │   │   └── Properties: Read
    │   └── Appearance (0x2A01)
    │       └── Properties: Read
    ├── Device Information Service (0x180A)
    │   ├── Manufacturer Name (0x2A29)
    │   │   └── Properties: Read
    │   ├── Model Number (0x2A24)
    │   │   └── Properties: Read
    │   └── Firmware Revision (0x2A26)
    │       └── Properties: Read
    ├── Battery Service (0x180F)
    │   └── Battery Level (0x2A19)
    │       └── Properties: Read, Notify
    ├── LED Control Service (12345678-1234-5678-9ABC-123456789ABC)
    │   └── LED Control (87654321-4321-8765-CBA9-987654321CBA)
    │       └── Properties: Read, Notify, Write

```

Advertising data and parameters, services, characteristics are configurable from single point `Ble_Peripheral_Config.c`

## Status Reporting

### BLE connection

The blue LED represents the BLE connection/advertising status

| LD6 State | BLE Status |
| --------- | ------------- |
| OFF | Disconnected |
| Blink | Advertising |
| ON | Connected |

### Status LED

The white LED represents the value of the _LED Control characteristic_ of the proprietary _LED Control Service_.

| Write value | LED Status |
| --------- | ------------- |
| 0 | ON |
| 1 | OFF |

## Firmware architecture

![Firmware architecture BLE](.images/ble_arch_diagram.png)

## SDK API Reference

See the [API Manual](../../../Documents/API Manuals/API_Manuals.rst)

## Building and flashing

Please refer to the section [Installation Guide](../../../sphinx/docs/QUICKSTART.rst#install-the-sdk) to setup a build environment.

Please refer to the section [Building and flashing the example applications](../../../sphinx/docs/QUICKSTART.rst#working-with-examples) to get instructions on
how to build and program the Ble peripheral example application.
