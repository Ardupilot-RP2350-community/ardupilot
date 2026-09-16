#pragma once

#include <AP_HAL/AP_HAL.h>
#include <AP_Filesystem/AP_Filesystem_NAND_LittleFS.h>

namespace AP_HAL_FreeRTOS {

struct HALConfig {
    AP_HAL::UARTDriver *serial0 {};
    AP_HAL::UARTDriver *serial1 {};
    AP_HAL::UARTDriver *serial2 {};
    AP_HAL::UARTDriver *serial3 {};
    AP_HAL::UARTDriver *serial4 {};
    AP_HAL::UARTDriver *serial5 {};
    AP_HAL::UARTDriver *serial6 {};
    AP_HAL::UARTDriver *serial7 {};
    AP_HAL::UARTDriver *serial8 {};
    AP_HAL::UARTDriver *serial9 {};

    AP_HAL::I2CDeviceManager *i2c {};
    AP_HAL::SPIDeviceManager *spi {};
    AP_HAL::WSPIDeviceManager *wspi {};

    AP_HAL::AnalogIn *analogin {};
    AP_HAL::Storage *storage {};
    AP_HAL::UARTDriver *console {};

    AP_HAL::GPIO *gpio {};
    AP_HAL::RCInput *rcin {};
    AP_HAL::RCOutput *rcout {};

    AP_HAL::Scheduler *scheduler {};
    AP_HAL::Util *util {};
    AP_HAL::OpticalFlow *opticalflow {};
    AP_HAL::Flash *flash {};

#if HAL_WITH_DSP
    AP_HAL::DSP *dsp {};
#endif
};

namespace Platform {

const HALConfig &get_hal_config();
AP_Filesystem_NAND_Device* get_nand_device();
void init_usb_cdc_drivers();
void init();

} // namespace Platform

} // namespace AP_HAL_FreeRTOS