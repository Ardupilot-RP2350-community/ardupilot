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

#pragma once

#include <AP_HAL_FreeRTOS/Scheduler.h>
#include <AP_HAL_FreeRTOS/AP_HAL_FreeRTOS_Namespace.h>

namespace AP_HAL_FreeRTOS::RP2350 {

class Scheduler : public AP_HAL_FreeRTOS::Scheduler
{
public:
    Scheduler();
    void init() override;
    void delay_microseconds(uint16_t us) override;
    void reboot(bool hold_in_bootloader) override;

    // Priorities (the higher the number, the higher the priority)
    static const int MAIN_PRIO    = MAX_PRIO - 2;   // Main loop (Core 0)
    static const int TIMER_PRIO   = MAX_PRIO;       // Timers (must be higher than MAIN for accuracy)
    static const int USB_PRIO     = MAX_PRIO;       // USB CDC
    static const int UART_PRIO    = MAX_PRIO - 1;   // UART (fast output, but below the timer)
    static const int SPI_PRIORITY = MAX_PRIO - 3;   // Fast sensors (IMU)
    static const int RCOUT_PRIO   = MAX_PRIO - 15;  // PWM output
    static const int RCIN_PRIO    = MAX_PRIO - 15;  // Signal input
    static const int I2C_PRIORITY = MAX_PRIO - 17;  // I2C is usually slower than SPI
    static const int IO_PRIO      = MAX_PRIO - 20;  // Background operations
    static const int STORAGE_PRIO = MAX_PRIO - 21;  // Flash recording (lowest, may be long)

    // Stack sizes (in words, 1 word = 4 bytes on RP2350)
    // Values increased by 20% because Cortex-M33 with FPU needs more space when switching context
    static const int MAIN_SS      = 3072; // 12 KB (INS init + printf need headroom on CM33)
    static const int TIMER_SS     = 1024; // 4 KB
    static const int UART_SS      = 1024; // 4 KB
    static const int DEVICE_SS    = 1536; // 6 KB (for SPI/I2C buses)
    static const int RCIN_SS      = 768;  // 3 KB
    static const int RCOUT_SS     = 512;  // 2 KB
    static const int IO_SS        = 1024; // 4 KB
    static const int STORAGE_SS   = 1024; // 4 KB
    static const int USB_SS       = 1024; // 4 KB

private:
    static const Config _config;
    static const TaskConfig _usb_config;

    void main_task_init() override;
    void timer_task_init() override;
    void io_task_init() override;
    void delay_without_scheduler(uint16_t ms) override;
    static void usb_device_task(void *context);
};
} // namespace AP_HAL_FreeRTOS::RP2350