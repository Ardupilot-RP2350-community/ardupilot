#include "rp2350_flash.h"
#include <string.h>
#include "hardware/flash.h"
#include "hardware/sync.h"

#ifndef AP_BOOTLOADER_RP_FW_SIZE
#define AP_BOOTLOADER_RP_FW_SIZE (2048 * 1024)
#endif

#ifndef FLASH_BASE_ADDR
#define FLASH_BASE_ADDR 0x10000000
#endif

#ifndef BOOTLOADER_SIZE
#define BOOTLOADER_SIZE (64 * 1024)
#endif

namespace AP_Bootloader_RP {

namespace {

static bool flash_initialized = false;

static uint32_t crc32_update(uint32_t crc, uint8_t data)
{
    crc ^= data;
    for (uint8_t i = 0; i < 8; i++) {
        const uint32_t mask = -(crc & 1U);
        crc = (crc >> 1U) ^ (0xEDB88320U & mask);
    }
    return crc;
}

static inline bool is_address_valid(uint32_t address)
{
    uint32_t app_start = FLASH_BASE_ADDR + BOOTLOADER_SIZE;
    uint32_t app_end = app_start + AP_BOOTLOADER_RP_FW_SIZE;
    return (address >= app_start && address < app_end);
}

static inline bool is_length_valid(uint32_t address, size_t length)
{
    if (length == 0) return false;
    uint32_t app_end = FLASH_BASE_ADDR + BOOTLOADER_SIZE + AP_BOOTLOADER_RP_FW_SIZE;
    return (address + length <= app_end);
}

} // namespace

bool rp2350_flash_erase_app()
{
    uint32_t flash_offs = BOOTLOADER_SIZE;
    uint32_t size = AP_BOOTLOADER_RP_FW_SIZE;

    if ((flash_offs % FLASH_SECTOR_SIZE) != 0 || (size % FLASH_SECTOR_SIZE) != 0) {
        return false;
    }

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_offs, size);
    restore_interrupts(ints);

    return true;
}

bool rp2350_flash_write(uint32_t address, const uint8_t *data, size_t length)
{
    if (data == nullptr || !is_address_valid(address) ||
        !is_length_valid(address, length) || (length % FLASH_PAGE_SIZE) != 0) {
        return false;
    }

    uint32_t flash_offs = address - FLASH_BASE_ADDR;

    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(flash_offs, data, length);
    restore_interrupts(ints);

    return true;
}

bool rp2350_flash_read(uint32_t address, uint8_t *data, size_t length)
{
    if (data == nullptr || !is_address_valid(address) || !is_length_valid(address, length)) {
        return false;
    }

    memcpy(data, (void*)address, length);
    return true;
}

uint32_t rp2350_flash_crc()
{
    if (AP_BOOTLOADER_RP_FW_SIZE == 0) {
        return 0;
    }

    const uint32_t base_addr = FLASH_BASE_ADDR + BOOTLOADER_SIZE;
    uint32_t crc = 0xFFFFFFFFU;
    static uint8_t buffer[FLASH_PAGE_SIZE];

    for (uint32_t offset = 0; offset < AP_BOOTLOADER_RP_FW_SIZE; offset += FLASH_PAGE_SIZE)
    {
        uint32_t chunk = AP_BOOTLOADER_RP_FW_SIZE - offset;
        if (chunk > FLASH_PAGE_SIZE) {
            chunk = FLASH_PAGE_SIZE;
        }
        if (!rp2350_flash_read(base_addr + offset, buffer, chunk)) {
            return 0;
        }
        for (uint32_t i = 0; i < chunk; i++) {
            crc = crc32_update(crc, buffer[i]);
        }
    }
    return ~crc;
}

} // namespace AP_Bootloader_RP