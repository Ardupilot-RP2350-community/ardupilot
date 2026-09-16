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

#include <AP_HAL/AP_HAL_Namespace.h>
#include <stdint.h>

namespace AP_HAL_FreeRTOS {

// The platform scheduler supplies its stack allowance and native priority.
bool thread_create(AP_HAL::MemberProc proc, const char *name,
                   uint32_t stack_size_bytes, uint8_t priority);
} // namespace FreeRTOS
