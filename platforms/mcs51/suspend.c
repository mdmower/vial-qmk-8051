/* Copyright 2017 Fred Sundvik
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

#include "suspend.h"

/** \brief Run when the USB bus enters suspend
 *
 * Called once per suspend by protocol_pre_task() (ch555). The MCU keeps running and scanning, or with
 * CH555_SUSPEND_SLEEP sleeps until bus activity or a key wakes it.
 */
void suspend_power_down(void) {
    suspend_power_down_quantum();
}

/** \brief Run when the USB bus resumes
 *
 * The matrix is scanned whenever the MCU is awake, so the key state is not cleared.
 */
void suspend_wakeup_init(void) {
    suspend_wakeup_init_quantum();
}
