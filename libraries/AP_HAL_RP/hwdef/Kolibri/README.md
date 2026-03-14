# Kolibri v0.6

This is an open-hardware project, that is available by the following link https://github.com/bastian2001/Kolibri-FC-Hardware

## Features

- Processor
  - RP2350A Cortex-M33 2-core 32-bit Processor
- Sensors
  - ICM-42688-P IMU
  - STM LPS22HB Baro
- Power
  - External Power Supply 3-8S
  - Logic level at 3.3V
  - 5V 2.5A, 10V 2.5A buck converters
- Interfaces
  - 4x PWM
  - 3x UARTs
  - 2x SPI
  - 1x I2C
  - 1x SWD
  - 1x USB
- Memory
  - NOR Flash (2MB)
  - NAND Flash (256MB)
- Miscellaneous
  - Onboard ERLS 2.4 GHz
  - Onboard 3 color LEDs
  - Buzzer
  - Bootsel Button
  - ERLS Button

## Build

`./waf configure --board Kolibri`
`./waf copter`
