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

#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>

namespace AP_HAL_FreeRTOS {

namespace Port {
// The default backend uses standard FreeRTOS. Stack sizes are always in bytes.
bool create_task(TaskFunction_t entry, const char *name, uint32_t stack_size_bytes,
                 void *context, UBaseType_t priority, TaskHandle_t *handle,
                 UBaseType_t affinity_mask = 0);

void yield_from_isr(BaseType_t higher_priority_task_woken);

} // namespace Port

} // namespace FreeRTOS