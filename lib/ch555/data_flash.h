/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include <stdint.h>

/*
 * CH555 data flash: DATA_FLASH_ADDR (0xF000) to BOOT_LOAD_ADDR - 1 (0xF3FF), 16 pages of 64 bytes.
 * Erased bytes read 0x00, programming changes bits from 0 to 1. Read it as code space (MOVC).
 *
 * Writes are enabled with bDATA_WE only, which covers the data flash; code flash stays
 * write-protected (bCODE_WE, CH555 datasheet, GLOBAL_CFG). Addresses outside the data flash are
 * refused. The CPU stops during each erase or program (2-8 ms, datasheet 19.5), and interrupts are
 * off from write enable to write disable.
 *
 * Both functions return 0 on success.
 */

/* Erases the 64-byte page that holds addr. */
uint8_t data_flash_erase(uint16_t addr);

/* Programs len bytes (1-64, within one page) at addr from xRAM at buf. The low 6 bits of buf must
 * equal those of addr (block program, datasheet 6.5). */
uint8_t data_flash_program(uint16_t addr, __xdata const uint8_t *buf, uint8_t len);
