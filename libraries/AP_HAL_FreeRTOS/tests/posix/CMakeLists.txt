cmake_minimum_required(VERSION 3.16)
project(freertos_scheduler_tests C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(ARDUPILOT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
set(FREERTOS_KERNEL_PATH "$ENV{FREERTOS_KERNEL_PATH}" CACHE PATH "FreeRTOS kernel source directory")
set(POSIX_PORT "${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/Posix")
if(NOT EXISTS "${POSIX_PORT}/port.c")
    message(FATAL_ERROR "Set FREERTOS_KERNEL_PATH to a kernel with the GCC POSIX port")
endif()

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
add_subdirectory("${ARDUPILOT_ROOT}/modules/gtest" gtest EXCLUDE_FROM_ALL)
set(COMMON "${ARDUPILOT_ROOT}/libraries/AP_HAL_FreeRTOS_Common")
add_executable(test_scheduler
    test_scheduler.cpp
    ${COMMON}/Scheduler.cpp ${COMMON}/Thread.cpp
    ${COMMON}/Semaphores.cpp ${COMMON}/Port.cpp
    ${ARDUPILOT_ROOT}/libraries/AP_HAL/Scheduler.cpp
    ${ARDUPILOT_ROOT}/libraries/AP_HAL/Semaphores.cpp
    ${FREERTOS_KERNEL_PATH}/tasks.c ${FREERTOS_KERNEL_PATH}/queue.c
    ${FREERTOS_KERNEL_PATH}/list.c ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
    ${POSIX_PORT}/port.c ${POSIX_PORT}/utils/wait_for_event.c)
target_include_directories(test_scheduler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} ${ARDUPILOT_ROOT}/libraries ${ARDUPILOT_ROOT}/tests
    ${FREERTOS_KERNEL_PATH}/include ${POSIX_PORT} ${POSIX_PORT}/utils)
target_compile_definitions(test_scheduler PRIVATE
    CONFIG_HAL_BOARD=HAL_BOARD_EMPTY AP_SIM_ENABLED=0 HAL_WITH_DSP=0
    AP_SCRIPTING_ENABLED=0 AP_MATH_ALLOW_DOUBLE_FUNCTIONS=1 __AP_LINE__=__LINE__
    # Omit WithSemaphore's peripheral diagnostic bookkeeping in this host HAL.
    HAL_BOOTLOADER_BUILD=1)
target_compile_options(test_scheduler PRIVATE
    -ffunction-sections -fdata-sections
    "$<$<COMPILE_LANGUAGE:CXX>:-include${CMAKE_CURRENT_SOURCE_DIR}/hal_types.h>")
target_link_options(test_scheduler PRIVATE -Wl,--gc-sections -Wl,--wrap=xTaskCreate)
target_link_libraries(test_scheduler PRIVATE gtest pthread)
enable_testing()
add_test(NAME freertos_scheduler COMMAND test_scheduler)
set_tests_properties(freertos_scheduler PROPERTIES TIMEOUT 20)
