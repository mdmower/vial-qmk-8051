/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "led_fx.h"
#include "led_dma.h"

/*
 * Colours are HSV. Hue runs 0-191 around the colour wheel (6 sectors of 32); the hue setting has 24
 * steps of 8, so red, yellow, green, cyan, blue and magenta are every 4th step. Saturation 0 is
 * white at any hue.
 */
#define HUE_RANGE 192
#define HUE_STEPS 24

enum { MODE_STATIC, MODE_RAINBOW, MODE_RAINBOW_REV, MODE_WAVE, MODE_WAVE_REV, MODE_CYCLE, MODE_BREATHE, MODES };
#define F_POS 0x01     // hue changes with the LED position (led_pos)
#define F_REV 0x02     // ... in the other direction
#define F_MOVE 0x04    // hue changes with time
#define F_BREATHE 0x08 // brightness changes with time

static __code const uint8_t mode_flags[MODES]   = {0, F_POS, F_POS | F_REV, F_POS | F_MOVE, F_POS | F_REV | F_MOVE, F_MOVE, F_BREATHE};
static __code const uint8_t setting_steps[]     = {MODES, HUE_STEPS, 5, 8, 5, 2}; // mode, hue, sat, val, speed, on
static __code const uint8_t sat_table[5]        = {0, 96, 160, 208, 255};
static __code const uint8_t val_table[8]        = {8, 16, 32, 64, 96, 144, 200, 255};
/* Phase advance per pass (one pass per LED frame, ~12.7 ms), in 1/256: one cycle takes 26, 13, 6.5,
 * 3.2 or 1.6 s */
static __code const uint16_t speed_table[5]     = {32, 64, 128, 256, 512};

/*
 * Position of each LED along its zone, 0-191, by buffer slot (COM * 8 + group). F_POS modes add it
 * to the hue (F_REV: subtract it), and F_MOVE modes shift the hue so that the colours move towards
 * higher positions.
 * Keys: key centre x on a 22.5-unit-wide board (standard 104-key positions), so position grows from
 * left to right.
 * Side lights: pair index * 24; stock table @0x30c9 pairs COM c with COM 15 - c, one on each side.
 * Pair 0 (COMs 0 and 15) is at the back of the keyboard (the edge away from the user), pair 7 (COMs 7
 * and 8) at the front, so position grows from back to front.
 * MONO: one LED on the red line, white for G1-G5, red for Rec and the media keys.
 */
#define MONO 0xFF
static __code const uint8_t led_pos[LED_DMA_COMS * LED_DMA_GROUPS] = {
    // clang-format off
      4,   4,   6,   7,  10,   5,   0,   0, // COM 0
     21,  13,  17,  19,   0,  16,   0,  24, // COM 1
     30,  21,  26,  28,  23,  27,   0,  48, // COM 2
     38,  30,  34,  36,  32,  59,   0,  72, // COM 3
     47,  38,  43,  45,  41,  91,   0,  96, // COM 4
     60,  47,  51,  53,  49, 101,   0, 120, // COM 5
     68,  55,  60,  62,  58, 112, MONO, 144, // COM 6
     77,  64,  68,  70,  66, 123, MONO, 168, // COM 7
     85,  73,  77,  79,  75, 134, MONO, 168, // COM 8
     98,  81,  85,  87,  83, 143, MONO, 144, // COM 9
    107,  90,  94,  96,  92, 151, MONO, 120, // COM 10
    115,  98, 102, 105, 100, 166, MONO,  96, // COM 11
    124, 107, 111,   0,   0, 179, MONO,  72, // COM 12
    134, 119, 122, 118, 116, 162, MONO,  48, // COM 13
    143, 134, 134, 162, 143, 171, MONO,  24, // COM 14
    151, 143, 143, 171, 162, 179, MONO,   0, // COM 15
    162, 151, 151, 179, 171, 188,   0,   0, // COM 16
    171, 179, 188, 188, 179,   0,   0,   0, // COM 17
    // clang-format on
};

/* Settings, indexed by LED_FX_MODE...LED_FX_ON; zone 0 = keys, zone 1 = side lights */
static __xdata uint8_t settings[2][LED_FX_SETTINGS] = {
    {MODE_WAVE, 0, 4, 4, 2, 1},
    {MODE_RAINBOW, 0, 4, 4, 2, 1},
};
static __xdata uint16_t phase[2];

/* Per-zone values for the current pass; in iRAM, as are the other values used for every LED */
typedef struct {
    uint8_t flags;
    uint8_t hue;
    uint8_t sat;
    uint8_t val;
} zone_t;
static __xdata zone_t zone[2];

static __xdata uint8_t fx_com = LED_DMA_COMS; // next COM to render; LED_DMA_COMS: no pass running
static __xdata uint8_t fx_frames;             // led_dma_frames at the start of the last pass
static __bit           fx_valid;              // the last pass used the current settings
static __bit           fx_animated;           // a zone changes with time

/* a * b / 255, rounded down; b = 255 gives a */
#define SCALE8(a, b) ((uint8_t)(((uint16_t)(uint8_t)(a) * (uint8_t)(b) + (uint8_t)(a)) >> 8))

/* hsv_to_rgb input and output */
static __data uint8_t hsv_h, hsv_s, hsv_v;
static __data uint8_t rgb[3];

/* Per hue sector: which of v, p, q, t (hsv_c[0..3]) are red, green and blue */
static __code const uint8_t hsv_order[6][3] = {{0, 3, 1}, {2, 0, 1}, {1, 0, 3}, {1, 2, 0}, {3, 1, 0}, {0, 1, 2}};
static __data uint8_t       hsv_c[4];

static void hsv_to_rgb(void) {
    uint8_t               rem = (hsv_h & 31) << 3;
    __code const uint8_t *o   = hsv_order[hsv_h >> 5];

    hsv_c[0] = hsv_v;
    hsv_c[1] = SCALE8(hsv_v, 255 - hsv_s);
    hsv_c[2] = SCALE8(hsv_v, 255 - SCALE8(hsv_s, rem));
    hsv_c[3] = SCALE8(hsv_v, 255 - SCALE8(hsv_s, 255 - rem));
    rgb[0]   = hsv_c[o[0]];
    rgb[1]   = hsv_c[o[1]];
    rgb[2]   = hsv_c[o[2]];
}

/* hsv_h = (hsv_h + b) mod HUE_RANGE, for b <= HUE_RANGE */
static void hue_add(uint8_t b) {
    uint8_t a = hsv_h;

    hsv_h += b;
    if (hsv_h < a || hsv_h >= HUE_RANGE) {
        hsv_h -= HUE_RANGE;
    }
}

/* Sets zone[] from the settings; the phase advances once per pass. */
static void start_pass(void) {
    __xdata uint8_t  *set = settings[0];
    __xdata uint16_t *ph  = phase;
    __xdata zone_t   *zs  = zone;
    uint8_t           z;

    fx_animated = 0;
    for (z = 0; z < 2; z++, set += LED_FX_SETTINGS, ph++, zs++) {
        uint8_t flags = mode_flags[set[LED_FX_MODE]];
        uint8_t p, v = 0;

        *ph += speed_table[set[LED_FX_SPEED]];
        p     = *ph >> 8;
        hsv_h = set[LED_FX_HUE] << 3;
        if (set[LED_FX_ON]) {
            v = val_table[set[LED_FX_VAL]];
            if (flags & (F_MOVE | F_BREATHE)) {
                fx_animated = 1;
            }
        }
        if (flags & F_MOVE) {
            hue_add(HUE_RANGE - ((p >> 1) + (p >> 2))); // hue - p * 3/4
        }
        if (flags & F_BREATHE) {
            uint8_t t = p << 1; // triangle wave 0-254-1
            if (p & 0x80) {
                t = ~t;
            }
            v = SCALE8(v, SCALE8(t, t));
        }
        zs->flags = flags;
        zs->hue   = hsv_h;
        zs->sat   = sat_table[set[LED_FX_SAT]];
        zs->val   = v;
    }
}

/* Writes red, green, blue of the 8 LEDs of COM c (group 7 = side lights, zone 1) */
static void render_com(uint8_t c) {
    __xdata uint8_t      *p   = led_dma_buf + ((uint16_t)c << 5);
    __code const uint8_t *pos = led_pos + (c << 3);
    __xdata zone_t       *zs  = zone;
    uint8_t               n;

    for (n = 0; n < LED_DMA_GROUPS; n++, p += 4) {
        uint8_t x = *pos++;

        if (n == LED_DMA_GROUPS - 1) {
            zs++;
        }
        if (x == MONO) {
            /* single-colour LED: lit at the zone's brightness whatever the hue */
            rgb[0] = zs->val;
            rgb[1] = 0;
            rgb[2] = 0;
        } else {
            hsv_h = zs->hue;
            if (zs->flags & F_POS) {
                hue_add((zs->flags & F_REV) ? HUE_RANGE - x : x);
            }
            hsv_s = zs->sat;
            hsv_v = zs->val;
            hsv_to_rgb();
        }
        p[1] = rgb[0];
        p[2] = rgb[1];
        p[3] = rgb[2];
    }
}

void led_fx_load(__code const uint8_t *p) {
    __xdata uint8_t *v = &settings[0][0];
    uint8_t          i, s = 0;

    for (i = 0; i < sizeof(settings); i++) {
        if (p[i] < setting_steps[s]) {
            v[i] = p[i];
        }
        if (++s == LED_FX_SETTINGS) {
            s = 0;
        }
    }
    fx_valid = 0;
}

void led_fx_save(__xdata uint8_t *dst) {
    __xdata uint8_t *v = &settings[0][0];
    uint8_t          i;

    for (i = 0; i < sizeof(settings); i++) {
        dst[i] = v[i];
    }
}

void led_fx_task(void) {
    if (fx_com >= LED_DMA_COMS) {
        if (fx_valid && (!fx_animated || led_dma_frames == fx_frames)) {
            return;
        }
        fx_frames = led_dma_frames;
        fx_valid  = 1;
        fx_com    = 0;
        start_pass();
    }
    render_com(fx_com++);
}

void led_fx_adjust(uint8_t action, bool reverse) {
    __xdata uint8_t *v = &settings[0][0] + action;
    uint8_t          s = action < LED_FX_SETTINGS ? action : action - LED_FX_SETTINGS;

    if (s == LED_FX_ON) {
        *v = !*v;
    } else {
        uint8_t last = setting_steps[s] - 1;
        if (reverse) {
            if (*v) {
                (*v)--;
            } else if (s <= LED_FX_HUE) {
                *v = last; // mode and hue wrap around
            }
        } else if (*v < last) {
            (*v)++;
        } else if (s <= LED_FX_HUE) {
            *v = 0;
        }
    }
    fx_valid = 0;
}
