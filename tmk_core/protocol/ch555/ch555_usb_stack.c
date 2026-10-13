/*
 * Copyright 2012 Jun Wako <wakojun@gmail.com>
 * This file is based on:
 *     LUFA-120219/Demos/Device/Lowlevel/KeyboardMouse
 *     LUFA-120219/Demos/Device/Lowlevel/GenericHID
 */

/*
             LUFA Library
     Copyright (C) Dean Camera, 2012.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
 * QMK host driver and protocol_* hooks for the CH555 USB device (usb_device.c).
 *
 * send_report(ep, ...) and read_EP(ep, ...) call send_report_EP<ep>() and read_EP<ep>(), with the QMK
 * endpoint numbers of usb_descriptor.h: EP1 keyboard, EP2 shared (NKRO, extra keys), EP5 and EP3 raw HID.
 */

#define send_report(endpoint, report, size) _send_report_EP(endpoint, report, size)
#define _send_report_EP(endpoint, report, size) send_report_EP##endpoint(report, size)

#define read_EP(endpoint, report, size) _read_EP(endpoint, report, size)
#define _read_EP(endpoint, report, size) read_EP##endpoint(report, size)

#include "report.h"
#include "host.h"
#include "host_driver.h"
#include "keyboard.h"
#include "action.h"
#include "led.h"
#include "debug.h"
#include "suspend.h"
#include "wait.h"
#include "timer.h"

#include "usb_descriptor.h"
#include "ch555_usb_stack.h"
#include "hal.h"        // CfgFsys()
#include "usb_device.h" // USB device and endpoint registers, endpoint buffers
#include "usb_device_state.h"

#if defined(CONSOLE_ENABLE) || defined(MIDI_ENABLE) || defined(VIRTSER_ENABLE)
#    error "CONSOLE_ENABLE, MIDI_ENABLE and VIRTSER_ENABLE are not implemented in the CH555 protocol"
#endif

#ifdef NKRO_ENABLE
#    include "keycode_config.h"

extern keymap_config_t keymap_config;
#endif

#ifdef RAW_ENABLE
#    include "raw_hid.h"
#endif

uint8_t keyboard_idle = 0;
/* 0: Boot Protocol, 1: Report Protocol(default); follows SET_PROTOCOL (Report_Value) in protocol_post_task() */
uint8_t        keyboard_protocol  = 1;
static uint8_t keyboard_led_state = 0;

static report_keyboard_t keyboard_report_sent;

/* Host driver */
static uint8_t keyboard_leds(void);
static void    send_keyboard(report_keyboard_t *report);
static void    send_mouse(report_mouse_t *report);
static void    send_extra(report_extra_t *report);
host_driver_t  ch555_usb_stack_driver = {keyboard_leds, send_keyboard, send_mouse, send_extra};

#ifdef USE_D0_EP2_IN
/* The shared endpoint (EP2 IN) holds one report until the host reads it. A report that finds
 * it busy is kept here, the latest one per report ID, and sent from protocol_post_task(). */
static report_keyboard_t nkro_pending;
static bool              nkro_pending_set = false;
static report_extra_t    extra_pending[2]; /* [0] REPORT_ID_SYSTEM, [1] REPORT_ID_CONSUMER */
static bool              extra_pending_set[2] = {false, false};
#endif

/*******************************************************************************
 * Endpoints
 ******************************************************************************/
/* Remote wakeup, if the host enabled it and the bus is suspended: one resume signal per suspend. Until the
 * host makes a transfer (USB_WakeUpSent), later reports, and the retries of a report that found its endpoint
 * busy, wait without signalling again. */
static void wake_host(void) {
    if (USB_SleepStatus == 0x03 && !USB_WakeUpSent) USB_WakeUp_PC();
}

/* An IN endpoint answers ACK while it holds a report the host has not read yet; the USB interrupt sets
 * it back to NAK when the host has read it. */

#ifdef USE_D0_EP1_IN
void send_report_EP1(void *report, size_t size) {
    if (!USB_EnumStatus) return;
    wake_host();

    // Check if write ready for a polling interval around 10ms
    uint8_t timeout = 255;
    while (timeout-- && ((D0_EP1RES & MASK_UEP_X_RES) == UEP_X_RES_ACK)) {
        wait_us(40);
    }
    if ((D0_EP1RES & MASK_UEP_X_RES) == UEP_X_RES_ACK) return;

    memcpy(pUSB_BUF_DEV0 + UX_EP1_ADDR, (uint8_t *)report, size);
    D0_EP1T_L = size;
    D0_EP1RES = D0_EP1RES & ~MASK_UEP_X_RES | UEP_X_RES_ACK;
}
#endif
#ifdef USE_D0_EP2_IN
/* Shared endpoint (NKRO, extra keys). Does not wait: returns false if the endpoint still holds the
 * previous report. */
bool send_report_EP2(void *report, size_t size) {
    if (!USB_EnumStatus) return false;
    wake_host();
    if ((D0_EP2RES & MASK_UEP_X_RES) == UEP_X_RES_ACK) return false;
    memcpy(pUSB_BUF_DEV0 + UX_EP2_ADDR, (uint8_t *)report, size);
    D0_EP2T_L = size;
    D0_EP2RES = D0_EP2RES & ~MASK_UEP_X_RES | UEP_X_RES_ACK;
    return true;
}
#endif
#ifdef USE_D0_EP3_IN
void send_report_EP3(void *report, size_t size) {
    if (!USB_EnumStatus) return;
    wake_host();

    // Check if write ready for a polling interval around 10ms
    uint8_t timeout = 255;
    while (timeout-- && ((D0_EP3RES & MASK_UEP_X_RES) == UEP_X_RES_ACK)) {
        wait_us(40);
    }
    if ((D0_EP3RES & MASK_UEP_X_RES) == UEP_X_RES_ACK) return;

    memcpy(pUSB_BUF_DEV0 + UX_EP3_ADDR, (uint8_t *)report, size);
    D0_EP3T_L = size;
    D0_EP3RES = D0_EP3RES & ~MASK_UEP_X_RES | UEP_X_RES_ACK;
}
#endif
#ifdef USE_D0_EP4_IN
void send_report_EP4(void *report, size_t size) {
    if (!USB_EnumStatus) return;
    wake_host();

    // Check if write ready for a polling interval around 10ms
    uint8_t timeout = 255;
    while (timeout-- && ((D0_EP4RES & MASK_UEP_X_RES) == UEP_X_RES_ACK)) {
        wait_us(40);
    }
    if ((D0_EP4RES & MASK_UEP_X_RES) == UEP_X_RES_ACK) return;

    memcpy(pUSB_BUF_DEV0 + UX_EP4_ADDR, (uint8_t *)report, size);
    D0_EP4T_L = size;
    D0_EP4RES = D0_EP4RES & ~MASK_UEP_X_RES | UEP_X_RES_ACK;
}
#endif
#ifdef USE_D0_EP5_IN
void send_report_EP5(void *report, size_t size) {
    if (!USB_EnumStatus) return;
    wake_host();
    uint8_t timeout = 255;
    while (timeout-- && ((D0_EP5RES & MASK_UEP_X_RES) == UEP_X_RES_ACK)) {
        wait_us(40);
    }
    if ((D0_EP5RES & MASK_UEP_X_RES) == UEP_X_RES_ACK) return;
    memcpy(pUSB_BUF_DEV0 + UX_EP5_ADDR, (uint8_t *)report, size);
    D0_EP5T_L = size;
    D0_EP5RES = D0_EP5RES & ~MASK_UEP_X_RES | UEP_X_RES_ACK;
}
#endif

/* An OUT endpoint answers NAK from the packet's arrival (USB interrupt) until it is read here. */
#ifdef USE_D0_EP1_OUT
bool read_EP1(void *report, size_t size) {
    if (ep1_data_wait) {
        ep1_data_wait = 0;
        memcpy(report, pUSB_BUF_DEV0 + UX_EP1_ADDR, size);
        D0_EP1RES = D0_EP1RES & ~MASK_UEP_X_RES | UEP_X_RES_ACK;
        return true;
    }
    return false;
}
#endif
#ifdef USE_D0_EP2_OUT
bool read_EP2(void *report, size_t size) {
    if (ep2_data_wait) {
        ep2_data_wait = 0;
        memcpy(report, pUSB_BUF_DEV0 + UX_EP2_ADDR, size);
        D0_EP2RES = D0_EP2RES & ~MASK_UEP_X_RES | UEP_X_RES_ACK;
        return true;
    }
    return false;
}
#endif
#ifdef USE_D0_EP3_OUT
bool read_EP3(void *report, size_t size) {
    if (ep3_data_wait) {
        ep3_data_wait = 0;
        memcpy(report, pUSB_BUF_DEV0 + UX_EP3_ADDR, size);
        D0_EP3RES = D0_EP3RES & ~MASK_UEP_X_RES | UEP_X_RES_ACK;
        return true;
    }
    return false;
}
#endif
#ifdef USE_D0_EP4_OUT
bool read_EP4(void *report, size_t size) {
    if (ep4_data_wait) {
        ep4_data_wait = 0;
        memcpy(report, pUSB_BUF_DEV0 + UX_EP4_ADDR, size);
        D0_EP4RES = D0_EP4RES & ~MASK_UEP_X_RES | UEP_X_RES_ACK;
        return true;
    }
    return false;
}
#endif

/*******************************************************************************
 * Raw HID
 ******************************************************************************/
#ifdef RAW_ENABLE

void raw_hid_send(uint8_t *data, uint8_t length) {
    if (length != RAW_EPSIZE) return;
    send_report(RAW_IN_EPNUM, data, RAW_EPSIZE);
}

#    ifndef VIA_ENABLE
/* SDCC has no weak symbols, so a keymap cannot replace this: without VIA, received reports are
 * dropped. */
void raw_hid_receive(uint8_t *data, uint8_t length) {}
#    endif

static void raw_hid_task(void) {
    uint8_t data[RAW_EPSIZE];
    bool    data_read = false;

    if (!USB_EnumStatus) return;

    data_read = read_EP(RAW_OUT_EPNUM, data, RAW_EPSIZE);
    if (data_read) {
        raw_hid_receive(data, sizeof(data));
    }
}
#endif

/*******************************************************************************
 * Host driver
 ******************************************************************************/
static uint8_t keyboard_leds(void) {
    return keyboard_led_state;
}

static void send_keyboard(report_keyboard_t *report) {
    uint8_t size = KEYBOARD_REPORT_SIZE;

    /* If we're in Boot Protocol, don't send any report ID or other funky fields */
    if (!keyboard_protocol) {
        send_report(KEYBOARD_IN_EPNUM, &report->mods, 8);
    } else {
#ifdef NKRO_ENABLE
        if (keymap_config.nkro) {
            size = sizeof(struct nkro_report);
#    ifdef USE_D0_EP2_IN
            nkro_pending_set = !send_report(SHARED_IN_EPNUM, report, size);
            if (nkro_pending_set) {
                memcpy(&nkro_pending, report, size);
            }
#    else
            send_report(SHARED_IN_EPNUM, report, size);
#    endif
            keyboard_report_sent = *report;
            return;
        }
#endif

        send_report(KEYBOARD_IN_EPNUM, report, size);
    }

    keyboard_report_sent = *report;
}

static void send_mouse(report_mouse_t *report) {
#ifdef MOUSE_ENABLE
    send_report(MOUSE_IN_EPNUM, report, sizeof(report_mouse_t));
#endif
}

static void send_extra(report_extra_t *report) {
#ifdef EXTRAKEY_ENABLE
#    ifdef USE_D0_EP2_IN
    uint8_t i = (report->report_id == REPORT_ID_CONSUMER) ? 1 : 0;
    extra_pending_set[i] = !send_report(SHARED_IN_EPNUM, report, sizeof(report_extra_t));
    if (extra_pending_set[i]) {
        memcpy(&extra_pending[i], report, sizeof(report_extra_t));
    }
#    else
    send_report(SHARED_IN_EPNUM, report, sizeof(report_extra_t));
#    endif
#endif
}

#ifdef USE_D0_EP2_IN
/* Send one report kept in nkro_pending or extra_pending, if the shared endpoint is free. */
static void send_pending_shared_report(void) {
    uint8_t i;
    if (nkro_pending_set) {
        nkro_pending_set = !send_report(SHARED_IN_EPNUM, &nkro_pending, sizeof(struct nkro_report));
        return;
    }
    for (i = 0; i < 2; i++) {
        if (extra_pending_set[i]) {
            extra_pending_set[i] = !send_report(SHARED_IN_EPNUM, &extra_pending[i], sizeof(report_extra_t));
            return;
        }
    }
}
#endif

void send_joystick(report_joystick_t *report) {
#ifdef JOYSTICK_ENABLE
    send_report(JOYSTICK_IN_EPNUM, report, sizeof(report_joystick_t));
#endif
}

void send_programmable_button(report_programmable_button_t *report) {
#ifdef PROGRAMMABLE_BUTTON_ENABLE
    send_report(SHARED_IN_EPNUM, report, sizeof(report_programmable_button_t));
#endif
}

void send_digitizer(report_digitizer_t *report) {
#ifdef DIGITIZER_ENABLE
    send_report(DIGITIZER_IN_EPNUM, report, sizeof(report_digitizer_t));
#endif
}

/*******************************************************************************
 * main
 ******************************************************************************/
static void setup_mcu(void) {
    CfgFsys();
    wait_ms(10); // wait for the clock to settle
#ifndef NO_PRINT
    mInitSTDIO();
    println("init UART0 done");
#endif
}

static void setup_usb(void) {
    USB_Device_Init();
}

void protocol_setup(void) {
    setup_mcu();
    usb_device_state_init();
}

void protocol_pre_init(void) {
    setup_usb();
    println("done init usb");
}

void protocol_post_init(void) {
    host_set_driver(&ch555_usb_stack_driver);

    /* Wait up to 5 s for the host to configure D0 (USB_EnumStatus is set by its SET_CONFIGURATION). */
    {
        uint16_t timeout = 5000;
        while (!USB_EnumStatus && timeout) {
            wait_ms(1);
            timeout--;
        }
    }

#ifdef USE_D0_EP1_IN
    /* Send an initial empty keyboard report on EP1 immediately.
     * Windows' HidUsb driver has a ~500ms timeout for the first
     * interrupt IN response after SET_CONFIGURATION. If EP1 stays
     * in NAK (because keyboard_task hasn't run yet to detect any
     * key changes), Windows aborts the pipe and reports Code 43.
     * Pre-loading an empty report ensures Windows gets data in time.
     */
    if (USB_EnumStatus) {
        static const uint8_t empty_report[8] = {0};
        send_report_EP1((void *)empty_report, 8);
    }
#endif
}

#ifndef USB_RESUME_DELAY_MS
/* Bus active this long before the wakeup hooks run; the bus switches between suspend and active
 * several times while a PC resumes from standby. */
#    define USB_RESUME_DELAY_MS 500
#endif

void protocol_pre_task(void) {
#if !defined(NO_USB_STARTUP_CHECK)
    /* Run the QMK suspend hooks when the bus enters or leaves suspend. Unlike LUFA/ChibiOS this does
     * not wait in a loop: the matrix is still scanned while suspended, and a key press wakes the host
     * through wake_host() in the send_report_* functions (if the host enabled remote wakeup).
     * After that signal, the bus counts as suspended until the host makes a transfer. */
    static bool     suspended = false;
    static uint16_t suspend_time;

    if ((USB_IF & bUX_SUSPEND) != 0 || USB_WakeUpSent) {
        suspend_time = timer_read();
        if (!suspended) {
            suspended = true;
            suspend_power_down();
        }
    } else if (suspended && timer_elapsed(suspend_time) >= USB_RESUME_DELAY_MS) {
        suspended = false;
        suspend_wakeup_init();
    }
#endif
}

void protocol_post_task(void) {
#ifdef RAW_ENABLE
    raw_hid_task();
#endif

    /* Sync LED state from ISR-updated KB_USB_SetReport to keyboard_led_state,
     * then call led_set() safely from the main loop (not inside the USB ISR). */
    if (KB_USB_SetReport != keyboard_led_state) {
        keyboard_led_state = KB_USB_SetReport;
        led_set(keyboard_led_state);
    }

    /* SET_PROTOCOL (USB interrupt) takes effect here. QMK core builds boot reports (6 key bytes, EP1) in boot
     * protocol and NKRO reports (bitmap, shared endpoint) in report protocol; clear_keyboard() empties the
     * report and sends it in the new format, as LUFA and ChibiOS do. */
    if (Report_Value != keyboard_protocol) {
        keyboard_protocol = Report_Value;
        clear_keyboard();
    }

#ifdef USE_D0_EP2_IN
    send_pending_shared_report();
#endif
}
