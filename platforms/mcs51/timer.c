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

//#include <stdint.h>
#include "timer.h"
#include "print.h"
#include "mcs51_isr.h"
//#include "atomic_util.h"
//#include "hal.h" 

// counter resolution 1ms
// NOTE: union { uint32_t timer32; struct { uint16_t dummy; uint16_t timer16; }}
//__data volatile uint32_t timer_count;
volatile uint32_t timer_count;

/** \brief timer initialization
 *
 * FIXME: needs doc
 */
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
    EA  = 1; // golbal interrupt enable
}

/** \brief timer clear
 *
 * FIXME: needs doc
 */
inline void timer_clear(void) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        timer_count = 0;
    }
}

/** \brief timer read
 *
 * FIXME: needs doc
 */
inline uint16_t timer_read(void) {
    uint32_t t;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = timer_count;
    }

    return (t & 0xFFFF);
}

/** \brief timer read32
 *
 * FIXME: needs doc
 */
inline uint32_t timer_read32(void) {
    uint32_t t;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = timer_count;
    }

    return t;
}

/** \brief timer elapsed
 *
 * FIXME: needs doc
 */
inline uint16_t timer_elapsed(uint16_t last) {
    uint32_t t;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = timer_count;
    }

    return TIMER_DIFF_16((t & 0xFFFF), last);
}

/** \brief timer elapsed32
 *
 * FIXME: needs doc
 */
inline uint32_t timer_elapsed32(uint32_t last) {
    uint32_t t;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = timer_count;
    }

    return TIMER_DIFF_32(t, last);
}

// excecuted once per 1ms.(excess for just timer count?)
//volatile uint32_t s_tik = 1;
MCS51_ISR(Timer0_ISR, INT_NO_TMR0) {
    //TR0 = 0;
    setTimer0Mode1Val(F_CPU/12/1000);
    //TR0 = 1; not stopping timer might makes it more precise, probably?

    //static uint8_t bf, af;
    //bf = timer_count & 0x00000100;
    timer_count++;
    //af = timer_count & 0x00000100;
    //if ( bf != af ) println("tik");
    //s_tik++;
    //if ( s_tik==0 ) println("tik");
}
