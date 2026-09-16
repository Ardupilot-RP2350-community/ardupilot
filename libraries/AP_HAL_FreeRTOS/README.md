# AP_HAL_FreeRTOS

`AP_HAL_FreeRTOS` provides a common ArduPilot HAL foundation for platforms that use **FreeRTOS together with the native vendor SDK**.

The goal is to avoid creating a separate top-level HAL for every MCU family while still using the vendor-provided peripheral drivers and low-level support.

## Architecture

```text
ardupilot
   |
libraries
   │
   └── AP_HAL_FreeRTOS
             ├── AP_HAL_FreeRTOS.h
             ├── AP_HAL_FreeRTOS_Namespace.h
             ├── AP_HAL_FreeRTOS_Private.h
             ├── HAL_FreeRTOS_Class.h/.cpp
             │
             ├── Port.h/.cpp
             ├── Scheduler.h/.cpp
             ├── Semaphores.h/.cpp
             ├── Thread.h/.cpp
             │
             ├── DESIGN.md
             ├── README.md
             │
             ├── hwdef
             │      ├── scripts/
             │      └── <board>/hwdef.dat
             │
             ├── platforms
             │      ├── esp32/         ESP-IDF
             │      ├── imxrt/         MCUXpresso SDK
             │      ├── rp2350/        Pico SDK
             │      ├── renesas_ra/    Renesas FSP
             │      └── ti_am26x/      TI MCU+ SDK
             └── tests
                    └── posix/
```

The root of AP_HAL_FreeRTOS contains the FreeRTOS-specific shared implementation, including scheduler, task management, semaphores, timing, synchronization, and other common HAL infrastructure that can be reused across platforms.

Each directory under `platforms/` implements the hardware-specific parts of the HAL using the MCU vendor's native SDK, including UART, SPI, I2C, GPIO, DMA, PWM, CAN, flash, and other peripherals.

## Design Goals

- Share FreeRTOS-specific HAL infrastructure between MCU families.
- Reuse mature native vendor SDK drivers instead of reimplementing peripheral support.
- Keep MCU-specific code isolated inside platform backends.
- Make adding a new FreeRTOS-based MCU family possible without introducing another top-level `AP_HAL_*` directory.
- Preserve access to platform-specific hardware features and optimizations.

## Non-Goal

`AP_HAL_FreeRTOS` is not intended to provide a universal hardware driver layer. Peripheral implementations remain platform-specific and are provided by the corresponding native SDK.

## Summary:

> **FreeRTOS provides the common runtime model, while native vendor SDKs provide the hardware layer. AP_HAL_FreeRTOS connects these two layers to ArduPilot without owning either the vendor SDK or the FreeRTOS kernel.**
