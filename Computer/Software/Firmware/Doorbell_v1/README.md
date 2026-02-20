# HelloWorld

## Introduction

The Hello World reference application demonstrates the basic logging functionality of the QPG6200 Development Kit.

## Hardware Board
![QPG6200 Carrier Board, LED](.images/iot_carrier_board.png)

## QPG6200 GPIO Configurations

| GPIO Name| Direction| Connected to| Comments|
|:----------:|:----------:|:----------:|:---------|
| GPIO11    | Output     | D6 | RGB LED|
| GPIO9     | Output     | J35| Configured as UART TX |
| GPIO8     | Input      | J35| Configured as UART RX |


## Usage

The application every second:
-   Prints "Hello World"
-   Blinks the LED

For a guide to enable serial logging, please refer to the [serial logging section](../../sphinx/docs/QUICKSTART.rst#serial-logging).

Once serial logging is correctly setup, after resetting the programmed QPG6200 with the application, you will see "Hello world" is printed out periodically which will be similar as below:

```
Hello world
Hello world
Hello world
Hello world
Hello world
Hello world

```
