#include "LED.h"

#if AP_NOTIFY_GPIO_LED_1_ENABLED

extern const AP_HAL::HAL& hal;

namespace AP_HAL_FreeRTOS::RP2350 {

bool LED::init(void) {
	bool status = _led.init();
	if (status) {
		hal.scheduler->register_io_process(FUNCTOR_BIND_MEMBER(&LED::update, void));
	} 
	return status;
}

void LED::update(void) {
	_led.update();
}

} // namespace AP_HAL_FreeRTOS::RP2350

#endif // AP_NOTIFY_GPIO_LED_1_ENABLED
