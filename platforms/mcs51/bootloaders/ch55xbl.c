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

#include "bootloader.h"
#include "wait.h"

/*
 * bootloader_jump()
 * -----------------
 * Disables USB and all interrupts, waits for the host to detect the
 * disconnect, then jumps to the WCH factory bootloader at BOOT_LOAD_ADDR
 * (0xF400 on the CH555).
 *
 * With EVISION_OEM_BOOTLOADER defined, it instead erases the Evision OEM
 * bootloader's boot marker and resets (see below). That bootloader has no
 * timeout: the board stays in it, across replugs, until a flash completes.
 * If the erase fails, the reset starts the app again.
 */
void bootloader_jump(void) {
#if defined(EVISION_OEM_BOOTLOADER)
    /*
     * Evision OEM bootloader (0c45:7687, app at 0x2800): it starts the app only while the
     * marker AA 55 33 CC at 0x27C0 is present. Erase that 64-byte page and reset, as the
     * Redragon K580 stock firmware does for its "04 EE EE" command. The bootloader rewrites
     * the marker after a successful flash. Nothing else below 0x2800 is written.
     */
    USB_CTRL = 0;
    EA = 0; // Disabling all interrupts is required.
    wait_us(50000);
    wait_us(50000);

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG |= bCODE_WE;
    ROM_ADDR_H = 0x27;
    ROM_ADDR_L = 0xC0;
    ROM_DATA_HH = 0x80;
    ROM_DATA_HL = 0x00;
    if (ROM_CTRL & bROM_ADDR_OK) {
        ROM_CTRL = ROM_CMD_ERASE;
    }
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG &= ~bCODE_WE;

    mcu_reset();
#elif BOOT_LOAD_ADDR == 0x3800
    USB_CTRL = 0;
    EA = 0; // Disabling all interrupts is required.
    TMOD = 0;
    wait_us(50000);
    wait_us(50000);

    __asm__("lcall #0x3800"); // Jump to bootloader code

    while (1)
      ;
#elif (BOOT_LOAD_ADDR == 0xF400)
#   if defined(QMK_MCU_CH559) || defined(QMK_MCU_CH555)
    USB_CTRL = 0;
    EA = 0; // Disabling all interrupts is required.
    wait_us(50000);
    wait_us(50000);

    __asm__("lcall #0xF400"); // Jump to bootloader code

    while (1)
      ;
#   else
    // todo: not working well, CH549 doesn't support direct jump
#   endif
#endif
}

/*
 * mcu_reset()
 * -----------
 * Performs a software reset of the CH555 by writing the magic sequence
 * into SAFE_MOD to enter safe mode, then setting the bSW_RESET bit in
 * GLOBAL_CFG. Execution does not return.
 *
 * After a software reset the CPU starts at address 0x0000 and bBOOT_LOAD
 * is 0 (datasheet 7.3.3). With EVISION_OEM_BOOTLOADER, 0x0000 is the
 * Evision OEM bootloader, which starts the app at 0x2800 while its boot
 * marker at 0x27C0 is present.
 */
void mcu_reset(void) {
    EA = 0;           /* disable all interrupts first */
    SAFE_MOD = 0x55;  /* enter safe mode step 1 */
    SAFE_MOD = 0xAA;  /* enter safe mode step 2 */
    GLOBAL_CFG |= bSW_RESET; /* trigger software reset - does not return */
    while (1);        /* should never reach here */
}
