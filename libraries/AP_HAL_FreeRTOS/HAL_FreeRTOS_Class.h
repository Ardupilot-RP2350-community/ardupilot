#pragma once

#include <AP_HAL/AP_HAL.h>
#include "Platform.h"

class HAL_FreeRTOS : public AP_HAL::HAL {
    HAL_FreeRTOS(const AP_HAL_FreeRTOS::HALConfig &c);
public:
    HAL_FreeRTOS();
    void run(int argc, char* const* argv, Callbacks* callbacks) const override;
};