/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "ch555.h"

/*
 * CH555 LED controller (LED-DMA), set up as the stock K580 firmware 1.04 does it.
 *
 * 18 commons (COM), one per key matrix column: P0.0-P0.7, P3.0-P3.7, P7.0, P7.1.
 * 8 PWM groups per COM: red P4.0-P4.7, green P2.0-P2.7, blue P1.0-P1.7. Group n of COM c is
 * the LED of key (n, c) for n = 0-6; group 7 of COMs 0-15 are the side lights.
 * Buffer: 4 bytes per LED at (c * 8 + n) * 4: intensity, red, green, blue.
 *
 * P4 is also the key matrix row input, so the LED interrupt reads the key rows of each COM
 * while the controller is inhibited between COMs.
 */
#define LED_DMA_COMS 18
#define LED_DMA_GROUPS 8

/* At 0x1400, above the USB buffers (0x1000-0x13FF) and outside the linker's xRAM (0x0100-0x0FFF);
 * 256-byte aligned, as the stock buffer at 0x0200 is. */
#define LED_DMA_BUF_ADDR 0x1400
extern __xdata __at(LED_DMA_BUF_ADDR) uint8_t led_dma_buf[LED_DMA_COMS * LED_DMA_GROUPS * 4];

/* Key rows read on each COM: bit n set = key (n, COM) pressed. */
extern volatile __xdata uint8_t led_dma_keys[LED_DMA_COMS];
/* Set after the last COM of a frame has been read. */
extern volatile __bit led_dma_frame;
/* Frame counter, +1 after the last COM of each frame (~12.7 ms). */
extern volatile __data uint8_t led_dma_frames;

void led_dma_init(void);
void led_dma_output(bool on);

/* Declared here so that main.c, which includes this through sdcc_dummy_macros.h, puts it in the
 * interrupt vector table. */
void led_dma_isr(void) __interrupt(INT_NO_PWM_I2C);
