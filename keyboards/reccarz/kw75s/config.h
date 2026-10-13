/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

/* Matrix config */

#define MATRIX_ROWS 6
#define MATRIX_COLS 15

#define MATRIX_ROW_PINS { PORT4_0, PORT4_1, PORT4_2, PORT4_3, PORT4_4, PORT4_5 }
#define MATRIX_COL_PINS { PORT0_0, PORT0_1, PORT0_2, PORT0_3, PORT0_4, PORT0_5, PORT0_6, PORT0_7, PORT7_1, PORT7_0, PORT3_2, PORT3_3, PORT3_4, PORT3_5, PORT3_6 }

#define DIODE_DIRECTION ROW2COL

#define NUM_ENCODERS 1
#define ENCODERS_PAD_A { PORT2_7 }
#define ENCODERS_PAD_B { PORT3_7 }
#define ENCODER_RESOLUTION 2

//#define NO_PRINT 
//#define NO_DEBUG
#define ACTION_DEBUG

// for ch552, the code size is too small:
//#define NO_ACTION_LAYER
//#define NO_ACTION_ONESHOT
//#define NO_ACTION_TAPPING

#define HOLD_ON_OTHER_KEY_PRESS

/* NKRO report from startup; the transient EEPROM does not keep the setting */
#define FORCE_NKRO
