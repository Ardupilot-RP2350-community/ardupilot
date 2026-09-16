#pragma once

#include <AP_HAL_FreeRTOS/AP_HAL_FreeRTOS.h>
#include <AP_HAL_FreeRTOS/AP_HAL_FreeRTOS_Private.h>
#include <AP_HAL_Empty/AP_HAL_Empty_Private.h>
#include <AP_Notify/GPIO_LED_1.h>
#include "RP2350_Private.h"

#if AP_NOTIFY_GPIO_LED_1_ENABLED

namespace AP_HAL_FreeRTOS::RP2350 {

class LED
{
public:
	bool init(void);
	void update(void);
private:
	GPIO_LED_1 _led;
};
} // namespace AP_HAL_FreeRTOS::RP2350

#endif // AP_NOTIFY_GPIO_LED_1_ENABLED
