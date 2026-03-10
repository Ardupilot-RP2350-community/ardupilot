#include "bl_protocol_common.h"

#include <string.h>

namespace AP_Bootloader_RP {

protocol_context::protocol_context(platform_ops &platform) :
    _platform(platform),
    _write_offset(0)
{
}

proto_result protocol_context::handle_command(const uint8_t *packet, size_t packet_len,
        uint8_t *reply, size_t reply_capacity, size_t &reply_len)
{
    reply_len = 0;

    if (packet_len < 2 || packet[packet_len - 1] != PROTO_EOC || reply_capacity < 2) {
        return proto_result::invalid;
    }

    const uint8_t opcode = packet[0];

    switch (opcode) {
    case PROTO_GET_SYNC:
        return protocol_success(reply, reply_len);

    case PROTO_GET_DEVICE: {
        const board_info info = _platform.get_board_info();
        if (reply_capacity < sizeof(info) + 2) {
            return proto_result::failed;
        }
        memcpy(reply, &info, sizeof(info));
        reply_len = sizeof(info);
        return protocol_success(reply, reply_len);
    }

    case PROTO_CHIP_ERASE:
        if (!_platform.flash_erase_app()) {
            return protocol_failure(reply, reply_len);
        }
        _write_offset = 0;
        return protocol_success(reply, reply_len);

    case PROTO_PROG_MULTI: {
        if (packet_len < 4) {
            return proto_result::invalid;
        }
        const uint8_t length = packet[1];
        if (length == 0 || (size_t)length + 3 != packet_len) {
            return proto_result::invalid;
        }
        if (!_platform.flash_write(_write_offset, &packet[2], length)) {
            return protocol_failure(reply, reply_len);
        }
        _write_offset += length;
        return protocol_success(reply, reply_len);
    }

    case PROTO_GET_CRC: {
        if (reply_capacity < 6) {
            return proto_result::failed;
        }
        const uint32_t crc = _platform.flash_crc();
        memcpy(reply, &crc, sizeof(crc));
        reply_len = sizeof(crc);
        return protocol_success(reply, reply_len);
    }

    case PROTO_GET_VERSION:
        reply[reply_len++] = 0x01;
        return protocol_success(reply, reply_len);

    case PROTO_BOOT:
        reply[reply_len++] = PROTO_INSYNC;
        reply[reply_len++] = PROTO_OK;
        return proto_result::boot_requested;

    default:
        reply[reply_len++] = PROTO_INSYNC;
        reply[reply_len++] = PROTO_INVALID;
        return proto_result::invalid;
    }
}

} // namespace AP_Bootloader_RP