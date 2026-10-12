/* SPDX-License-Identifier: GPL-2.0-or-later */
/* No EEPROM: QMK's eeconfig reads zeros and its writes are dropped. The K580 keeps nothing of QMK's
 * eeconfig across power-off anyway (the transient driver held it in RAM). */
#include "sdcc_dummy_macros.h"
#include <stdint.h>
#include <string.h>
#include "eeprom_driver.h"

void eeprom_driver_init(void) {}

void eeprom_driver_erase(void) {}

void eeprom_read_block(void *buf, const void *addr, size_t len) {
    (void)addr;
    memset(buf, 0, len);
}

void eeprom_write_block(const void *buf, void *addr, size_t len) {
    (void)buf;
    (void)addr;
    (void)len;
}
