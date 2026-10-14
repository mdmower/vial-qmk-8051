// Copyright 2022 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include "hardware_id.h"
#include <string.h>
#include "ch555.h"

/* CH555 datasheet 6.7: the 48-bit chip ID at offsets 0x10-0x15 of the read-only information area, lowest byte
 * first, and the 16-bit sum of its three words at 0x16-0x17. The area is read 4 bytes at a time (datasheet 6.5,
 * step 6). The ID goes to the first 6 bytes of id; all zeros if the sum does not match. */
hardware_id_t get_hardware_id(void) {
    hardware_id_t id;
    UINT16        w[4];

    ROM_ADDR = ROM_CHIP_ID_LO;
    ROM_CTRL = ROM_CMD_RD_OTP;
    w[0]     = ROM_DATA_LO;
    w[1]     = ROM_DATA_HI;
    ROM_ADDR = ROM_CHIP_ID_HI;
    ROM_CTRL = ROM_CMD_RD_OTP;
    w[2]     = ROM_DATA_LO;
    w[3]     = ROM_DATA_HI;
    memset(&id, 0, sizeof(hardware_id_t));
    if ((UINT16)(w[0] + w[1] + w[2]) == w[3]) {
        memcpy(&id, w, 6);
    }
    return id;
}
