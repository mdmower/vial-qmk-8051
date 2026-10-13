/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "led_dma.h"
#include "mcs51_isr.h"

/* LED_COMMON value of each COM, in key matrix column order (stock 1.04 table @0x3027) */
static __code const uint8_t com_select[LED_DMA_COMS] = {
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, // P0.0-P0.7
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, // P3.0-P3.7
    0x0E, 0x0F                                      // P7.0, P7.1
};

__xdata __at(LED_DMA_BUF_ADDR) uint8_t led_dma_buf[LED_DMA_COMS * LED_DMA_GROUPS * 4];
volatile __xdata uint8_t led_dma_keys[LED_DMA_COMS];
volatile __bit           led_dma_frame;
volatile __data uint8_t  led_dma_frames;
static __data uint8_t    led_com;
/* LED data for the COMs while the LEDs are off: all zero */
static __xdata __at(LED_DMA_BUF_ADDR + sizeof(led_dma_buf)) uint8_t led_dma_off[LED_DMA_GROUPS * 4];
static __bit led_dark;

/*
 * Loads the 32 bytes of the current COM into the LED controller (stock 1.04 @0xd686). With P
 * (PSW.0) set, MOVX A,@DPTR reads from LED_DMA instead of DPTR, and LED_DMA advances; E_DIS
 * blocks interrupts during the load.
 */
static void led_dma_load(void) __naked {
    // clang-format off
    __asm
        setb    0xae    ; E_DIS
        setb    0xd0    ; P
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        nop
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        nop
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        nop
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        movx    a,@dptr
        clr     0xae    ; E_DIS
        ret
    __endasm;
    // clang-format on
}

/*
 * Runs at the start of each inhibition period between COMs (stock 1.04 @0x7edf): selects the
 * next COM, loads its LED data, then reads the key rows on P4 with that COM driven low.
 */
MCS51_ISR(led_dma_isr, INT_NO_PWM_I2C) {
    __data uint8_t com = led_com;
    __data uint8_t rows;

    if (com == LED_DMA_COMS) {
        com = 0;
    }
    LED_COMMON = com_select[com]; // also clears bLED_IF
    /* Stock sets LED_DMA once per frame and lets it advance; here it is set for each COM, so that
     * the LEDs can switch to led_dma_off at any COM. */
    {
        uint16_t addr = led_dark ? (uint16_t)led_dma_off : (uint16_t)led_dma_buf + ((uint16_t)com << 5);
        LED_DMA_H     = addr >> 8;
        LED_DMA_L     = addr & 0xFF;
    }
    led_dma_load();

    if (com < 8) {
        P0 &= ~(uint8_t)(1 << com);
    } else if (com < 16) {
        P3 &= ~(uint8_t)(1 << (com - 8));
    } else if (com == 16) {
        P7 |= bP7_0_DIR;
        P7 &= ~bP7_0_OUT_PU;
    } else {
        P7 |= bP7_1_DIR;
        P7 &= ~bP7_1_OUT_PU;
    }

    /* P4 to key input with pull-up, wait as stock does, read the rows */
    P4_LED_KEY = 0xFF;
    P4_MOD_OC  = 0xFF;
    __asm__("nop");
    __asm__("nop");
    (void)LED_CTRL;
    // clang-format off
    __asm
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
    __endasm;
    // clang-format on
    rows = P4;

    /* P4 back to push-pull, PWM outputs on (bLED_PWM_INHIB turned them off at the end of the
     * previous COM; this write also starts the next COM: with 0 the controller stops), all COM
     * pins high push-pull */
    P4_MOD_OC  = 0;
    P4_LED_KEY = 0;
    LED_PWM_OE = 0xFF;
    P0_MOD_OC  = 0;
    P3_MOD_OC  = 0;
    P0_DIR_PU  = 0xFF;
    P3_DIR_PU  = 0xFF;
    P7 |= bP7_0_DIR | bP7_1_DIR;
    P0 = 0xFF;
    P3 = 0xFF;
    P7 |= bP7_0_OUT_PU | bP7_1_OUT_PU;

    led_dma_keys[com] = ~rows;
    if (com == LED_DMA_COMS - 1) {
        led_dma_frame = 1;
        led_dma_frames++;
    }
    led_com = com + 1;
}

/* Stock 1.04 @0x9ecf */
void led_dma_init(void) {
    uint16_t i;

    /* Intensity 0xFF (as stock), colours 0; led_fx.c writes the colours */
    for (i = 0; i < sizeof(led_dma_buf); i++) {
        led_dma_buf[i] = (i & 3) ? 0 : 0xFF;
    }
    for (i = 0; i < sizeof(led_dma_off); i++) {
        led_dma_off[i] = 0;
    }
    led_com       = 0;
    led_dma_frame = 0;
    led_dark      = 0;

    P4        = 0xFF;
    P2        = 0xFF;
    P1        = 0xFF;
    P3        = 0xFF;
    P0        = 0xFF;
    P1_MOD_OC = 0;
    P2_MOD_OC = 0;
    P4_MOD_OC = 0;
    P3_MOD_OC = 0;
    P0_MOD_OC = 0;
    P7 |= bP7_0_DIR | bP7_1_DIR | bP7_0_OUT_PU | bP7_1_OUT_PU;

    LED_CTRL    = bLED_EN;
    LED_CYCLE   = bLED_INTEN_CYC1 | bLED_INTEN_CYC0; // 64-clock intensity PWM, 256-step colour PWM, Fsys
    LED_FRAME   = bLED_INH_TMR2 | bLED_INH_TMR0;     // inhibition 8 intensity PWM cycles, no repeat
    LED_DMA_H   = (uint16_t)led_dma_buf >> 8;
    LED_DMA_L   = (uint16_t)led_dma_buf & 0xFF;
    LED_INT_ADJ = 0;
    LED_PWM_OE  = 0xFF;
    LED_COMMON  = com_select[0];
    LED_CTRL    = bLED_IE_INHIB | bLED_BLUE_EN | bLED_GREEN_EN | bLED_RED_EN | bLED_COM_AHEAD | bLED_PWM_INHIB | bLED_EN;
    LED_STATUS |= bLED_IF_SET; // first interrupt now
    IE_PWM_I2C  = 1;
    LED_RED_ADJ = 0;
    LED_GRE_ADJ = 0;
    LED_BLU_ADJ = 0;
}

/* LEDs on or off; the controller and the key scan keep running in both cases. */
void led_dma_output(bool on) {
    led_dark = !on;
}
