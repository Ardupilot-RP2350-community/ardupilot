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

#include "Port.h"

bool AP_HAL_FreeRTOS::Port::create_task(TaskFunction_t entry, const char *name,
                                        uint32_t stack_size_bytes, void *context,
                                        UBaseType_t priority, TaskHandle_t *handle, UBaseType_t affinity_mask)
{
    if (entry == nullptr || stack_size_bytes == 0 || priority >= configMAX_PRIORITIES) {
        return false;
    }
    const uint32_t stack_depth = stack_size_bytes / sizeof(StackType_t) +
                                 (stack_size_bytes % sizeof(StackType_t) != 0);
    if (static_cast<configSTACK_DEPTH_TYPE>(stack_depth) != stack_depth) {
        return false;
    }
    if (affinity_mask != 0) {
#if configNUMBER_OF_CORES > 1 && configUSE_CORE_AFFINITY == 1
        // Set affinity before the task becomes runnable, including on a running kernel.
        return xTaskCreateAffinitySet(entry, name, stack_depth, context, priority,
                                      affinity_mask, handle) == pdPASS;
#else
        if (affinity_mask != 1 || configNUMBER_OF_CORES != 1) {
            return false;
        }
#endif
    }
    return xTaskCreate(entry, name, stack_depth, context, priority, handle) == pdPASS;
}

void AP_HAL_FreeRTOS::Port::yield_from_isr(BaseType_t higher_priority_task_woken)
{
    portYIELD_FROM_ISR(higher_priority_task_woken);
}
