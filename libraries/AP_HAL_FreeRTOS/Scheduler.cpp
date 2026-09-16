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
#include "Port.h"
#include "Thread.h"

extern const AP_HAL::HAL& hal;

using namespace AP_HAL_FreeRTOS;

constexpr uint8_t Scheduler::MAX_PRIO;
constexpr uint8_t Scheduler::MIN_PRIO;
constexpr uint8_t Scheduler::NUM_PRIORITY_BASES;
constexpr uint8_t Scheduler::MAX_PROCESSES;

Scheduler::Scheduler(const Config &config) :
    _config(config)
{
}

void Scheduler::create_task(TaskFunction_t entry, const TaskConfig &config,
                            void *context, TaskHandle_t *handle)
{
    if (!Port::create_task(entry, config.name, config.stack_size_bytes,
                           context, config.priority, handle, config.affinity_mask)) {
        AP_HAL::panic("FreeRTOS: cannot create %s", config.name);
    }
}

void Scheduler::init()
{
    if (_tasks_created || _callbacks == nullptr) {
        AP_HAL::panic("FreeRTOS: invalid scheduler initialization");
    }
    _tasks_created = true;
    create_task(main_task, _config.main, this, &_main_task_handle);
    create_task(timer_task, _config.timer, this);
    create_task(uart_task, _config.uart, this);
    create_task(io_task, _config.io, this);
}

void Scheduler::start()
{
    vTaskStartScheduler();
    AP_HAL::panic("FreeRTOS: scheduler stopped");
}

void Scheduler::main_task(void *context)
{
    auto *scheduler = static_cast<Scheduler *>(context);
    scheduler->main_task_init();
    scheduler->_callbacks->setup();
    scheduler->set_system_initialized();
    for (;;) {
        scheduler->_callbacks->loop();
        scheduler->main_task_yield();
    }
}

void Scheduler::main_task_yield()
{
    taskYIELD();
}

void Scheduler::timer_task(void *context)
{
    auto *scheduler = static_cast<Scheduler *>(context);
    scheduler->timer_task_init();
    for (;;) {
        scheduler->run_timer_processes();
        scheduler->delay(scheduler->_config.timer_period_ms);
    }
}

void Scheduler::io_task(void *context)
{
    auto *scheduler = static_cast<Scheduler *>(context);
    scheduler->io_task_init();
    for (;;) {
        scheduler->run_io_processes();
        scheduler->delay(scheduler->_config.io_period_ms);
    }
}

void Scheduler::uart_task(void *context)
{
    auto *scheduler = static_cast<Scheduler *>(context);
    scheduler->wait_for_system_initialized();
    for (;;) {
        for (uint8_t i = 0; i < hal.num_serial; i++) {
            if (hal.serial(i) != nullptr) {
                hal.serial(i)->_timer_tick();
            }
        }
        if (hal.console != nullptr) {
            hal.console->_timer_tick();
        }
        scheduler->delay(1);
    }
}

void Scheduler::wait_for_system_initialized()
{
    while (!is_system_initialized()) {
        delay(10);
    }
}

void Scheduler::set_current_thread_priority(uint8_t priority)
{
    if (priority < MIN_PRIO || priority > MAX_PRIO) {
        AP_HAL::panic("FreeRTOS: invalid task priority");
    }
    vTaskPrioritySet(nullptr, priority);
}

void Scheduler::register_process(Processes &processes, AP_HAL::MemberProc proc)
{
    if (!proc) {
        return;
    }
    WITH_SEMAPHORE(processes.semaphore);
    const uint8_t count = processes.count.load(std::memory_order_relaxed);
    for (uint8_t i = 0; i < count; i++) {
        if (processes.callbacks[i] == proc) {
            return;
        }
    }
    if (count < MAX_PROCESSES) {
        processes.callbacks[count] = proc;
        // Published slots are immutable. Readers can invoke them without a lock.
        processes.count.store(count + 1, std::memory_order_release);
    }
}

void Scheduler::run_processes(const Processes &processes)
{
    const uint8_t count = processes.count.load(std::memory_order_acquire);
    for (uint8_t i = 0; i < count; i++) {
        processes.callbacks[i]();
    }
}

void Scheduler::register_timer_process(AP_HAL::MemberProc proc)
{
    register_process(_timer_processes, proc);
}

void Scheduler::register_io_process(AP_HAL::MemberProc proc)
{
    register_process(_io_processes, proc);
}

void Scheduler::register_timer_failsafe(AP_HAL::Proc proc, uint32_t period_us)
{
    // Preserve the RP policy: the failsafe runs on every timer iteration.
    (void)period_us;
    _failsafe.store(proc, std::memory_order_release);
}

void Scheduler::run_timer_processes()
{
    run_processes(_timer_processes);
    const AP_HAL::Proc failsafe = _failsafe.load(std::memory_order_acquire);
    if (failsafe != nullptr) {
        failsafe();
    }
}

void Scheduler::run_io_processes()
{
    run_processes(_io_processes);
}

void Scheduler::set_system_initialized()
{
    _initialized.store(true, std::memory_order_release);
}

bool Scheduler::is_system_initialized()
{
    return _initialized.load(std::memory_order_acquire);
}

bool Scheduler::in_main_thread() const
{
    return _main_task_handle != nullptr && xTaskGetCurrentTaskHandle() == _main_task_handle;
}

void Scheduler::delay(uint16_t ms)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) {
        delay_without_scheduler(ms);
        return;
    }
    if (!_config.run_delay_callbacks || !in_main_thread() || ms < _min_delay_cb_ms) {
        vTaskDelay(pdMS_TO_TICKS(ms));
        return;
    }

    const uint64_t start_us = AP_HAL::micros64();
    const TickType_t ticks = pdMS_TO_TICKS(1) > 0 ? pdMS_TO_TICKS(1) : 1;
    while (AP_HAL::micros64() - start_us < uint32_t(ms) * 1000U) {
        vTaskDelay(ticks);
        call_delay_cb();
    }
}

uint8_t Scheduler::calculate_thread_priority(priority_base base, int8_t priority) const
{
    if (base < PRIORITY_BOOST || base >= NUM_PRIORITY_BASES) {
        return _config.priorities[PRIORITY_IO];
    }
    const int16_t adjusted = _config.priorities[base] + int16_t(priority);
    if (adjusted < MIN_PRIO) {
        return MIN_PRIO;
    }
    if (adjusted > MAX_PRIO) {
        return MAX_PRIO;
    }
    return adjusted;
}

bool Scheduler::thread_create(AP_HAL::MemberProc proc, const char *name,
                              uint32_t stack_size, priority_base base, int8_t priority)
{
    const uint8_t unit = _config.thread_stack_unit_bytes;
    if (unit == 0 || stack_size > (UINT32_MAX - _config.thread_stack_extra_bytes) / unit) {
        return false;
    }
    const uint32_t bytes = stack_size * unit + _config.thread_stack_extra_bytes;
    return AP_HAL_FreeRTOS::thread_create(proc, name, bytes, calculate_thread_priority(base, priority));
}
