/*
 * This file is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Scheduler.h"
#include <AP_HAL_FreeRTOS/AP_HAL_FreeRTOS_Private.h>
#include "RP2350_Private.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"

using namespace AP_HAL_FreeRTOS::RP2350;

extern const AP_HAL::HAL& hal;

const Scheduler::Config Scheduler::_config = {
    {"Ardu_Main", MAIN_SS * sizeof(StackType_t), MAIN_PRIO, 1U << 0},
    {"Ardu_Timer", TIMER_SS * sizeof(StackType_t), TIMER_PRIO, 1U << 0},
    {"Ardu_UART", UART_SS * sizeof(StackType_t), UART_PRIO, 1U << 0},
    {"Ardu_IO", IO_SS * sizeof(StackType_t), MAIN_PRIO + 1, 1U << 1},
    {
        IO_PRIO,        // PRIORITY_BOOST
        MAIN_PRIO,      // PRIORITY_MAIN
        SPI_PRIORITY,   // PRIORITY_SPI
        I2C_PRIORITY,   // PRIORITY_I2C
        IO_PRIO,        // PRIORITY_CAN
        TIMER_PRIO,     // PRIORITY_TIMER
        RCOUT_PRIO,     // PRIORITY_RCOUT
        IO_PRIO,        // PRIORITY_LED
        RCIN_PRIO,      // PRIORITY_RCIN
        IO_PRIO,        // PRIORITY_IO
        UART_PRIO,      // PRIORITY_UART
        STORAGE_PRIO,   // PRIORITY_STORAGE
        UART_PRIO,      // PRIORITY_SCRIPTING
        IO_PRIO,        // PRIORITY_NET
    },
    1024U * sizeof(StackType_t), // Preserve the historical extra stack depth.
    sizeof(StackType_t),         // RP historically treats thread_create sizes as words.
    1,                          // Timer period in milliseconds.
    10,                         // IO period in milliseconds.
    false,                      // Preserve RP delay behaviour without delay callbacks.
};

const Scheduler::TaskConfig Scheduler::_usb_config = {
    "Ardu_USB", USB_SS * sizeof(StackType_t), USB_PRIO, 1U << 1
};

Scheduler::Scheduler() :
    AP_HAL_FreeRTOS::Scheduler(_config)
{
}

void Scheduler::init()
{
    AP_HAL_FreeRTOS::Scheduler::init();
    create_task(usb_device_task, _usb_config, this);
}

void Scheduler::main_task_init()
{
#if HAL_RCOUT_DRIVER_ENABLED
    hal.rcout->init();
#endif
}

void Scheduler::timer_task_init()
{
#if PICO_MULTICORE
    multicore_lockout_victim_init();
#endif
}

void Scheduler::io_task_init()
{
#if PICO_MULTICORE
    multicore_lockout_victim_init();
#endif
    set_current_thread_priority(IO_PRIO);
}

void Scheduler::usb_device_task(void *context)
{
    auto *scheduler = static_cast<Scheduler *>(context);
    scheduler->wait_for_system_initialized();
    AP_HAL_FreeRTOS::Platform::init_usb_cdc_drivers();
    for (;;) {
        USBSerialDriver::backend_task();
        scheduler->delay(1);
    }
}

void Scheduler::delay_without_scheduler(uint16_t ms)
{
    sleep_ms(ms);
}

void Scheduler::delay_microseconds(uint16_t us)
{
    sleep_us(us);
}

void Scheduler::reboot(bool hold_in_bootloader)
{
    if (hold_in_bootloader) {
        reset_usb_boot(0, 0);
    } else {
        watchdog_reboot(0, 0, 0);
    }
}
