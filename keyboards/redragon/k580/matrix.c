/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "matrix.h"
#include "led_dma.h"

/* The LED-DMA interrupt drives the COM (column) pins, so it also reads the key rows. */
void matrix_init_custom(void) {
    led_dma_init();
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool    changed = false;
    uint8_t row, col, mask;

    if (!led_dma_frame) {
        return false;
    }
    led_dma_frame = 0;

    for (row = 0, mask = 1; row < MATRIX_ROWS; row++, mask <<= 1) {
        matrix_row_t bits = 0;
        matrix_row_t bit  = 1;
        for (col = 0; col < MATRIX_COLS; col++, bit <<= 1) {
            if (led_dma_keys[col] & mask) {
                bits |= bit;
            }
        }
        if (current_matrix[row] != bits) {
            current_matrix[row] = bits;
            changed             = true;
        }
    }
    return changed;
}
