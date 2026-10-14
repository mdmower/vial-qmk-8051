/********************************** (C) COPYRIGHT *******************************
* File Name          : DEVICE.H
* Author             : MJX
* Version            : V1.00
* Date               : 2021/04/29
* Description        : CH55X Usb device operation related definitions
*******************************************************************************/

#pragma once

/******************************************************************************/
/* The header file contains */
#include "ch555.h"
#include "usb.h"
#include "usb_descriptor.h"

/***********************************************************************************************************************/
/* Usb device related macro definitions */
#define bUEP_R_TOG        0x80      // expected data toggle flag of USB endpoint X receiving (OUT): 0=DATA0, 1=DATA1
#define bUEP_T_TOG        0x40      // prepared data toggle flag of USB endpoint X transmittal (IN): 0=DATA0, 1=DATA1
#define bUEP_R_RES1       0x08      // handshake response type high bit for USB endpoint X receiving (OUT)
#define bUEP_R_RES0       0x04      // handshake response type low bit for USB endpoint X receiving (OUT)
#define MASK_UEP_R_RES    0x0C      // bit mask of handshake response type for USB endpoint X receiving (OUT)
#define UEP_R_RES_ACK     0x00
#define UEP_R_RES_TOUT    0x04
#define UEP_R_RES_NAK     0x08
#define UEP_R_RES_STALL   0x0C
// bUEP_R_RES1 & bUEP_R_RES0: handshake response type for USB endpoint X receiving (OUT)
//   00: ACK (ready)
//   01: no response, time out to host, for non-zero endpoint isochronous transactions
//   10: NAK (busy)
//   11: STALL (error)

#define bUEP_T_RES1       0x02      // handshake response type high bit for USB endpoint X transmittal (IN)
#define bUEP_T_RES0       0x01      // handshake response type low bit for USB endpoint X transmittal (IN)
#define MASK_UEP_T_RES    0x03      // bit mask of handshake response type for USB endpoint X transmittal (IN)
#define UEP_T_RES_ACK     0x00
#define UEP_T_RES_TOUT    0x01
#define UEP_T_RES_NAK     0x02
#define UEP_T_RES_STALL   0x03
// bUEP_T_RES1 & bUEP_T_RES0: handshake response type for USB endpoint X transmittal (IN)
//   00: DATA0 or DATA1 then expecting ACK (ready)
//   01: DATA0 or DATA1 then expecting no response, time out from host, for non-zero endpoint isochronous transactions
//   10: NAK (busy)
//   11: STALL (error)


/************************************************/ 
/* Endpoint size and other definitions */
#define DEF_ENDP0_SIZE             FIXED_CONTROL_ENDPOINT_SIZE
#if KEYBOARD_IN_EPNUM==1 || \
    RAW_IN_EPNUM==1 || \
    SHARED_IN_EPNUM==1 || \
    CONSOLE_IN_EPNUM==1 || \
    MIDI_STREAM_IN_EPNUM==1 || \
    CDC_IN_EPNUM==1 || \
    JOYSTICK_IN_EPNUM==1 || \
    DIGITIZER_IN_EPNUM==1
#define USE_D0_EP1_IN
#endif
#if KEYBOARD_IN_EPNUM==2 || RAW_IN_EPNUM==2 || SHARED_IN_EPNUM==2 || CONSOLE_IN_EPNUM==2 || MIDI_STREAM_IN_EPNUM==2 || CDC_IN_EPNUM==2 || JOYSTICK_IN_EPNUM==2 || DIGITIZER_IN_EPNUM==2
#define USE_D0_EP2_IN
#endif
#if KEYBOARD_IN_EPNUM==3 || RAW_IN_EPNUM==3 || SHARED_IN_EPNUM==3 || CONSOLE_IN_EPNUM==3 || MIDI_STREAM_IN_EPNUM==3 || CDC_IN_EPNUM==3 || JOYSTICK_IN_EPNUM==3 || DIGITIZER_IN_EPNUM==3
#define USE_D0_EP3_IN
#endif
#if KEYBOARD_IN_EPNUM==5 || RAW_IN_EPNUM==5 || SHARED_IN_EPNUM==5 || CONSOLE_IN_EPNUM==5 || MIDI_STREAM_IN_EPNUM==5 || CDC_IN_EPNUM==5 || JOYSTICK_IN_EPNUM==5 || DIGITIZER_IN_EPNUM==5
#define USE_D0_EP5_IN
#endif
#if KEYBOARD_IN_EPNUM==6 || RAW_IN_EPNUM==6 || SHARED_IN_EPNUM==6 || CONSOLE_IN_EPNUM==6 || MIDI_STREAM_IN_EPNUM==6 || CDC_IN_EPNUM==6 || JOYSTICK_IN_EPNUM==6 || DIGITIZER_IN_EPNUM==6
#define USE_D0_EP6_IN
#endif
#if RAW_OUT_EPNUM==1 || CONSOLE_OUT_EPNUM==1 || MIDI_STREAM_OUT_EPNUM==1 || CDC_OUT_EPNUM==1 
#define USE_D0_EP1_OUT
#endif
#if RAW_OUT_EPNUM==2 || CONSOLE_OUT_EPNUM==2 || MIDI_STREAM_OUT_EPNUM==2 || CDC_OUT_EPNUM==2 
#define USE_D0_EP2_OUT
#endif
#if RAW_OUT_EPNUM==3 || CONSOLE_OUT_EPNUM==3 || MIDI_STREAM_OUT_EPNUM==3 || CDC_OUT_EPNUM==3 
#define USE_D0_EP3_OUT
#endif
#if RAW_OUT_EPNUM==4 || CONSOLE_OUT_EPNUM==4 || MIDI_STREAM_OUT_EPNUM==4 || CDC_OUT_EPNUM==4
#define USE_D0_EP4_OUT
#endif

/************************************************/
/* Hid type request */ 
#define DEF_USB_GET_REPORT         0x01           								/* get report */
#define DEF_USB_GET_IDLE           0x02           								/* get idle for key or mouse */
#define DEF_USB_GET_PROTOCOL       0x03          								/* get protocol for bios type */
#define DEF_USB_SET_REPORT         0x09           								/* set report for key */
#define DEF_USB_SET_IDLE           0x0A           								/* set idle for key or mouse */
#define DEF_USB_SET_PROTOCOL       0x0B           								/* set protocol for bios type */

/***********************************************************************************************************************/
/* variable expansion */

extern volatile UINT8  D0SetupReqCode;											/* USB Setup package request code */
extern volatile UINT16 D0SetupLen;												/* USB Setup packet length */
extern volatile PUINT8C pD0Descr;  /* points into __code Flash */
extern volatile UINT8  D0UsbConfig;												/* USB configuration flags */

extern volatile UINT8  USB_EnumStatus;											/* USB enumeration status */	
extern volatile UINT8  USB_SleepStatus;											/* USB sleep state */	
extern volatile UINT8  USB_WakeUpSent;											/* remote wakeup signalled, no host transfer since */

extern volatile UINT8  KB_USB_SetReport;
extern volatile UINT8  Report_Value;											/* HID protocol of interface 0: 0 boot, 1 report */
extern volatile UINT8  Idle_Value;												/* SET_IDLE duration of interface 0, 4 ms units */
extern UINT8 kbd_report[8];														/* current report of interface 0 (ch555_usb_stack.c) */
#ifdef USE_D0_EP1_OUT
extern volatile UINT8  ep1_data_wait;											/* endpoint 1 data waiting flag*/
#endif
#ifdef USE_D0_EP2_OUT
extern volatile UINT8  ep2_data_wait;											/* endpoint 2 data waiting flag*/
#endif
#ifdef USE_D0_EP3_OUT
extern volatile UINT8  ep3_data_wait;											/* endpoint 3 data waiting flag*/
#endif
#ifdef USE_D0_EP4_OUT
extern volatile UINT8  ep4_data_wait;											/* endpoint 4 data waiting flag*/
#endif
/***********************************************************************************************************************/
/* Function expansion */
extern void USB_Device_Init( void );											/* CH55X USB device initialization */
extern void USB_WakeUp_PC( void );												/* CH55X Usb device wakes up computer */
#ifdef CH555_SUSPEND_SLEEP
extern void USB_Sleep( void );													/* MCU sleep while the bus is suspended */
/* Defined by the keyboard: called with interrupts off before the MCU sleeps (key_wake: set up the keys
 * as a wake source) and after it wakes. */
void ch555_sleep_enter_kb( UINT8 key_wake );
void ch555_sleep_exit_kb( void );
#endif

#pragma save
#pragma nooverlay
extern void USB_DeviceInterrupt( void ) __interrupt(INT_NO_USB);
#pragma restore

/***********************************************************************************************************************/


