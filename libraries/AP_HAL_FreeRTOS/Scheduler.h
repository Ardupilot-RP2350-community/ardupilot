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

#include <AP_HAL/AP_HAL.h>
#include "Semaphores.h"
#include <atomic>
#include <FreeRTOS.h>
#include <task.h>

namespace AP_HAL_FreeRTOS
{

class Scheduler : public AP_HAL::Scheduler
{
public:
    static constexpr uint8_t MAX_PRIO = configMAX_PRIORITIES - 1;
    static constexpr uint8_t MIN_PRIO = tskIDLE_PRIORITY + 1;
    static constexpr uint8_t NUM_PRIORITY_BASES = PRIORITY_NET + 1;

    struct TaskConfig {
        const char *name;
        uint32_t stack_size_bytes;
        uint8_t priority;
        // Zero allows any core; otherwise each bit selects an allowed core.
        UBaseType_t affinity_mask;
    };

    struct Config {
        TaskConfig main;
        TaskConfig timer;
        TaskConfig uart;
        TaskConfig io;
        uint8_t priorities[NUM_PRIORITY_BASES];
        uint32_t thread_stack_extra_bytes;
        uint8_t thread_stack_unit_bytes;
        uint16_t timer_period_ms;
        uint16_t io_period_ms;
        bool run_delay_callbacks;
    };

    // Configuration must outlive the scheduler and its tasks.
    explicit Scheduler(const Config &config);
    CLASS_NO_COPY(Scheduler);

    void init() override;
    void delay(uint16_t ms) override;
    void register_timer_process(AP_HAL::MemberProc proc) override;
    void register_io_process(AP_HAL::MemberProc proc) override;
    void register_timer_failsafe(AP_HAL::Proc proc, uint32_t period_us) override;
    void set_system_initialized() override;
    bool is_system_initialized() override;
    bool in_main_thread() const override;
    bool thread_create(AP_HAL::MemberProc proc, const char *name, uint32_t stack_size,
                       priority_base base, int8_t priority) override;

    void set_callbacks(AP_HAL::HAL::Callbacks *callbacks)
    {
        _callbacks = callbacks;
    }
    void start();

protected:
    // Hooks execute in the corresponding worker, before its normal work starts.
    virtual void main_task_init() {}
    virtual void timer_task_init() {}
    virtual void io_task_init() {}
    virtual void main_task_yield();
    virtual void delay_without_scheduler(uint16_t ms) = 0;

    void create_task(TaskFunction_t entry, const TaskConfig &config, void *context,
                     TaskHandle_t *handle = nullptr);
    void wait_for_system_initialized();
    void set_current_thread_priority(uint8_t priority);

    void run_timer_processes();
    void run_io_processes();

private:
    static constexpr uint8_t MAX_PROCESSES = 10;
    struct Processes {
        Semaphore semaphore;
        AP_HAL::MemberProc callbacks[MAX_PROCESSES] {};
        std::atomic<uint8_t> count {0};
    };

    static void register_process(Processes &processes, AP_HAL::MemberProc proc);
    static void run_processes(const Processes &processes);
    uint8_t calculate_thread_priority(priority_base base, int8_t priority) const;

    static void main_task(void *context);
    static void timer_task(void *context);
    static void uart_task(void *context);
    static void io_task(void *context);

    const Config &_config;
    Processes _timer_processes;
    Processes _io_processes;
    std::atomic<AP_HAL::Proc> _failsafe {nullptr};
    std::atomic<bool> _initialized {false};
    TaskHandle_t _main_task_handle = nullptr;
    AP_HAL::HAL::Callbacks *_callbacks = nullptr;
    bool _tasks_created = false;
};

} // namespace AP_HAL_FreeRTOS
