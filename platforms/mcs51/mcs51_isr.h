/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

/*
 * MCS51_ISR(name, num) { ... } defines the handler of interrupt num (INT_NO_* in ch555.h). The declaration
 * that main.c sees stays `void name(void) __interrupt(num);`, so that SDCC puts the handler in its vector table.
 *
 * Without EVISION_OEM_BOOTLOADER the image starts at 0x0000 and the vectors jump to SDCC's handlers, which
 * save the registers they use and end with RETI.
 *
 * With EVISION_OEM_BOOTLOADER the vectors belong to the Evision OEM bootloader. Its stub for each vector
 * (0x1858 + 0x4C each) saves ACC, B, DPH, DPL, PSW and R0-R7, sets PSW to 0 (register bank 0), calls the
 * app's entry in the table at 0x3000 (util/evision_oem_image.py), restores the registers and ends with RETI.
 * The app's handler must end with RET, as the stock firmware's handlers do. A RETI in the handler ends the
 * interrupt for the priority logic before the stub has restored the registers: a pending interrupt of the
 * same priority is then taken inside the stub and nests on its stack frame (30 bytes per interrupt), and an
 * interrupt at high priority would end, with the stub's RETI, the low-priority interrupt that it preempted.
 * Here SDCC's vector slot jumps to a naked function, which jumps to name_body(): a plain function, which
 * saves nothing and returns to the stub with RET. The stub saves every register that compiled code changes,
 * except SDCC's bit registers (`bits`), which no function in this build uses. SDCC ends some handlers with a
 * jump to sdcc_atomic_maybe_rollback (restartable atomics), which reads the return address on top of the
 * stack; under the stub that is the stub's address, so the check never applied in these builds, and no code
 * here uses SDCC's atomic exchange routines.
 */
#ifdef EVISION_OEM_BOOTLOADER
#    define MCS51_ISR(name, num)                                                       \
        void name##_body(void);                                                        \
        void name(void) __interrupt(num) __naked { __asm__("ljmp _" #name "_body"); } \
        void name##_body(void)
#else
#    define MCS51_ISR(name, num) void name(void) __interrupt(num)
#endif
