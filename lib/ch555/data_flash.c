/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "ch555.h"
#include "data_flash.h"

/* DPTR as one 16-bit SFR: the block program reads its data from xRAM at DPTR. */
__sfr16 __at(0x8382) DF_DPTR;

/* GLOBAL_CFG is writable for 13-23 cycles after 0x55, 0xAA to SAFE_MOD (datasheet, SAFE_MOD). */
#define DATA_WE_ON()            \
    do {                        \
        SAFE_MOD = 0x55;        \
        SAFE_MOD = 0xAA;        \
        GLOBAL_CFG |= bDATA_WE; \
        SAFE_MOD = 0;           \
    } while (0)
#define DATA_WE_OFF()            \
    do {                         \
        SAFE_MOD = 0x55;         \
        SAFE_MOD = 0xAA;         \
        GLOBAL_CFG &= ~bDATA_WE; \
        SAFE_MOD = 0;            \
    } while (0)

#define IN_DATA_FLASH(a) ((a) >= DATA_FLASH_ADDR && (a) < BOOT_LOAD_ADDR)

uint8_t data_flash_erase(uint16_t addr) {
    uint8_t status = 0xFF;
    __bit   ea     = EA;

    if (IN_DATA_FLASH(addr)) {
        EA = 0;
        DATA_WE_ON();
        ROM_ADDR    = addr;
        ROM_BUF_MOD = bROM_BUF_BYTE; // 64-byte page erase
        ROM_DAT_BUF = 0;
        if (ROM_STATUS & bROM_ADDR_OK) {
            ROM_CTRL = ROM_CMD_ERASE;
            status   = ROM_STATUS & bROM_CMD_ERR;
        }
        DATA_WE_OFF();
        EA = ea;
    }
    return status;
}

uint8_t data_flash_program(uint16_t addr, __xdata const uint8_t *buf, uint8_t len) {
    uint8_t status = 0xFF;
    uint8_t offset = (uint8_t)addr & MASK_ROM_ADDR;
    __bit   ea     = EA;

    if (IN_DATA_FLASH(addr) && len && len <= ROM_PAGE_SIZE - offset && ((uint8_t)(uint16_t)buf & MASK_ROM_ADDR) == offset) {
        EA = 0;
        DATA_WE_ON();
        ROM_ADDR    = addr;
        ROM_BUF_MOD = offset + len - 1; // block program: low 6 bits of the last address
        if (ROM_STATUS & bROM_ADDR_OK) {
            /* Nothing between these two may use DPTR (checked in the compiler output). */
            DF_DPTR  = (uint16_t)buf;
            ROM_CTRL = ROM_CMD_PROG;
            status   = ROM_STATUS & bROM_CMD_ERR;
        }
        DATA_WE_OFF();
        EA = ea;
    }
    return status;
}
