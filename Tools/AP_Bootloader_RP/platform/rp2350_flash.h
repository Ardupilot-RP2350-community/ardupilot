#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AP_Bootloader_RP {

bool rp2350_flash_erase_app();
bool rp2350_flash_write(uint32_t address, const uint8_t *data, size_t length);
bool rp2350_flash_read(uint32_t address, uint8_t *data, size_t length);
uint32_t rp2350_flash_crc();

} // namespace AP_Bootloader_RP