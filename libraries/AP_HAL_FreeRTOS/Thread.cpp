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

#include <AP_HAL/AP_HAL.h>

#include "Thread.h"
#include "Port.h"
#include <cstdlib>

namespace AP_HAL_FreeRTOS {

static void thread_create_trampoline(void *context)
{
    AP_HAL::MemberProc *proc = static_cast<AP_HAL::MemberProc *>(context);
    (*proc)();
    free(proc);
    vTaskDelete(nullptr);
}

bool thread_create(AP_HAL::MemberProc proc, const char *name,
                   uint32_t stack_size_bytes, uint8_t priority)
{
    if (stack_size_bytes == 0 || priority >= configMAX_PRIORITIES) {
        return false;
    }

    // Keep the delegate alive until the thread returns.
    AP_HAL::MemberProc *context = static_cast<AP_HAL::MemberProc *>(calloc(1, sizeof(proc)));
    if (context == nullptr) {
        return false;
    }
    *context = proc;

    TaskHandle_t handle;
    if (!Port::create_task(thread_create_trampoline, name, stack_size_bytes,
                           context, priority, &handle)) {
        free(context);
        return false;
    }
    return true;
}

} // namespace AP_HAL_FreeRTOS