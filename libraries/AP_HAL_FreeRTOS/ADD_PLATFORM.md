# Adding a New Platform to `AP_HAL_FreeRTOS`

## 1. Purpose

`AP_HAL_FreeRTOS` provides the common ArduPilot HAL implementation for platforms that use FreeRTOS, while keeping MCU/vendor-specific code outside the generic layer.

Expected architecture:

```text
ArduPilot
    ↓
AP_HAL
    ↓
AP_HAL_FreeRTOS
    ↓
platform adapter
    ↓
vendor SDK / BSP
    ↓
MCU
```

Examples:

```text
AP_HAL_FreeRTOS
├── RP2350
│   └── Pico SDK
├── ESP32
│   └── ESP-IDF
└── i.MX RT
    └── MCUXpresso SDK
```

`AP_HAL_FreeRTOS` should abstract FreeRTOS/ArduPilot behaviour, not replace native vendor SDKs with another hardware-abstraction framework.

---

## 2. Directory Layout

A new platform is added below:

```text
libraries/AP_HAL_FreeRTOS/platforms/
```

Recommended structure:

```text
libraries/AP_HAL_FreeRTOS/
├── AP_HAL_FreeRTOS.h
├── AP_HAL_FreeRTOS_Private.h
├── HAL_FreeRTOS_Class.cpp
├── HAL_FreeRTOS_Class.h
├── Platform.h
├── Port.cpp
├── Port.h
├── Scheduler.cpp
├── Scheduler.h
├── Semaphores.cpp
├── Semaphores.h
├── Thread.cpp
├── Thread.h
│
├── hwdef/
│   ├── Kolibri/
│   │   └── hwdef.dat
│   └── scripts/
│       └── rp2350_hwdef.py
│
└── platforms/
    ├── rp2350/
    │   ├── waf.py
    │   ├── Platform.cpp
    │   ├── Scheduler.cpp
    │   ├── Scheduler.h
    │   ├── RP2350_Private.h
    │   ├── RP2350_Namespace.h
    │   └── ...
    ├── esp32/
    │   ├── waf.py
    │   ├── Platform.cpp
    │   ├── Scheduler.cpp
    │   ├── Scheduler.h
    │   └── ...
    └── imxrt/
        ├── waf.py
        ├── Platform.cpp
        ├── Scheduler.cpp
        ├── Scheduler.h
        └── ...
```

---

## 3. Generic vs Platform-Specific Code

### Generic `AP_HAL_FreeRTOS`

Place code in the root of `AP_HAL_FreeRTOS` if it depends only on:

- FreeRTOS API;
- `AP_HAL`;
- generic C/C++;
- `AP_HAL_FreeRTOS::Platform` API.

Typical generic files:

```text
Scheduler.cpp
Semaphores.cpp
Thread.cpp
Port.cpp
HAL_FreeRTOS_Class.cpp
```

Typical generic functionality:

- task creation;
- scheduler startup;
- main/timer/IO tasks;
- callback registration;
- semaphores and mutexes;
- member-function threads;
- generic FreeRTOS task management.

### Platform-specific code

Place code under:

```text
platforms/<platform>/
```

if it depends on:

- vendor SDK;
- MCU registers;
- peripheral instances;
- interrupt controller;
- clocks;
- DMA;
- cache;
- USB implementation;
- flash layout;
- linker layout;
- startup code;
- vendor-specific timer implementation.

Examples:

```text
platforms/rp2350/UARTDriver.cpp
platforms/rp2350/GPIO.cpp
platforms/rp2350/Flash.cpp
platforms/rp2350/Platform.cpp
```

For i.MX RT this layer would use MCUXpresso SDK. For ESP32 it would use ESP-IDF.

---

## 4. Register the Platform in the FreeRTOS Waf Dispatcher

Edit:

```text
Tools/ardupilotwaf/freertos.py
```

Add the new platform:

```python
SUPPORTED_PLATFORMS = (
    'rp2350',
    'esp32',
    'imxrt',
)
```

The dispatcher loads:

```text
libraries/AP_HAL_FreeRTOS/platforms/<platform>/waf.py
```

Each platform therefore needs:

```text
platforms/<platform>/waf.py
```

Possible hooks:

```python
def configure(cfg):
    pass


def configure_env(cfg, env):
    pass


def pre_build(bld):
    pass
```
---

## 5. Select the Platform in `hwdef.dat`

A FreeRTOS board should explicitly select both platform and MCU.

RP2350 example:

```text
PLATFORM RP2350
MCU RP2350A
```

i.MX RT example:

```text
PLATFORM IMXRT
MCU MIMXRT1176
```

ESP32 example:

```text
PLATFORM ESP32
MCU ESP32S3
```

Recommended terminology:

```text
HAL      = FreeRTOS
Platform = RP2350 / ESP32 / IMXRT
MCU      = specific MCU
Board    = specific flight controller
```

Do not use `CONFIG_HAL_BOARD_SUBTYPE` to distinguish FreeRTOS platforms.

Keep:

```cpp
CONFIG_HAL_BOARD=HAL_BOARD_FREERTOS
CONFIG_HAL_BOARD_SUBTYPE=HAL_BOARD_SUBTYPE_NONE
```

---

## 6. Dynamic Board Registration

The dynamically generated board class must carry the selected platform.

Conceptually:

```python
newclass = type(
    board_name,
    (freertos,),
    {
        'name': board_name,
        'platform': platform,
    }
)
```

The `platform` value should be parsed from `hwdef.dat`.

Without this, the generic `freertos` board class cannot select:

```text
platforms/rp2350/waf.py
platforms/esp32/waf.py
platforms/imxrt/waf.py
```

---

## 7. Create `platforms/<platform>/waf.py`

The platform Waf file is responsible for the vendor toolchain and build environment.

Typical responsibilities:

```python
def configure(cfg):
    # Check SDK path and compiler.
    # Parse/generate platform-specific hwdef data.
    pass


def configure_env(cfg, env):
    # Compiler flags.
    # Platform defines.
    # SDK include paths.
    # Linker configuration.
    # Vendor build variables.
    pass


def pre_build(bld):
    # Generate vendor build files / headers if needed.
    pass
```

Examples:

### RP2350

- Pico SDK;
- Cortex-M33 flags;
- FreeRTOS RP2350 port;
- generated Pico CMake project;
- PIO sources.

### ESP32

- ESP-IDF;
- Xtensa or RISC-V toolchain depending on MCU;
- IDF component configuration;
- ESP-IDF FreeRTOS.

### i.MX RT

- MCUXpresso SDK;
- Cortex-M7/M33 compiler flags;
- linker script;
- cache configuration;
- startup files;
- FreeRTOS portable layer.

Vendor-specific configuration must stay out of generic `freertos.py`.

---

## 8. Add a Platform Namespace

Platform-specific classes should have their own namespace.

RP2350:

```cpp
namespace AP_HAL_FreeRTOS::RP2350 {
class UARTDriver;
class GPIO;
class Scheduler;
}
```

ESP32:

```cpp
namespace AP_HAL_FreeRTOS::ESP32 {
class UARTDriver;
class GPIO;
class Scheduler;
}
```

i.MX RT:

```cpp
namespace AP_HAL_FreeRTOS::IMXRT {
class UARTDriver;
class GPIO;
class Scheduler;
}
```

In platform code, a local alias is useful:

```cpp
namespace RP = AP_HAL_FreeRTOS::RP2350;
```

Avoid broad `using namespace AP_HAL_FreeRTOS;`, especially where generic and platform classes have the same names.

---

## 9. Create a Platform Private Header

Create:

```text
platforms/<platform>/<PLATFORM>_Private.h
```

Example:

```text
platforms/imxrt/IMXRT_Private.h
```

Typical contents:

```cpp
#pragma once

#include "Scheduler.h"
#include "UARTDriver.h"
#include "GPIO.h"
#include "SPIDevice.h"
#include "I2CDevice.h"
#include "Storage.h"
#include "Util.h"
```

The root `AP_HAL_FreeRTOS_Private.h` should contain only generic FreeRTOS components.

---

## 10. Implement the Platform Contract

The generic HAL accesses platform-specific objects through:

```text
AP_HAL_FreeRTOS/Platform.h
```

The platform must implement at least:

```cpp
namespace AP_HAL_FreeRTOS::Platform {

const HALConfig &get_hal_config();
void init();

}
```

Optional platform services may also be exposed here when needed.

---

## 11. Implement `Platform.cpp`

Create:

```text
platforms/<platform>/Platform.cpp
```

Example skeleton:

```cpp
#include <AP_HAL_FreeRTOS/Platform.h>
#include "IMXRT_Private.h"

namespace PLAT = AP_HAL_FreeRTOS::IMXRT;

namespace AP_HAL_FreeRTOS::Platform {

static PLAT::UARTDriver serial0Driver;
static PLAT::GPIO gpioDriver;
static PLAT::Scheduler schedulerInstance;
static PLAT::Util utilInstance;

static const HALConfig hal_config {
    .serial0 = &serial0Driver,
    .console = &serial0Driver,
    .gpio = &gpioDriver,
    .scheduler = &schedulerInstance,
    .util = &utilInstance,
};

const HALConfig &get_hal_config()
{
    return hal_config;
}

void init()
{
    // Platform initialization before ArduPilot starts.
}

}
```

`Platform.cpp` is the composition root for the platform hardware implementation.

---

## 12. Implement a Concrete Platform Scheduler

The root scheduler:

```text
AP_HAL_FreeRTOS/Scheduler.h
```

contains common FreeRTOS scheduler behaviour and may remain abstract.

The platform scheduler derives from it:

```cpp
namespace AP_HAL_FreeRTOS::IMXRT {

class Scheduler : public AP_HAL_FreeRTOS::Scheduler
{
public:
    Scheduler();

    void delay_microseconds(uint16_t us) override;
    void delay_without_scheduler(uint16_t ms) override;
    void reboot(bool hold_in_bootloader = false) override;
};

}
```

The platform constructor must supply the generic scheduler configuration:

```cpp
namespace AP_HAL_FreeRTOS::IMXRT {

static const AP_HAL_FreeRTOS::Scheduler::Config scheduler_config {
    // task names
    // stack sizes
    // priorities
    // periods
    // affinity masks
};

Scheduler::Scheduler()
    : AP_HAL_FreeRTOS::Scheduler(scheduler_config)
{
}

}
```

Never instantiate the generic abstract scheduler directly:

```cpp
// Wrong
static AP_HAL_FreeRTOS::Scheduler schedulerInstance;
```

Use:

```cpp
static AP_HAL_FreeRTOS::IMXRT::Scheduler schedulerInstance;
```

---

## 13. Implement Required Drivers

Recommended bring-up order:

1. Scheduler
2. Util
3. GPIO
4. UART
5. console
6. Storage
7. SPI
8. I2C
9. AnalogIn
10. RCInput
11. RCOutput
12. Flash
13. optional peripherals

Do not implement drivers that are not needed for the first board.

---

## 14. UART / Console

Implement a platform-specific driver derived from:

```cpp
AP_HAL::UARTDriver
```

Use the native vendor API:

```text
RP2350 -> Pico SDK UART
ESP32  -> ESP-IDF UART driver
i.MX RT -> MCUXpresso LPUART
```

Do not add a new generic register-level UART abstraction to `AP_HAL_FreeRTOS`.

---

## 15. GPIO

Implement:

```cpp
AP_HAL::GPIO
```

and, where required:

```cpp
AP_HAL::DigitalSource
```

Use the vendor SDK GPIO API. Prefer generating board pin mappings from `hwdef.dat`.

---

## 16. SPI and I2C

Implement standard ArduPilot interfaces:

```cpp
AP_HAL::SPIDeviceManager
AP_HAL::SPIDevice
AP_HAL::I2CDeviceManager
AP_HAL::I2CDevice
```

Verify:

- bus locking;
- semaphores;
- timeout handling;
- DMA compatibility;
- cache coherency;
- chip select handling;
- bus speed changes;
- ISR restrictions.

On Cortex-M7 systems such as many i.MX RT devices, DMA/cache coherency requires special attention.

---

## 17. Storage

Implement:

```cpp
AP_HAL::Storage
```

Possible backends:

```text
internal flash
external NOR flash
FRAM
NAND
vendor NVS
```

If using `AP_FlashStorage`, make sure its implementation is linked from the ArduPilot libraries.

---

## 18. USB

USB is platform-specific.

Examples:

```text
RP2350 -> TinyUSB through Pico SDK
ESP32  -> ESP-IDF/TinyUSB depending on MCU
i.MX RT -> MCUXpresso USB stack or TinyUSB
```

Use explicit namespaces for platform helper APIs.

Bad:

```cpp
extern void init_usb_cdc_drivers();
```

Preferred:

```cpp
namespace AP_HAL_FreeRTOS::Platform {
void init_usb_cdc_drivers();
}
```

Call it as:

```cpp
AP_HAL_FreeRTOS::Platform::init_usb_cdc_drivers();
```

Declaration, definition and call must have exactly the same namespace.

---

## 19. Optional Platform Services

Not every platform-specific device belongs in `AP_HAL::HAL`.

For example, NAND may be exposed through:

```cpp
namespace AP_HAL_FreeRTOS::Platform {
AP_Filesystem_NAND_Device *get_nand_device();
}
```

This is preferable to adding platform-specific devices to the generic `AP_HAL::HAL` constructor.

Keep `Platform.h` small and focused; do not turn it into a vendor API.

---

## 20. Generate Platform Headers from `hwdef`

Where practical, let the platform-specific hwdef generator convert board configuration into generated headers.

Example:

For i.MX RT:

```text
hwdef.dat
    ↓
imxrt_hwdef.py
    ↓
imxrt_gpio_defines.h
```

Useful generated data includes:

- MCU type;
- pin definitions;
- UART assignments;
- SPI buses;
- I2C buses;
- flash layout;
- CPU/memory class;
- feature macros.

---

## 21. Build Defines

Keep the generic HAL identity:

```cpp
CONFIG_HAL_BOARD=HAL_BOARD_FREERTOS
CONFIG_HAL_BOARD_SUBTYPE=HAL_BOARD_SUBTYPE_NONE
```

Use separate MCU/platform defines where needed, for example:

```cpp
HAL_MCU_IMXRT1176_ENABLED=1
```

Do not reintroduce platform-specific `CONFIG_HAL_BOARD` values such as `HAL_BOARD_RP2350`.

---

## 22. Toolchain Selection

Do not assume all FreeRTOS platforms use the same compiler.

Examples:

```text
RP2350    -> arm-none-eabi
i.MX RT   -> arm-none-eabi
ESP32-S3  -> xtensa-esp32s3-elf
ESP32-C6  -> riscv32-esp-elf
```

Toolchain selection should therefore belong to the platform configuration rather than the generic FreeRTOS board class.

---

## 23. Vendor Build-System Integration

Some SDKs require their own build system.

Examples:

```text
Pico SDK   -> CMake
ESP-IDF    -> CMake/idf.py
MCUXpresso -> CMake or SDK-specific integration
```

Keep vendor build integration under:

```text
platforms/<platform>/
```

The generic FreeRTOS layer should not know vendor build-system details.

---

## 24. Do Not Compile Generic Sources Twice

Generic files such as:

```text
AP_HAL_FreeRTOS/Scheduler.cpp
AP_HAL_FreeRTOS/Thread.cpp
AP_HAL_FreeRTOS/Semaphores.cpp
AP_HAL_FreeRTOS/HAL_FreeRTOS_Class.cpp
```

should normally be compiled by Waf as part of ArduPilot.

If a platform uses a vendor CMake project, avoid compiling those same generic sources again.

Preferred split:

```text
Waf:
    generic AP_HAL_FreeRTOS

Vendor CMake:
    platform/<platform> sources
    vendor SDK
```

A vendor CMake target should ideally represent only the platform:

```cmake
add_library(AP_HAL_FreeRTOS_IMXRT STATIC
    ${HAL_SRC}/Platform.cpp
    ${HAL_SRC}/Scheduler.cpp
    ${HAL_SRC}/UARTDriver.cpp
    ${HAL_SRC}/GPIO.cpp
)
```

This avoids:

- duplicate symbols;
- ODR problems;
- different compile flags for the same source;
- confusing linker behaviour.

---

## 25. Static Library Link Order

GNU `ld` processes static libraries in order.

A platform library may depend on ArduPilot libraries while ArduPilot libraries also reference platform symbols.

Example cycle:

```text
libArduCopter_libs.a
    ↓
Platform::get_hal_config()

platform library
    ↓
AP_FlashStorage
    ↓
libArduCopter_libs.a
```

Use a linker group when necessary:

```cmake
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    "-Wl,--start-group"

    "${ARDUPILOT_BIN}/libarducopter.a"
    "${ARDUPILOT_LIB}/libArduCopter_libs.a"
    AP_HAL_FreeRTOS_IMXRT

    "-Wl,--end-group"
)
```

---

## 26. Namespace Rules

Be strict about namespaces.

Declaration:

```cpp
namespace AP_HAL_FreeRTOS {
bool thread_create(...);
}
```

Correct definition:

```cpp
namespace AP_HAL_FreeRTOS {

bool thread_create(...)
{
    ...
}

}
```

Incorrect:

```cpp
using namespace AP_HAL_FreeRTOS;

bool thread_create(...)
{
}
```

The latter defines:

```cpp
::thread_create()
```

not:

```cpp
AP_HAL_FreeRTOS::thread_create()
```

`using namespace` affects lookup only; it does not place new declarations into that namespace.

---

## 27. Validate Linker Symbols with `nm`

For linker errors, inspect actual symbols.

Example:

```bash
arm-none-eabi-nm -C \
    build/<board>/lib/libArduCopter_libs.a \
    | grep thread_create
```

Legend:

```text
T  defined text/code symbol
t  local text symbol
U  undefined symbol expected from another object/library
```

For platform functions:

```bash
arm-none-eabi-nm -C \
    build/<board>/<vendor-build>/libAP_HAL_FreeRTOS_IMXRT.a \
    | grep "Platform::"
```

Expected examples:

```text
T AP_HAL_FreeRTOS::Platform::init()
T AP_HAL_FreeRTOS::Platform::get_hal_config()
```

---

## 28. First Minimal Bring-Up

### Stage 1 — Toolchain

Confirm:

```text
compiler works
startup works
linker script works
binary is generated
```

### Stage 2 — FreeRTOS

Confirm:

```text
FreeRTOS starts
main task runs
tick works
delay works
task creation works
```

### Stage 3 — HAL skeleton

Implement:

```text
Platform.cpp
Scheduler
Util
GPIO
console UART
```

Confirm that ArduPilot reaches early startup.

### Stage 4 — Persistent storage

Add:

```text
Storage
Flash/NVS backend
```

### Stage 5 — Sensor buses

Add:

```text
SPI
I2C
DMA if required
```

### Stage 6 — Flight-control I/O

Add:

```text
RCInput
RCOutput
UART telemetry
```

### Stage 7 — Optional peripherals

Add only when required:

```text
USB
CAN
NAND
SD
WSPI
DSP
networking
```

---

## 29. New Platform Checklist

### Build integration

- [ ] Add platform to `SUPPORTED_PLATFORMS`.
- [ ] Create `platforms/<platform>/waf.py`.
- [ ] Make the dynamic board class set `platform`.
- [ ] Add `PLATFORM <name>` to `hwdef.dat`.
- [ ] Add MCU selection.
- [ ] Configure vendor SDK.
- [ ] Configure compiler/toolchain.
- [ ] Configure linker script/vendor build system.

### Platform code

- [ ] Create platform namespace.
- [ ] Create platform private header.
- [ ] Implement `Platform.cpp`.
- [ ] Implement `get_hal_config()`.
- [ ] Implement `Platform::init()`.
- [ ] Implement concrete Scheduler.
- [ ] Implement `delay_microseconds()`.
- [ ] Implement `delay_without_scheduler()`.
- [ ] Implement `reboot()`.

### Basic HAL

- [ ] Util
- [ ] GPIO
- [ ] UART
- [ ] console
- [ ] Storage
- [ ] SPI
- [ ] I2C

### Flight-control drivers

- [ ] AnalogIn
- [ ] RCInput
- [ ] RCOutput
- [ ] Flash
- [ ] optional USB
- [ ] optional CAN
- [ ] optional NAND/SD

### Build validation

- [ ] Generic FreeRTOS sources are not compiled twice.
- [ ] Platform-specific sources are compiled.
- [ ] No old `HAL_BOARD_<platform>` remains.
- [ ] `CONFIG_HAL_BOARD=HAL_BOARD_FREERTOS`.
- [ ] No platform selection through `HAL_BOARD_SUBTYPE`.
- [ ] Static library ordering is correct.
- [ ] Linker group is used if circular dependencies exist.
- [ ] Namespace symbols match declarations exactly.

---

## 30. Example: Adding i.MX RT

Possible initial structure:

```text
libraries/AP_HAL_FreeRTOS/platforms/imxrt/
├── waf.py
├── Platform.cpp
├── IMXRT_Private.h
├── Scheduler.cpp
├── Scheduler.h
├── UARTDriver.cpp
├── UARTDriver.h
├── GPIO.cpp
├── GPIO.h
├── SPIDevice.cpp
├── SPIDevice.h
├── I2CDevice.cpp
├── I2CDevice.h
├── Storage.cpp
├── Storage.h
├── Util.cpp
├── Util.h
└── system.cpp
```

Board:

```text
libraries/AP_HAL_FreeRTOS/hwdef/MyIMXRTBoard/hwdef.dat
```

Example:

```text
PLATFORM IMXRT
MCU MIMXRT1176
```

Use MCUXpresso SDK for:

- clocks;
- LPUART;
- LPSPI;
- LPI2C;
- GPIO;
- timers;
- DMA;
- cache;
- reset.

Pay special attention to:

- Cortex-M7 D-cache;
- DMA cache maintenance;
- memory regions;
- tightly coupled memory;
- FreeRTOS-compatible interrupt priorities;
- timer source for microsecond timing;
- dual-core startup if using an M7/M4 variant.

---

## 31. Example: Adding ESP32

Possible initial structure:

```text
libraries/AP_HAL_FreeRTOS/platforms/esp32/
├── waf.py
├── Platform.cpp
├── ESP32_Private.h
├── Scheduler.cpp
├── Scheduler.h
├── UARTDriver.cpp
├── GPIO.cpp
├── SPIDevice.cpp
├── I2CDevice.cpp
├── Storage.cpp
└── Util.cpp
```

Example board:

```text
PLATFORM ESP32
MCU ESP32S3
```

Use native ESP-IDF for:

- FreeRTOS integration;
- UART;
- GPIO;
- SPI;
- I2C;
- flash/NVS;
- timers;
- USB where supported;
- networking if required later.

Pay special attention to:

- ESP-IDF already owns/configures FreeRTOS;
- task affinity;
- dual-core vs single-core chips;
- Xtensa vs RISC-V toolchains;
- IRAM/DRAM requirements;
- interrupt-safe functions;
- flash-cache restrictions;
- ESP-IDF component build integration.

Do not import a second FreeRTOS kernel when using ESP-IDF.

---

## 32. Design Rule

Use this rule when deciding where code belongs:

> If the implementation can compile for RP2350, ESP32 and i.MX RT without including vendor SDK headers, it is probably generic.

If it requires headers such as:

```cpp
#include "pico/..."
#include "driver/uart.h"
#include "fsl_lpuart.h"
```

it belongs in:

```text
platforms/<platform>/
```

---

## 33. Recommended Final Architecture

```text
                       AP_HAL
                         │
                         ▼
                 AP_HAL_FreeRTOS
                  generic layer
                         │
            ┌────────────┼────────────┐
            │            │            │
            ▼            ▼            ▼
         RP2350         ESP32        i.MX RT
            │            │            │
            ▼            ▼            ▼
        Pico SDK       ESP-IDF     MCUXpresso
            │            │            │
            ▼            ▼            ▼
          RP2350       ESP32xx      MIMXRTxxxx
```

`AP_HAL_FreeRTOS` owns common ArduPilot/FreeRTOS behaviour.

The platform owns hardware-specific adaptation.

The vendor SDK owns the MCU-level implementation.