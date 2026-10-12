/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/*
 * Settings saved in the CH555 data flash (0xF000-0xF3FF) as a log of 32-byte records, 2 per 64-byte
 * page, 32 in all. A save programs the record after the newest one and reads it back; a record that
 * starts a page has its page erased first. The newest record is then in the previous page and stays
 * intact, so a save cut short by a power loss leaves the previous settings. Each page is erased once
 * per 32 saves (10K erase cycles guaranteed per page, datasheet 19.5: 320K saves).
 *
 * Record: 16-bit sequence number, FLASH_STORE_SIZE data bytes, check byte (the 32 bytes sum to 0xFF),
 * format byte. The block program writes the bytes in address order (datasheet 6.5), so a record cut
 * short lacks its format byte. The newest valid record is the one with the highest sequence number.
 */
#define FLASH_STORE_SIZE 28

/* Finds the newest valid record and returns its data (in code space), or 0 if there is none. Call
 * once at startup, before flash_store_save. */
__code const uint8_t *flash_store_find(void);

/* Saves len bytes (up to FLASH_STORE_SIZE; the rest of the record is 0) as a new record. If the
 * read-back differs, tries once more at the start of the next page. Returns false if both failed. */
bool flash_store_save(__xdata const uint8_t *src, uint8_t len);
