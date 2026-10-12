/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once
/* Matrix config */

#define MATRIX_ROWS 7  // Keypu 0 - 6
#define MATRIX_COLS 18 // Com 16 - 33

#define MATRIX_ROW_PINS {PORT4_0, PORT4_1, PORT4_2, PORT4_3, PORT4_4, PORT4_5, PORT4_6}
#define MATRIX_COL_PINS {PORT0_0, PORT0_1, PORT0_2, PORT0_3, PORT0_4, PORT0_5, PORT0_6, PORT0_7, PORT3_0, PORT3_1, PORT3_2, PORT3_3, PORT3_4, PORT3_5, PORT3_6, PORT3_7, PORT7_0, PORT7_1}

#define DIODE_DIRECTION ROW2COL

// Volume roller: quadrature encoder on P5.5/P5.6, 2 transitions per detent.
#define NUM_ENCODERS 1
#define ENCODERS_PAD_A {PORT5_5}
#define ENCODERS_PAD_B {PORT5_6}
#define ENCODER_RESOLUTION 2
#define ENCODER_MAP_KEY_DELAY 10

#define NO_PRINT
#define NO_DEBUG
#define ACTION_DEBUG

// for ch552, the code size is too small:
// #define NO_ACTION_LAYER
// #define NO_ACTION_ONESHOT
// #define NO_ACTION_TAPPING

// #define HOLD_ON_OTHER_KEY_PRESS

/* Bootmagic: hold this key at power-on to jump to bootloader */
/* Row 0, Col 0 = Esc key on the K580 (confirmed from EVT demo key table) */
#define BOOTMAGIC_LITE_ROW 0
#define BOOTMAGIC_LITE_COLUMN 0

#define USB_MAX_POWER_CONSUMPTION 100

/* RGB Matrix */
//#define RGB_MATRIX_LED_COUNT 104 /* 104 physical keys; DMA buffer is 18×6=108 internally */
//#define RGB_MATRIX_MAXIMUM_BRIGHTNESS 200
//#define RGB_MATRIX_DEFAULT_VAL RGB_MATRIX_MAXIMUM_BRIGHTNESS

/* Indicator LEDs — active high (LED on when pin is driven high) */
#define LED_NUM_LOCK_PIN PORT5_3
#define LED_CAPS_LOCK_PIN PORT5_2
#define LED_SCROLL_LOCK_PIN PORT5_1
#define LED_PIN_ON_STATE 1
/* Win Lock LED (Fn+Win in the default keymap) */
#define LED_WIN_LOCK_PIN PORT5_0
