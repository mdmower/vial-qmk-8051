/* Copyright 2021 QMK
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <stdint.h>

// Use 8bit to represent a pin: <5bit - port><3bit - pin>
// Follow AVR port naming convention, P0 ~ PA, P1 ~ PB, ...
#define PORT_SHIFTER 3

// TODO implement P6 for CH555
#if defined(QMK_MCU_CH552) 
//#    define ADDRESS_BASE 0x00
#    define PORT1 0x1
#    define PORT3 0x3
#elif defined(QMK_MCU_CH555)
#    define PORT0 0x0
#    define PORT1 0x1
#    define PORT2 0x2
#    define PORT3 0x3
#    define PORT4 0x4
#    define PORT5 0x5
//#    define PORT6 0x6
#    define PORT7 0x7
#endif

//__code uint8_t pin_to_pinmask[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
extern __code uint8_t pin_to_pinmask[];
#define PINDEF(port, pin) ((PORT##port << PORT_SHIFTER) | pin)

// Get Pin Mask from pin def
//#define PIN_MASK(p) 1<<(p & 0x07)
// The damn 8051 don't have barrel shifter, so do this:
#define PIN_MASK(p) pin_to_pinmask[p & ((1<<PORT_SHIFTER)-1)]
// Get Port ID from pin def
#define PORTx(p) (p >> PORT_SHIFTER)
// Get Pin ID from pin def
#define PINx(p) (p & ((1<<PORT_SHIFTER)-1))

/* I/O pins */
#ifdef PORT0
#    define PORT0_0 PINDEF(0, 0)
#    define PORT0_1 PINDEF(0, 1)
#    define PORT0_2 PINDEF(0, 2)
#    define PORT0_3 PINDEF(0, 3)
#    define PORT0_4 PINDEF(0, 4)
#    define PORT0_5 PINDEF(0, 5)
#    define PORT0_6 PINDEF(0, 6)
#    define PORT0_7 PINDEF(0, 7)
#endif
#ifdef PORT1
#    define PORT1_0 PINDEF(1, 0)
#    define PORT1_1 PINDEF(1, 1)
#    define PORT1_2 PINDEF(1, 2)
#    define PORT1_3 PINDEF(1, 3)
#    define PORT1_4 PINDEF(1, 4)
#    define PORT1_5 PINDEF(1, 5)
#    define PORT1_6 PINDEF(1, 6)
#    define PORT1_7 PINDEF(1, 7)
#endif
#ifdef PORT2
#    define PORT2_0 PINDEF(2, 0)
#    define PORT2_1 PINDEF(2, 1)
#    define PORT2_2 PINDEF(2, 2)
#    define PORT2_3 PINDEF(2, 3)
#    define PORT2_4 PINDEF(2, 4)
#    define PORT2_5 PINDEF(2, 5)
#    define PORT2_6 PINDEF(2, 6)
#    define PORT2_7 PINDEF(2, 7)
#endif
#ifdef PORT3
#    define PORT3_0 PINDEF(3, 0)
#    define PORT3_1 PINDEF(3, 1)
#    define PORT3_2 PINDEF(3, 2)
#    define PORT3_3 PINDEF(3, 3)
#    define PORT3_4 PINDEF(3, 4)
#    define PORT3_5 PINDEF(3, 5)
#    define PORT3_6 PINDEF(3, 6)
#    define PORT3_7 PINDEF(3, 7)
#endif
#ifdef PORT4
#    define PORT4_0 PINDEF(4, 0)
#    define PORT4_1 PINDEF(4, 1)
#    define PORT4_2 PINDEF(4, 2)
#    define PORT4_3 PINDEF(4, 3)
#    define PORT4_4 PINDEF(4, 4)
#    define PORT4_5 PINDEF(4, 5)
#    define PORT4_6 PINDEF(4, 6)
#    define PORT4_7 PINDEF(4, 7)
#endif
#ifdef PORT5
#    define PORT5_0 PINDEF(5, 0)
#    define PORT5_1 PINDEF(5, 1)
#    define PORT5_2 PINDEF(5, 2)
#    define PORT5_3 PINDEF(5, 3)
#    define PORT5_4 PINDEF(5, 4)
#    define PORT5_5 PINDEF(5, 5)
#    define PORT5_6 PINDEF(5, 6)
#    define PORT5_7 PINDEF(5, 7)
#endif
#ifdef PORT6
#    define PORT6_0 PINDEF(6, 0)
#    define PORT6_1 PINDEF(6, 1)
#    define PORT6_2 PINDEF(6, 2)
#    define PORT6_3 PINDEF(6, 3)
#    define PORT6_4 PINDEF(6, 4)
#    define PORT6_5 PINDEF(6, 5)
#    define PORT6_6 PINDEF(6, 6)
#    define PORT6_7 PINDEF(6, 7)
#endif
#ifdef PORT7
// P7 has only P7.0 and P7.1; its register holds their DIR, OUT_PU and IN bits.
#    define PORT7_0 PINDEF(7, 0)
#    define PORT7_1 PINDEF(7, 1)
#endif
