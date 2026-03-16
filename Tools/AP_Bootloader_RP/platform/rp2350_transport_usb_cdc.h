#pragma once

#include <stddef.h>
#include <stdint.h>

#include "pico/sync.h"

namespace AP_Bootloader_RP {

class rp2350_transport_usb_cdc {
public:
    rp2350_transport_usb_cdc();

    bool init();
    bool read_byte(uint8_t &byte, uint32_t timeout_ms);
    bool write(const uint8_t *data, size_t length);
    void flush();

private:
    bool _initialized;
    mutex_t _write_mutex;
};

} // namespace AP_Bootloader_RP