/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "flash_store.h"
#include "ch555.h"
#include "data_flash.h"

#define REC_SIZE 32
#define RECS ((BOOT_LOAD_ADDR - DATA_FLASH_ADDR) / REC_SIZE) // 32
#define RECS_PER_PAGE (ROM_PAGE_SIZE / REC_SIZE)             // 2
#define REC(i) ((__code const uint8_t *)(DATA_FLASH_ADDR + (uint16_t)(i) * REC_SIZE))
#define NEXT_PAGE(i) ((uint8_t)(((i) | (RECS_PER_PAGE - 1)) + 1) % RECS) // first record of the next page
#define REC_SEQ(p) ((p)[0] | (uint16_t)(p)[1] << 8)
#define NONE 0xFF
#define TRIES 2

/* Records of another format are ignored: change it when the meaning of the data changes. */
#define FORMAT 0xA1

/* The block program reads xRAM at the same offset in a 64-byte block as the flash address, so a record
 * is built at its page offset in this 64-byte aligned buffer: above the LED buffers (led_dma.h), outside
 * the linker's xRAM (0x0100-0x0FFF). */
static __xdata __at(0x1680) uint8_t rec_buf[ROM_PAGE_SIZE];

static __xdata uint8_t  newest = NONE; // index of the newest valid record
static __xdata uint16_t seq;           // sequence number of the newest record, then of the last one tried

/* Sum of the REC_SIZE bytes at p (code or xdata) */
static uint8_t rec_sum(const uint8_t *p) {
    uint8_t sum = 0, j;

    for (j = 0; j < REC_SIZE; j++) {
        sum += p[j];
    }
    return sum;
}

static bool rec_blank(__code const uint8_t *p) {
    uint8_t j;

    for (j = 0; j < REC_SIZE; j++) {
        if (p[j]) {
            return false;
        }
    }
    return true;
}

__code const uint8_t *flash_store_find(void) {
    __code const uint8_t *p = REC(0);
    uint8_t               i;

    newest = NONE;
    for (i = 0; i < RECS; i++, p += REC_SIZE) {
        if (p[REC_SIZE - 1] == FORMAT && rec_sum(p) == 0xFF && (newest == NONE || (int16_t)(REC_SEQ(p) - seq) > 0)) {
            newest = i;
            seq    = REC_SEQ(p);
        }
    }
    return newest == NONE ? 0 : REC(newest) + 2;
}

bool flash_store_save(__xdata const uint8_t *src, uint8_t len) {
    __code const uint8_t *p;
    __xdata uint8_t      *r;
    uint8_t               i = (uint8_t)(newest + 1) % RECS; // NONE + 1: record 0
    uint8_t               tries, j;

    /* A record that is not blank (a write cut short) moves the save to the next page. */
    if (i % RECS_PER_PAGE && !rec_blank(REC(i))) {
        i = NEXT_PAGE(i);
    }
    for (tries = TRIES; tries; tries--) {
        p = REC(i);
        if (i % RECS_PER_PAGE == 0) {
            data_flash_erase((uint16_t)p);
        }
        r = rec_buf + (i % RECS_PER_PAGE) * REC_SIZE;
        seq++;
        r[0] = (uint8_t)seq;
        r[1] = seq >> 8;
        for (j = 0; j < FLASH_STORE_SIZE; j++) {
            r[2 + j] = j < len ? src[j] : 0;
        }
        r[REC_SIZE - 2] = 0;
        r[REC_SIZE - 1] = FORMAT;
        r[REC_SIZE - 2] = 0xFF - rec_sum(r);
        data_flash_program((uint16_t)p, r, REC_SIZE);

        /* The read-back also catches a failed erase or program. */
        for (j = 0; j < REC_SIZE && p[j] == r[j]; j++) {
        }
        if (j == REC_SIZE) {
            newest = i;
            return true;
        }
        /* The retry gets a higher number than the failed record holds, in case that passes the checks. */
        if ((int16_t)(REC_SEQ(p) - seq) > 0) {
            seq = REC_SEQ(p);
        }
        i = NEXT_PAGE(i);
    }
    return false;
}
