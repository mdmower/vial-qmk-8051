#include "rgb_matrix_ch555.h"
#include "ch555.h"

/*
 * CH555 RGB LED controller + key scan driver
 * ------------------------------------------
 * The LED ISR (INT_NO_PWM_I2C) fires once per COM column advance.
 * On each firing it:
 *   1. Increments LED_COMMON — this both selects the next COM and clears
 *      the interrupt flag (same as the EVT reference firmware).
 *   2. Resets LED_DMA at frame start (COM wraps to 0x10).
 *   3. Calls LED_LOAD_XRAM (ASM) — SETB P copies LED_DMA into DPTR and
 *      arms the feed; 32x MOVX clocks 8 rows × 4 bytes into the PWM engine.
 *   4. Briefly switches P4 to quasi-bidi input, reads row bits → shadow_matrix.
 *   5. Restores P4 to push-pull output for next PWM cycle.
 *
 * Pin mapping:
 *   RED   : P4.0–P4.5  (PWM rows 0–5)
 *   GREEN : P2.0–P2.5  (PWM rows 0–5)
 *   BLUE  : P1.0–P1.5  (PWM rows 0–5)
 *   Rows 6–7 reserved (macro/underglow — future)
 *   COMs  : P0.0–P0.7 (col 0–7), P3.0–P3.7 (col 8–15),
 *           P7.0 (col 16), P7.1 (col 17)
 *
 * LED_COMMON values:
 *   0x10–0x17 = col 0–7 (P0), 0x18–0x1F = col 8–15 (P3)
 *   0x0E = col 16 (P7.0), 0x0F = col 17 (P7.1)
 *   0x00 = wrap sentinel — ISR restarts at 0x10
 *
 * DMA buffer layout:
 *   slot = col * RGB_CH555_ROWS + row  (col 0–17, row 0–7)
 *   buf[slot][0] = intensity, [1] = R, [2] = G, [3] = B
 */

/* -------------------------------------------------------------------------
 * DMA buffer and shadow matrix — in xRAM
 * ---------------------------------------------------------------------- */

__xdata uint8_t rgb_dma_buf[RGB_CH555_LED_COUNT][4];
volatile uint8_t led_shadow_matrix[RGB_CH555_COLS];

/* -------------------------------------------------------------------------
 * ASM helper — rgbload.asm
 * Clocks 8 rows × 4 bytes from xRAM into the CH555 LED PWM engine.
 * SETB P inside the ASM copies LED_DMA into DPTR and arms the feed;
 * LED_DMA auto-increments after each byte so no manual tracking needed.
 * ---------------------------------------------------------------------- */
extern void LED_LOAD_XRAM(void);

/* -------------------------------------------------------------------------
 * LED + key scan ISR
 * ---------------------------------------------------------------------- */

void LED_INT(void) __interrupt(INT_NO_PWM_I2C) __using(1) {
    uint8_t rows;
    uint8_t com;

    /* Advance COM — also clears the interrupt flag */
    com = (uint8_t)(LED_COMMON + 1u) & 0x1Fu;
    if (com == 0) {
        /* Wrapped past 0x1F — restart at 0x10 (first COM column) */
        LED_COMMON = 0x10;
        /* Reset DMA pointer to start of buffer for new frame */
        LED_DMA = (uint16_t)(&rgb_dma_buf[0][0]);
    } else {
        LED_COMMON = com;
    }

    /* Current column index (0–17) from LED_COMMON */
    com = LED_COMMON;
    if (com >= 0x10 && com <= 0x17) {
        com = com - 0x10u;           /* col 0–7  */
    } else if (com >= 0x18) {
        com = com - 0x18u + 8u;      /* col 8–15 */
    } else if (com == 0x0E) {
        com = 16u;
    } else {
        com = 17u;                   /* 0x0F */
    }

    /* Load 8 rows × 4 bytes for this column into LED PWM engine */
    LED_LOAD_XRAM();

    /* --- Key scan on current COM column ---
     * Switch P4 to quasi-bidi (input with weak pull-up), wait for settle,
     * read row bits 0–5, then restore push-pull output for PWM. */
    P4_MOD_OC = 0xFF;
    _nop_(); _nop_(); _nop_(); _nop_(); _nop_();
    _nop_(); _nop_(); _nop_(); _nop_(); _nop_();
    _nop_(); _nop_(); _nop_(); _nop_(); _nop_();
    _nop_(); _nop_(); _nop_(); _nop_(); _nop_();

    rows = ~P4 & 0x3Fu;   /* rows 0–5 only */
    led_shadow_matrix[com] = rows;

    P4_MOD_OC = 0x00;
    LED_PWM_OE = 0xFF;   /* re-enable all 8 PWM row channels */

    /* Restore all COM pins high */
    P0 = 0xFF;
    P3 = 0xFF;
    P7 |= 0x03u;
}

/* -------------------------------------------------------------------------
 * Initialisation
 * ---------------------------------------------------------------------- */

void rgb_matrix_ch555_init(void) {
    uint8_t i;

    /* Clear DMA buffer — all LEDs off */
    for (i = 0; i < RGB_CH555_LED_COUNT; i++) {
        rgb_dma_buf[i][0] = 0;
        rgb_dma_buf[i][1] = 0;
        rgb_dma_buf[i][2] = 0;
        rgb_dma_buf[i][3] = 0;
    }

    /* GPIO init: P4/P2/P1 (PWM rows): push-pull, all high (off) */
    P4 = P2 = P1 = 0xFF;
    P4_MOD_OC = P2_MOD_OC = P1_MOD_OC = 0x00;
    P4_DIR_PU = P2_DIR_PU = P1_DIR_PU = 0xFF;

    /* P0, P3 (COM lines): push-pull, all high */
    P0 = P3 = 0xFF;
    P0_MOD_OC = P3_MOD_OC = 0x00;
    P0_DIR_PU = P3_DIR_PU = 0xFF;

    /* P7.0, P7.1 (COM col 16/17): push-pull, high */
    P7 |= 0x03u;
    P7 |= (0x03u << 2);

    /* LED controller config */
    LED_CTRL    = bLED_EN;
    LED_CYCLE   = bLED_INTEN_CYC0;
    LED_FRAME   = bLED_INH_TMR2 | bLED_INH_TMR0;
    LED_DMA     = (uint16_t)(&rgb_dma_buf[0][0]);
    LED_INT_ADJ = 0x05;
    LED_PWM_OE  = 0xFF;                          /* all 8 PWM rows */
    LED_COMMON  = 0xFF;                          /* EVT: preset 0xFF, first INC → 0x00 → restarts at 0x10 */

    LED_CTRL = bLED_IE_INHIB | bLED_BLUE_EN | bLED_GREEN_EN | bLED_RED_EN
             | bLED_COM_AHEAD | bLED_PWM_INHIB | bLED_EN;

    LED_STATUS |= bLED_IF_SET;   /* force first interrupt */
    IE_PWM_I2C = 1;
}
