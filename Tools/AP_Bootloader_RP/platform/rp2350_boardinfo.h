#pragma once

#include "../bl_protocol_common.h"

#define AP_BOOTLOADER_RP_BOARD_TYPE (APJ_BOARD_ID)
#define AP_BOOTLOADER_RP_FW_SIZE (BOARD_FLASH_SIZE - (FLASH_BOOTLOADER_LOAD_KB + FLASH_RESERVE_END_KB + APP_START_OFFSET_KB))*1024

namespace AP_Bootloader_RP {

board_info rp2350_get_board_info();

} // namespace AP_Bootloader_RP