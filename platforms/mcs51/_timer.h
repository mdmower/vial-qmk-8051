/* Copyright 2021 Simon Arlott
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#include "atomic_util.h"
#include "hal.h" 

// The platform is 8-bit, so prefer 16-bit timers to reduce code size
#define FAST_TIMER_T_SIZE 16

#define setTimer0Mode1Val(val) TL0 = (0x10000 - val) & 0xff; \
                               TH0 =((0x10000 - val)>>8) & 0xff

void Timer0_ISR(void) __interrupt (INT_NO_TMR0) ;
