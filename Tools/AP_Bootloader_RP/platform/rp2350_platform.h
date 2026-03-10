#pragma once

#include "../bl_protocol_common.h"
#include "rp2350_transport_usb_cdc.h"

namespace AP_Bootloader_RP {

class rp2350_platform : public platform_ops {
public:
    bool init();

    bool flash_erase_app() override;
    bool flash_write(uint32_t address, const uint8_t *data, size_t length) override;
    uint32_t flash_crc() override;
    board_info get_board_info() const override;

    bool transport_read(uint8_t &byte, uint32_t timeout_ms);
    bool transport_write(const uint8_t *data, size_t length);
    void transport_flush();

    void jump_to_application();

private:
    rp2350_transport_usb_cdc _transport;
};

} // namespace AP_Bootloader_RP