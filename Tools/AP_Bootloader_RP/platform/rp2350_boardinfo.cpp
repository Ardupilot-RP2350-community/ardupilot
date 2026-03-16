#include "rp2350_boardinfo.h"

#ifndef AP_BOOTLOADER_RP_BOARD_TYPE
#define AP_BOOTLOADER_RP_BOARD_TYPE 0
#endif

#ifndef AP_BOOTLOADER_RP_BOARD_REV
#define AP_BOOTLOADER_RP_BOARD_REV 0
#endif

#ifndef AP_BOOTLOADER_RP_FW_SIZE
#define AP_BOOTLOADER_RP_FW_SIZE 0
#endif

namespace AP_Bootloader_RP {

board_info rp2350_get_board_info()
{
    return {
        AP_BOOTLOADER_RP_BOARD_TYPE,
        AP_BOOTLOADER_RP_BOARD_REV,
        AP_BOOTLOADER_RP_FW_SIZE,
    };
}

} // namespace AP_Bootloader_RP
