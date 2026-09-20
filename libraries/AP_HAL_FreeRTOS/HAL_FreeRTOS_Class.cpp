#include "HAL_FreeRTOS_Class.h"
#include "AP_HAL_FreeRTOS_Private.h"

extern const AP_HAL::HAL& hal;

using namespace AP_HAL_FreeRTOS;

HAL_FreeRTOS::HAL_FreeRTOS()
    : HAL_FreeRTOS(Platform::get_hal_config())
{
}

HAL_FreeRTOS::HAL_FreeRTOS(const HALConfig &c)
    : AP_HAL::HAL(
        c.serial0,
        c.serial1,
        c.serial2,
        c.serial3,
        c.serial4,
        c.serial5,
        c.serial6,
        c.serial7,
        c.serial8,
        c.serial9,
        c.i2c,
        c.spi,
        c.wspi,
        c.analogin,
        c.storage,
        c.console,
        c.gpio,
        c.rcin,
        c.rcout,
        c.scheduler,
        c.util,
        c.opticalflow,
        c.flash,
#if AP_SIM_ENABLED
        &xsimstate,
#endif
#if HAL_WITH_DSP
        c.dsp,
#endif
        nullptr)
{
}

void HAL_FreeRTOS::run(int argc, char * const argv[], Callbacks *callbacks) const
{
    (void)argc;
    (void)argv;

    Platform::init();

    auto *scheduler =
        static_cast<Scheduler *>(hal.scheduler);

    scheduler->set_callbacks(callbacks);
    scheduler->init();
    scheduler->start();
}

const AP_HAL::HAL& AP_HAL::get_HAL() {
    static HAL_FreeRTOS hal_freertos;
    return hal_freertos;
}

AP_HAL::HAL& AP_HAL::get_HAL_mutable() {
    return const_cast<AP_HAL::HAL&>(get_HAL());
}

void AP_HAL::init()
{
}