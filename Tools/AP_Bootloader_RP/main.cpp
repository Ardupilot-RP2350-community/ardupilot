#include "bl_protocol_common.h"
#include "platform/rp2350_platform.h"

namespace AP_Bootloader_RP {

static bool read_packet(rp2350_platform &platform, uint8_t *buffer, size_t capacity, size_t &packet_len)
{
    packet_len = 0;

    while (packet_len < capacity) {
        uint8_t byte = 0;
        if (!platform.transport_read(byte, 5000)) {
            return false;
        }
        buffer[packet_len++] = byte;
        if (byte == PROTO_EOC) {
            return true;
        }
    }

    return false;
}

} // namespace AP_Bootloader_RP

int main()
{
    using namespace AP_Bootloader_RP;

    rp2350_platform platform;
    if (!platform.init()) {
        return 1;
    }

    protocol_context protocol(platform);
    static uint8_t packet[260] {};
    static uint8_t reply[300] {};

    while (true) {
        size_t packet_len = 0;
        if (!read_packet(platform, packet, sizeof(packet), packet_len)) {
            continue;
        }

        size_t reply_len = 0;
        const proto_result result = protocol.handle_command(packet, packet_len,
                                  reply, sizeof(reply), reply_len);

        if (reply_len > 0) {
            platform.transport_write(reply, reply_len);
        }
        if (result == proto_result::boot_requested) {
            platform.transport_flush();
            platform.jump_to_application();
        }
    }

    return 0;
}
