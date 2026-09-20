#include <AP_HAL_FreeRTOS/Platform.h>

#include <assert.h>
#include <AP_HAL_Empty/AP_HAL_Empty_Private.h>
#include <AP_HAL_FreeRTOS/AP_HAL_FreeRTOS_Private.h>
#include "RP2350_Private.h"
#include "pico/stdlib.h"

using namespace AP_HAL_FreeRTOS::RP2350;

namespace AP_HAL_FreeRTOS::Platform {

#ifndef HAL_RP_USB_CDC_CONNECT_TIMEOUT_MS
#define HAL_RP_USB_CDC_CONNECT_TIMEOUT_MS 1500U
#endif

#ifndef HAL_RP_USB_CDC_FALLBACK_TO_UART0
#define HAL_RP_USB_CDC_FALLBACK_TO_UART0 0
#endif

static UARTDriver serial0Driver(uart0, UART0_TX, UART0_RX, 0, true);
static UARTDriver serial1Driver(uart1, UART1_TX, UART1_RX, 1);
static USBSerialDriver usbConsoleDriver(0);
static USBSerialDriver usbMavlinkDriver(1);

#if HAL_SERIAL2_DRIVER_ENABLED
static PIOSerialDriver serial2Driver(PIO_UART, UART2_TX, UART2_RX);
#endif

#if HAL_SERIAL3_DRIVER_ENABLED
static PIOSerialDriver serial3Driver(PIO_UART, UART3_TX, UART3_RX);
#endif

#if HAL_SERIAL5_DRIVER_ENABLED
static PIOSerialDriver serial5Driver(PIO_UART, UART5_TX, UART5_RX);
#endif

#if HAL_SERIAL6_DRIVER_ENABLED
static PIOSerialDriver serial6Driver(PIO_UART, UART6_TX, UART6_RX);
#endif

#if HAL_SERIAL7_DRIVER_ENABLED
static PIOSerialDriver serial7Driver(PIO_UART, UART7_TX, UART7_RX);
#endif

#if HAL_SERIAL8_DRIVER_ENABLED
static PIOSerialDriver serial8Driver(PIO_UART, UART8_TX, UART8_RX);
#endif

#if HAL_SERIAL9_DRIVER_ENABLED
static PIOSerialDriver serial9Driver(PIO_UART, UART9_TX, UART9_RX);
#endif

#if HAL_SPI_DEVICE_DRIVER_ENABLED
static SPIDeviceManager spiDeviceManager;
#endif

#if HAL_I2C_DEVICE_DRIVER_ENABLED
static I2CDeviceManager i2cDeviceManager;
#endif

#if HAL_ANALOGIN_DRIVER_ENABLED
static AnalogIn analogIn;
#endif

#ifdef HAL_USE_EMPTY_STORAGE
static Empty::Storage storageDriver;
#elif HAL_STORAGE_DRIVER_ENABLED
static Storage storageDriver;
#endif

static GPIO gpioDriver;

#if HAL_RCIN_DRIVER_ENABLED
static RCInput rcinDriver;
#endif

#if HAL_RCOUT_DRIVER_ENABLED
static RCOutput rcoutDriver;
#endif

static AP_HAL_FreeRTOS::RP2350::Scheduler schedulerInstance;
static Util utilInstance;
static Empty::OpticalFlow opticalFlowDriver;

#if HAL_FLASH_DRIVER_ENABLED
static Flash flashDriver;
#endif

#if HAL_NAND_FLASH_ENABLED
static NAND_PIO_Driver nandPioDriver;
#endif

#if HAL_USE_WSPI == TRUE && defined(HAL_WSPI_DEVICE_LIST)
static WSPIDeviceManager wspiDeviceManager;
#endif

#if AP_NOTIFY_GPIO_LED_1_ENABLED
static LED greenLed;
#endif

static const HALConfig halConfig {

    .serial0 = &serial0Driver,
    .serial1 = &serial1Driver,

#if HAL_SERIAL2_DRIVER_ENABLED
    .serial2 = &serial2Driver,
#endif
#if HAL_SERIAL3_DRIVER_ENABLED
    .serial3 = &serial3Driver,
#endif
#if HAL_SERIAL4_DRIVER_ENABLED
    .serial4 = &usbMavlinkDriver,
#endif
#if HAL_SERIAL5_DRIVER_ENABLED
    .serial5 = &serial5Driver,
#endif
#if HAL_SERIAL6_DRIVER_ENABLED
    .serial6 = &serial6Driver,
#endif
#if HAL_SERIAL7_DRIVER_ENABLED
    .serial7 = &serial7Driver,
#endif
#if HAL_SERIAL8_DRIVER_ENABLED
    .serial8 = &serial8Driver,
#endif
#if HAL_SERIAL9_DRIVER_ENABLED
    .serial9 = &serial9Driver,
#endif

#if HAL_I2C_DEVICE_DRIVER_ENABLED
    .i2c = &i2cDeviceManager,
#endif

#if HAL_SPI_DEVICE_DRIVER_ENABLED
    .spi = &spiDeviceManager,
#endif

#if HAL_SPI_DEVICE_DRIVER_ENABLED
    .wspi = &wspiDeviceManager,
#endif

#if HAL_ANALOGIN_DRIVER_ENABLED
    .analogin = &analogIn,
#endif

#if HAL_STORAGE_DRIVER_ENABLED
    .storage = &storageDriver,
#endif

#if HAL_RP_CONSOLE_USB_CDC
    .console = &usbConsoleDriver,
#else
    .console = &serial0Driver,
#endif
    .gpio = &gpioDriver,

#if HAL_RCIN_DRIVER_ENABLED
    .rcin = &rcinDriver,
#endif

#if HAL_RCOUT_DRIVER_ENABLED
    .rcout = &rcoutDriver,
#endif

    .scheduler = &schedulerInstance,
    .util = &utilInstance,
    .opticalflow = &opticalFlowDriver,

#if HAL_FLASH_DRIVER_ENABLED
    .flash = &flashDriver,
#endif

#if HAL_WITH_DSP
    .dsp = &dspDriver,
#endif
};

const HALConfig &get_hal_config()
{
    return halConfig;
}

AP_Filesystem_NAND_Device* get_nand_device()
{
#if HAL_NAND_FLASH_ENABLED
    return &nandPioDriver;
#else
    return nullptr;
#endif
}

void init()
{
#if HAL_RP_CONSOLE_USB_CDC == 0
    if (console) {
        console->begin(115200);
    }
#endif

#if HAL_NAND_FLASH_ENABLED
    nandPioDriver.init(NAND_FLASH_IO_BASE, NAND_FLASH_SCLK, NAND_FLASH_CS);
#endif

#if AP_NOTIFY_GPIO_LED_1_ENABLED
    greenLed.init();
#endif
}

void init_usb_cdc_drivers()
{
    usbConsoleDriver.begin(115200);
    usbMavlinkDriver.begin(115200);
}

} // namespace AP_HAL_FreeRTOS::Platform
