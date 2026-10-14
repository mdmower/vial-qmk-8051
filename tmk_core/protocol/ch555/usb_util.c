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
#include "usb_util.h"
#include "usb_device.h"

/* QMK calls usb_disconnect() and usb_connected_state() only for split keyboards (quantum/split_common), which have
 * no transport driver on MCS51. */
void usb_disconnect(void) {}

/* Configured: SET_CONFIGURATION with a nonzero value (ChibiOS: USB_ACTIVE) */
bool usb_connected_state(void) {
    return (USB_EnumStatus != 0);
}
