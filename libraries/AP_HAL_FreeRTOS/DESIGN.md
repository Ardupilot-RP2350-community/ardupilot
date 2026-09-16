# AP_HAL_FreeRTOS Design

## Shared FreeRTOS Support for AP_HAL

`AP_HAL_FreeRTOS` provides the shared FreeRTOS-specific implementation of the ArduPilot HAL.

The shared layer implements common RTOS behaviour, including:

- `AP_HAL::Scheduler`
- task and thread management
- recursive mutexes and binary semaphores
- callback dispatch
- timing and delay handling
- stack and priority management
- the internal FreeRTOS port interface

Hardware-specific functionality remains implemented by the platform backends under `platforms/`.

## Scheduler and Platform Boundary

The shared scheduler owns:

- creation and execution of the main, timer, UART and IO tasks;
- task handles and main-thread identification;
- scheduler initialization state;
- timer and IO callback registration and dispatch;
- timer failsafe execution;
- RTOS delays and optional main-thread delay callbacks;
- thread creation and priority adjustment;
- stack-size validation and conversion;
- delegate and callback lifetime management.

Each platform provides a persistent scheduler `Config` containing:

- task names;
- stack sizes;
- task priorities;
- CPU affinity masks;
- public thread stack-unit policy;
- timer and IO task periods.

An affinity mask of zero allows execution on any available core. Non-zero masks select allowed cores by bit.

Platforms must implement:

- `delay_microseconds()`
- `delay_without_scheduler()`
- `reboot()`

Platforms may also provide optional hooks for main, timer and IO task initialization.

The default `main_task_yield()` yields after each application loop. Platforms may override this behaviour when they require a different delay, watchdog or scheduling policy.

`wait_for_system_initialized()` is available to platform hooks and additional platform tasks that must wait until ArduPilot initialization is complete.

## FreeRTOS Port Interface

`Port.h` defines the internal RTOS boundary used by the shared implementation.

The default `Port.cpp` backend provides standard FreeRTOS operations without dependencies on any vendor SDK.

It provides:

- task creation and deletion;
- task priority management;
- scheduler-state queries;
- current-task access;
- delays and yielding;
- recursive mutex operations;
- binary semaphore operations;
- ISR yield support;
- stack-size conversion.

Platforms using a FreeRTOS variant with different semantics may replace or adapt the default port implementation.

Examples include:

- ESP-IDF FreeRTOS stack sizes expressed in bytes;
- SMP-specific task creation and affinity APIs;
- platform-specific ISR yield mechanisms;
- vendor-specific scheduler extensions.

The shared scheduler must not depend directly on these platform-specific differences.

## FreeRTOS Kernel Requirements

The standard port requires the FreeRTOS configuration to provide:

- dynamic allocation;
- recursive mutexes;
- mutex-holder queries;
- task deletion;
- current-task queries;
- scheduler-state queries;
- runtime priority changes.

The build environment must provide:

```text
FreeRTOS.h
semphr.h
task.h
```

through the platform include paths.

When SMP affinity is enabled, the backend must support creating a task with its affinity set before the task becomes runnable.

## Stack Handling

Internal scheduler configuration expresses task stack sizes in bytes.

`Port.cpp` converts byte-sized stack requests to the stack units required by the underlying FreeRTOS API and checks for overflow during conversion.

New platforms should normally use:

```text
thread_stack_unit_bytes = 1
```

so the public thread API expresses stack sizes in bytes.

Platforms that must preserve an existing public API may use a different stack unit.

## Semaphore Behaviour

Semaphore timeout semantics are intentionally preserved:

- mutex `take(0)` blocks indefinitely;
- binary semaphore `wait(0)` performs a non-blocking wait;
- finite mutex waits use the HAL delay mechanism;
- finite binary semaphore waits convert the requested timeout to FreeRTOS ticks.

Existing allocation-failure and object-lifetime behaviour is preserved.

## Callback Registration

Timer and IO callback lists:

- ignore null callbacks;
- ignore duplicate registrations;
- have a fixed maximum capacity;
- serialize writers through a mutex;
- execute callbacks without holding the registration mutex.

Callback slots are published using release/acquire ordering so readers observe fully initialized entries.

Callbacks registered while a dispatch iteration is running become visible on the next iteration.

Callback registration is a task-context API and must not be used from an ISR.

## Task Creation Failures

Failure to create an essential scheduler task is considered fatal.

The scheduler panics instead of continuing with a missing worker task, because running ArduPilot with a partially initialized scheduler would leave the system in an invalid state.

## RP2350 Platform Behaviour

The RP2350 backend preserves the behaviour of the original RP HAL where practical.

It provides:

- scheduler configuration;
- RP2350 task priorities and stack sizes;
- CPU-core placement;
- Pico SDK timing and reboot operations;
- flash-lockout initialization;
- RC output initialization;
- the USB worker task.

Timer and IO tasks may start before application setup is complete.

Tasks that depend on initialized ArduPilot state, such as UART or USB workers, wait for system initialization before continuing.

The RP2350 timer and IO periods remain:

```text
Timer task: 1 ms
IO task:    10 ms
```

The timer failsafe executes once per timer iteration.

Device-bus callbacks remain implemented by the hardware-specific drivers.

RP2350 preserves its historical word-based public `thread_create()` stack behaviour through its platform configuration.

## Platform Backends

Hardware-specific implementations live under:

```text
platforms/<platform>/
```

For example:

```text
platforms/rp2350/
platforms/esp32/
platforms/imxrt/
```

A platform backend may provide:

- UART, SPI and I2C drivers;
- GPIO and ADC;
- DMA;
- PWM and RC input/output;
- CAN;
- flash and storage;
- USB;
- networking;
- watchdog support;
- platform startup and shutdown;
- SDK-specific build integration.

Vendor SDK dependencies must remain inside the platform backend and must not leak into the shared scheduler or semaphore implementation.

## ESP32 Integration

ESP32 integration is expected to require an ESP-IDF-specific implementation of the `Port.h` contract.

ESP-IDF differences that must be handled include:

- byte-sized stack arguments;
- SMP and affinity behaviour;
- ISR yield handling;
- watchdog integration;
- IRAM placement requirements;
- ESP-specific timing and worker-task policies.

The standard FreeRTOS stack conversion must not be applied blindly to ESP-IDF APIs that already use byte-sized stack sizes.

Migration of `AP_HAL_ESP32` should therefore be treated as a platform integration task, not as a simple source-code move.

## Tests

The standalone host test suite uses:

- ArduPilot `AP_gtest.h`;
- the repository GoogleTest submodule;
- the FreeRTOS POSIX port.

The suite covers:

- callback capacity;
- duplicate registration rejection;
- registration from callbacks;
- concurrent callback publishers;
- failsafe dispatch;
- initialization ordering;
- worker startup;
- priority clamping;
- stack conversion and overflow detection;
- delay callbacks.

Example:

```sh
cmake -S libraries/AP_HAL_FreeRTOS/tests/posix \
    -B /tmp/ap-hal-freertos-tests \
    -DFREERTOS_KERNEL_PATH=/path/to/FreeRTOS-Kernel

cmake --build /tmp/ap-hal-freertos-tests

ctest --test-dir /tmp/ap-hal-freertos-tests --output-on-failure
```

Hardware timing, peripherals, SMP behaviour and multicore execution must still be validated on the target hardware.

## Build Integration

Waf builds the shared `AP_HAL_FreeRTOS` sources together with the selected platform backend.

Only the active platform backend is compiled.

For example:

```text
RP2350 board
    AP_HAL_FreeRTOS shared sources
    +
    platforms/rp2350/

ESP32 board
    AP_HAL_FreeRTOS shared sources
    +
    platforms/esp32/

i.MX RT board
    AP_HAL_FreeRTOS shared sources
    +
    platforms/imxrt/
```

Other ArduPilot HALs do not gain a FreeRTOS dependency.

The selected board determines the FreeRTOS platform, native SDK, toolchain and platform-specific build integration.