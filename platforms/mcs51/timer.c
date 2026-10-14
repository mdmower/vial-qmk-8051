/*
Copyright 2011 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "timer.h"
#include "mcs51_isr.h"

// counter resolution 1ms
volatile uint32_t timer_count;

/** \brief timer initialization */
void timer_init(void) {
    // Set Timer0 clock = F_CPU/12
    T2MOD &= ~bT0_CLK;

    // Set Timer 0 mode 1 - 16bit no auto reload 
    TMOD = TMOD & 0xf0 | 0x01; //keep 4 MSB, set 4 LSB to value 1

    // Counts per 1 ms tick at F_CPU/12: 2000 at 24 MHz; 2666 at 32 MHz (2666.67 truncated, tick 0.025% short)
    setTimer0Mode1Val(F_CPU/12/1000);

    // Let timer0 run:
    TR0 = 1;

    // Enable Timer0 Interrupt
    ET0 = 1;
    EA  = 1; // global interrupt enable
}

/** \brief timer clear */
inline void timer_clear(void) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        timer_count = 0;
    }
}

/** \brief timer read */
inline uint16_t timer_read(void) {
    uint32_t t;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = timer_count;
    }

    return (t & 0xFFFF);
}

/** \brief timer read32 */
inline uint32_t timer_read32(void) {
    uint32_t t;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = timer_count;
    }

    return t;
}

/** \brief timer elapsed */
inline uint16_t timer_elapsed(uint16_t last) {
    uint32_t t;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = timer_count;
    }

    return TIMER_DIFF_16((t & 0xFFFF), last);
}

/** \brief timer elapsed32 */
inline uint32_t timer_elapsed32(uint32_t last) {
    uint32_t t;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = timer_count;
    }

    return TIMER_DIFF_32(t, last);
}

// Once per ms. Mode 1 does not reload by itself: the counts from the overflow to this reload are lost (8.6 per tick
// measured at 24 MHz under the OEM bootloader), so a tick is slightly longer than 1 ms.
MCS51_ISR(Timer0_ISR, INT_NO_TMR0) {
    setTimer0Mode1Val(F_CPU/12/1000);
    timer_count++;
}
