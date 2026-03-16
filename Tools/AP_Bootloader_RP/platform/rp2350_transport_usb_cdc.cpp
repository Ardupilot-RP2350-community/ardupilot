#include "rp2350_transport_usb_cdc.h"

#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "pico/time.h"

namespace AP_Bootloader_RP {

rp2350_transport_usb_cdc::rp2350_transport_usb_cdc() :
    _initialized(false),
    _write_mutex{}
{
}

bool rp2350_transport_usb_cdc::init()
{
    if (_initialized) {
        return true;
    }

    stdio_usb_init();
    mutex_init(&_write_mutex);
    _initialized = true;

    return true;
}

bool rp2350_transport_usb_cdc::read_byte(uint8_t &byte, uint32_t timeout_ms)
{
    if (!_initialized) {
        return false;
    }

    if (timeout_ms == 0) {
        const int ch = getchar_timeout_us(0);
        if (ch == PICO_ERROR_TIMEOUT) {
            return false;
        }
        byte = (uint8_t)ch;
        return true;
    }

    const absolute_time_t deadline = make_timeout_time_ms(timeout_ms);

    while (!time_reached(deadline)) {
        const int ch = getchar_timeout_us(0);
        if (ch != PICO_ERROR_TIMEOUT) {
            byte = (uint8_t)ch;
            return true;
        }
        sleep_us(50);
    }

    return false;
}

bool rp2350_transport_usb_cdc::write(const uint8_t *data, size_t length)
{
    if (!_initialized || data == nullptr || length == 0) {
        return false;
    }

    mutex_enter_blocking(&_write_mutex);

    for (size_t i = 0; i < length; i++) {
        putchar_raw((char)data[i]);
    }

    mutex_exit(&_write_mutex);
    return true;
}

void rp2350_transport_usb_cdc::flush()
{
    if (!_initialized) {
        return;
    }

    mutex_enter_blocking(&_write_mutex);
    stdio_flush();
    mutex_exit(&_write_mutex);
}

} // namespace AP_Bootloader_RP
