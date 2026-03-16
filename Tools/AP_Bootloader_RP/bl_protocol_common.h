#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AP_Bootloader_RP {

struct board_info {
    uint32_t board_type;
    uint32_t board_rev;
    uint32_t fw_size;
};

enum class proto_result : uint8_t {
    ok = 0,
    invalid,
    failed,
    boot_requested,
};

enum : uint8_t {
    PROTO_GET_SYNC = 0x21,
    PROTO_GET_DEVICE = 0x22,
    PROTO_CHIP_ERASE = 0x23,
    PROTO_PROG_MULTI = 0x27,
    PROTO_GET_CRC = 0x29,
    PROTO_GET_VERSION = 0x2f,
    PROTO_BOOT = 0x30,
    PROTO_EOC = 0x20,
    PROTO_INSYNC = 0x12,
    PROTO_OK = 0x10,
    PROTO_FAILED = 0x11,
    PROTO_INVALID = 0x13,
};

class platform_ops {
public:
    virtual ~platform_ops() = default;

    virtual bool flash_erase_app() = 0;
    virtual bool flash_write(uint32_t address, const uint8_t *data, size_t length) = 0;
    virtual uint32_t flash_crc() = 0;
    virtual board_info get_board_info() const = 0;
};

class protocol_context {
public:
    explicit protocol_context(platform_ops &platform);

    proto_result handle_command(const uint8_t *packet, size_t packet_len,
                                uint8_t *reply, size_t reply_capacity, size_t &reply_len);

private:
    static inline proto_result protocol_success(uint8_t *reply, size_t &reply_len) {
        reply[reply_len++] = PROTO_INSYNC;
        reply[reply_len++] = PROTO_OK;
        return proto_result::ok;
    }
    static inline proto_result protocol_failure(uint8_t *reply, size_t &reply_len) {
        reply[reply_len++] = PROTO_INSYNC;
        reply[reply_len++] = PROTO_FAILED;
        return proto_result::failed;        
    }

    platform_ops &_platform;
    uint32_t _write_offset;
};

} // namespace AP_Bootloader_RP