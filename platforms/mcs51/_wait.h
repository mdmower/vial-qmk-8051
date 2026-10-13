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

#include "hal.h"

/* at least ms milliseconds or us microseconds, up to 65535 */
#define wait_ms(ms) mDelaymS(ms)
#define wait_us(us) mDelayuS(us)
#define wait_cpuclock(n)                    \
    do {                                    \
        for (uint16_t i = n; i > 0; i--) {  \
            __asm__("nop");                 \
        }                                   \
    } while (0)
#define CPU_CLOCK F_CPU

/* Not sure about CH55x read delay for changes in the digital input signal.
 * Let's make it two clocks as AVR. */
#ifndef GPIO_INPUT_PIN_DELAY
//#    define GPIO_INPUT_PIN_DELAY 2
#    define GPIO_INPUT_PIN_DELAY 2
#endif

#define waitInputPinDelay() wait_cpuclock(GPIO_INPUT_PIN_DELAY)
