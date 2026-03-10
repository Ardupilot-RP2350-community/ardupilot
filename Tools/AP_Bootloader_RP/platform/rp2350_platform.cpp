#include "rp2350_platform.h"

#include "rp2350_boardinfo.h"
#include "rp2350_flash.h"

namespace AP_Bootloader_RP {

bool rp2350_platform::init()
{
    return _transport.init();
}

bool rp2350_platform::flash_erase_app()
{
    return rp2350_flash_erase_app();
}

bool rp2350_platform::flash_write(uint32_t address, const uint8_t *data, size_t length)
{
    return rp2350_flash_write(address, data, length);
}

uint32_t rp2350_platform::flash_crc()
{
    return rp2350_flash_crc();
}

board_info rp2350_platform::get_board_info() const
{
    return rp2350_get_board_info();
}

bool rp2350_platform::transport_read(uint8_t &byte, uint32_t timeout_ms)
{
    return _transport.read_byte(byte, timeout_ms);
}

bool rp2350_platform::transport_write(const uint8_t *data, size_t length)
{
    return _transport.write(data, length);
}

void rp2350_platform::transport_flush()
{
    _transport.flush();
}

void rp2350_platform::jump_to_application()
{
    // TODO: validate vector table and jump to application image.
}

} // namespace AP_Bootloader_RP