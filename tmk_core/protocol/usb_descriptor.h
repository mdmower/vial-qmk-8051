/*
 * Copyright 2012,2013 Jun Wako <wakojun@gmail.com>
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
    Copyright 2012  Dean Camera (dean [at] fourwalledcubicle [dot] com)
    Copyright 2010  Denver Gingerich (denver [at] ossguy [dot] com)

    Permission to use, copy, modify, distribute, and sell this
    software and its documentation for any purpose is hereby granted
    without fee, provided that the above copyright notice appear in
    all copies and that both that the copyright notice and this
    permission notice and warranty disclaimer appear in supporting
    documentation, and that the name of the author not be used in
    advertising or publicity pertaining to distribution of the
    software without specific, written prior permission.

    The author disclaim all warranties with regard to this
    software, including all implied warranties of merchantability
    and fitness.  In no event shall the author be liable for any
    special, indirect or consequential damages or any damages
    whatsoever resulting from loss of use, data or profits, whether
    in an action of contract, negligence or other tortious action,
    arising out of or in connection with the use or performance of
    this software.
*/

/** \file
 *
 *  Header file for Descriptors.c.
 */

#pragma once

#include <LUFA/Drivers/USB/USB.h>

#ifdef PROTOCOL_CHIBIOS
#    include <hal.h>
#    if STM32_USB_USE_OTG1 == TRUE
#        define USB_ENDPOINTS_ARE_REORDERABLE
#    endif
#endif

/*
 * USB descriptor structure
 */
typedef struct {
    USB_Descriptor_Configuration_Header_t Config;

#ifndef KEYBOARD_SHARED_EP
    // Keyboard HID Interface
    USB_Descriptor_Interface_t Keyboard_Interface;
    USB_HID_Descriptor_HID_t   Keyboard_HID;
    USB_Descriptor_Endpoint_t  Keyboard_INEndpoint;
#else
    // Shared Interface
    USB_Descriptor_Interface_t Shared_Interface;
    USB_HID_Descriptor_HID_t   Shared_HID;
    USB_Descriptor_Endpoint_t  Shared_INEndpoint;
#endif

#ifndef QMK_MCU_CH555
#ifdef RAW_ENABLE
    // Raw HID Interface
    USB_Descriptor_Interface_t Raw_Interface;
    USB_HID_Descriptor_HID_t   Raw_HID;
    USB_Descriptor_Endpoint_t  Raw_INEndpoint;
    USB_Descriptor_Endpoint_t  Raw_OUTEndpoint;
#endif
#endif /* !QMK_MCU_CH555 */

#if defined(MOUSE_ENABLE) && !defined(MOUSE_SHARED_EP)
    // Mouse HID Interface
    USB_Descriptor_Interface_t Mouse_Interface;
    USB_HID_Descriptor_HID_t   Mouse_HID;
    USB_Descriptor_Endpoint_t  Mouse_INEndpoint;
#endif

#ifndef QMK_MCU_CH555
#if defined(SHARED_EP_ENABLE) && !defined(KEYBOARD_SHARED_EP)
    // Shared Interface
    USB_Descriptor_Interface_t Shared_Interface;
    USB_HID_Descriptor_HID_t   Shared_HID;
    USB_Descriptor_Endpoint_t  Shared_INEndpoint;
#endif

#ifdef CONSOLE_ENABLE
    // Console HID Interface
    USB_Descriptor_Interface_t Console_Interface;
    USB_HID_Descriptor_HID_t   Console_HID;
    USB_Descriptor_Endpoint_t  Console_INEndpoint;
    USB_Descriptor_Endpoint_t  Console_OUTEndpoint;
#endif
#endif /* !QMK_MCU_CH555 */

#ifdef MIDI_ENABLE
    USB_Descriptor_Interface_Association_t Audio_Interface_Association;
    // MIDI Audio Control Interface
    USB_Descriptor_Interface_t          Audio_ControlInterface;
    USB_Audio_Descriptor_Interface_AC_t Audio_ControlInterface_SPC;
    // MIDI Audio Streaming Interface
    USB_Descriptor_Interface_t                Audio_StreamInterface;
    USB_MIDI_Descriptor_AudioInterface_AS_t   Audio_StreamInterface_SPC;
    USB_MIDI_Descriptor_InputJack_t           MIDI_In_Jack_Emb;
    USB_MIDI_Descriptor_InputJack_t           MIDI_In_Jack_Ext;
    USB_MIDI_Descriptor_OutputJack_t          MIDI_Out_Jack_Emb;
    USB_MIDI_Descriptor_OutputJack_t          MIDI_Out_Jack_Ext;
    USB_Audio_Descriptor_StreamEndpoint_Std_t MIDI_In_Jack_Endpoint;
    USB_MIDI_Descriptor_Jack_Endpoint_t       MIDI_In_Jack_Endpoint_SPC;
    USB_Audio_Descriptor_StreamEndpoint_Std_t MIDI_Out_Jack_Endpoint;
    USB_MIDI_Descriptor_Jack_Endpoint_t       MIDI_Out_Jack_Endpoint_SPC;
#endif

#ifdef VIRTSER_ENABLE
    USB_Descriptor_Interface_Association_t CDC_Interface_Association;
    // CDC Control Interface
    USB_Descriptor_Interface_t            CDC_CCI_Interface;
    USB_CDC_Descriptor_FunctionalHeader_t CDC_Functional_Header;
    USB_CDC_Descriptor_FunctionalACM_t    CDC_Functional_ACM;
    USB_CDC_Descriptor_FunctionalUnion_t  CDC_Functional_Union;
    USB_Descriptor_Endpoint_t             CDC_NotificationEndpoint;
    // CDC Data Interface
    USB_Descriptor_Interface_t CDC_DCI_Interface;
    USB_Descriptor_Endpoint_t  CDC_DataOutEndpoint;
    USB_Descriptor_Endpoint_t  CDC_DataInEndpoint;
#endif

#if defined(JOYSTICK_ENABLE) && !defined(JOYSTICK_SHARED_EP)
    // Joystick HID Interface
    USB_Descriptor_Interface_t Joystick_Interface;
    USB_HID_Descriptor_HID_t   Joystick_HID;
    USB_Descriptor_Endpoint_t  Joystick_INEndpoint;
#endif

#if defined(DIGITIZER_ENABLE) && !defined(DIGITIZER_SHARED_EP)
    // Digitizer HID Interface
    USB_Descriptor_Interface_t Digitizer_Interface;
    USB_HID_Descriptor_HID_t   Digitizer_HID;
    USB_Descriptor_Endpoint_t  Digitizer_INEndpoint;
#endif
} USB_Descriptor_Configuration_t;

typedef struct {
    USB_Descriptor_Configuration_Header_t Config;
#ifdef SHARED_EP_ENABLE
    USB_Descriptor_Interface_t Shared_Interface;
    USB_HID_Descriptor_HID_t   Shared_HID;
    USB_Descriptor_Endpoint_t  Shared_INEndpoint;
#endif
} ATTR_PACKED USB_Descriptor_Configuration_D1_t;

typedef struct {
    USB_Descriptor_Configuration_Header_t Config;

#ifdef RAW_ENABLE
    // Raw HID Interface
    USB_Descriptor_Interface_t Raw_Interface;
    USB_HID_Descriptor_HID_t   Raw_HID;
    USB_Descriptor_Endpoint_t  Raw_INEndpoint;
    USB_Descriptor_Endpoint_t  Raw_OUTEndpoint;
#endif

#ifdef CONSOLE_ENABLE
    // Console HID Interface
    USB_Descriptor_Interface_t Console_Interface;
    USB_HID_Descriptor_HID_t   Console_HID;
    USB_Descriptor_Endpoint_t  Console_INEndpoint;
    USB_Descriptor_Endpoint_t  Console_OUTEndpoint;
#endif
} ATTR_PACKED USB_Descriptor_Configuration_D2_t;

// Hub Descriptor (Type 0x29)
typedef struct {
    uint8_t  bDescLength;
    uint8_t  bDescriptorType;
    uint8_t  bNbrPorts;
    uint16_t wHubCharacteristics;
    uint8_t  bPwrOn2PwrGood;
    uint8_t  bHubContrCurrent;
    uint8_t  DeviceRemovable;
    uint8_t  PortPwrCtrlMask;
} ATTR_PACKED USB_Descriptor_Hub_t;

typedef struct {
    USB_Descriptor_Configuration_Header_t Config;
    USB_Descriptor_Interface_t            Hub_Interface;
    USB_Descriptor_Endpoint_t             Hub_INEndpoint;
} ATTR_PACKED USB_Descriptor_Hub_Configuration_t;


/*
 * Interface indexes
 */
enum usb_interfaces {
#ifndef KEYBOARD_SHARED_EP
    KEYBOARD_INTERFACE,
#else
    SHARED_INTERFACE,
#    define KEYBOARD_INTERFACE SHARED_INTERFACE
#endif

// It is important that the Raw HID interface is at a constant
// interface number, to support Linux/OSX platforms and chrome.hid
// If Raw HID is enabled, let it be always 1.
#ifdef RAW_ENABLE
    RAW_INTERFACE,
#endif

#if defined(MOUSE_ENABLE) && !defined(MOUSE_SHARED_EP)
    MOUSE_INTERFACE,
#endif

#if defined(SHARED_EP_ENABLE) && !defined(KEYBOARD_SHARED_EP)
    SHARED_INTERFACE,
#endif

#ifdef CONSOLE_ENABLE
    CONSOLE_INTERFACE,
#endif

#ifdef MIDI_ENABLE
    AC_INTERFACE,
    AS_INTERFACE,
#endif

#ifdef VIRTSER_ENABLE
    CCI_INTERFACE,
    CDI_INTERFACE,
#endif

#if defined(JOYSTICK_ENABLE) && !defined(JOYSTICK_SHARED_EP)
    JOYSTICK_INTERFACE,
#endif

#if defined(DIGITIZER_ENABLE) && !defined(DIGITIZER_SHARED_EP)
    DIGITIZER_INTERFACE,
#endif
    TOTAL_INTERFACES
};

#define NEXT_EPNUM __COUNTER__

/*
 * Endpoint numbers
 */
#ifndef QMK_MCU_CH555

enum usb_endpoints {
    __unused_epnum__ = NEXT_EPNUM, // Endpoint numbering starts at 1

#ifndef KEYBOARD_SHARED_EP
    KEYBOARD_IN_EPNUM = NEXT_EPNUM,
#else
#    define KEYBOARD_IN_EPNUM SHARED_IN_EPNUM
#endif

#if defined(MOUSE_ENABLE) && !defined(MOUSE_SHARED_EP)
    MOUSE_IN_EPNUM = NEXT_EPNUM,
#else
#    define MOUSE_IN_EPNUM SHARED_IN_EPNUM
#endif

#ifdef RAW_ENABLE
    RAW_IN_EPNUM = NEXT_EPNUM,
#    ifdef USB_ENDPOINTS_ARE_REORDERABLE
#        define RAW_OUT_EPNUM RAW_IN_EPNUM
#    else
    RAW_OUT_EPNUM         = NEXT_EPNUM,
#    endif
#endif

#ifdef SHARED_EP_ENABLE
    SHARED_IN_EPNUM = NEXT_EPNUM,
#endif

#ifdef CONSOLE_ENABLE
    CONSOLE_IN_EPNUM = NEXT_EPNUM,

#    ifdef PROTOCOL_CHIBIOS
// ChibiOS has enough memory and descriptor to actually enable the endpoint
// It could use the same endpoint numbers, as that's supported by ChibiOS
// But the QMK code currently assumes that the endpoint numbers are different
#        ifdef USB_ENDPOINTS_ARE_REORDERABLE
#            define CONSOLE_OUT_EPNUM CONSOLE_IN_EPNUM
#        else
    CONSOLE_OUT_EPNUM = NEXT_EPNUM,
#        endif
#    else
#        define CONSOLE_OUT_EPNUM CONSOLE_IN_EPNUM
#    endif
#endif

#ifdef MIDI_ENABLE
    MIDI_STREAM_IN_EPNUM = NEXT_EPNUM,
#    ifdef USB_ENDPOINTS_ARE_REORDERABLE
#        define MIDI_STREAM_OUT_EPNUM MIDI_STREAM_IN_EPNUM
#    else
    MIDI_STREAM_OUT_EPNUM = NEXT_EPNUM,
#    endif
#endif

#ifdef VIRTSER_ENABLE
    CDC_NOTIFICATION_EPNUM = NEXT_EPNUM,
    CDC_IN_EPNUM           = NEXT_EPNUM,
#    ifdef USB_ENDPOINTS_ARE_REORDERABLE
#        define CDC_OUT_EPNUM CDC_IN_EPNUM
#    else
    CDC_OUT_EPNUM         = NEXT_EPNUM,
#    endif
#endif

#ifdef JOYSTICK_ENABLE
#    if !defined(JOYSTICK_SHARED_EP)
    JOYSTICK_IN_EPNUM = NEXT_EPNUM,
#    else
#        define JOYSTICK_IN_EPNUM SHARED_IN_EPNUM
#    endif
#endif

#ifdef DIGITIZER_ENABLE
#    if !defined(DIGITIZER_SHARED_EP)
    DIGITIZER_IN_EPNUM = NEXT_EPNUM,
#    else
#        define DIGITIZER_IN_EPNUM SHARED_IN_EPNUM
#    endif
#endif
};

#else //CH555


#    define KEYBOARD_IN_EPNUM   1

#if defined(MOUSE_ENABLE)
#    define MOUSE_IN_EPNUM SHARED_IN_EPNUM
#endif

#ifdef RAW_ENABLE
#define    RAW_IN_EPNUM 5
#define    RAW_OUT_EPNUM 4
#endif

#ifdef SHARED_EP_ENABLE
#define    SHARED_IN_EPNUM 2
#endif

// TODO put console, midi and cdc endpoints to Device1 of ch555
#ifdef CONSOLE_ENABLE
#   define CONSOLE_IN_EPNUM 6
#   define CONSOLE_OUT_EPNUM 3
#endif

//    MIDI_STREAM_IN_EPNUM ,
//#        define MIDI_STREAM_OUT_EPNUM MIDI_STREAM_IN_EPNUM
//    MIDI_STREAM_OUT_EPNUM ,

//    CDC_NOTIFICATION_EPNUM ,
//    CDC_IN_EPNUM           ,
//#        define CDC_OUT_EPNUM CDC_IN_EPNUM
//    CDC_OUT_EPNUM         ,

#ifdef JOYSTICK_ENABLE
//    JOYSTICK_IN_EPNUM ,
#        define JOYSTICK_IN_EPNUM SHARED_IN_EPNUM
#endif

#ifdef DIGITIZER_ENABLE
//    DIGITIZER_IN_EPNUM ,
#        define DIGITIZER_IN_EPNUM SHARED_IN_EPNUM
#endif
#endif //CH555

#ifdef PROTOCOL_LUFA
// LUFA tells us total endpoints including control
#    define MAX_ENDPOINTS (ENDPOINT_TOTAL_ENDPOINTS - 1)
#elif defined(PROTOCOL_CHIBIOS)
// ChibiOS gives us number of available user endpoints, not control
#    define MAX_ENDPOINTS USB_MAX_ENDPOINTS
#elif defined(PROTOCOL_CH554)
#    define MAX_ENDPOINTS 3
#elif defined(PROTOCOL_CH555)
#    define MAX_ENDPOINTS 6
#endif

// TODO - ARM_ATSAM

#if (NEXT_EPNUM - 1) > MAX_ENDPOINTS
#    error There are not enough available endpoints to support all functions. Please disable one or more of the following: Mouse Keys, Extra Keys, Console, NKRO, MIDI, Serial, Steno
#endif

#define KEYBOARD_EPSIZE 8
#define SHARED_EPSIZE 32
#define MOUSE_EPSIZE 8
#define RAW_EPSIZE 32
#define CONSOLE_EPSIZE 32
#define MIDI_STREAM_EPSIZE 64
#define CDC_NOTIFICATION_EPSIZE 8
#define CDC_EPSIZE 16
#define JOYSTICK_EPSIZE 8
#define DIGITIZER_EPSIZE 8


#ifndef KEYBOARD_SHARED_EP
extern const uint8_t KeyboardReport_size;
extern const USB_Descriptor_HIDReport_Datatype_t PROGMEM KeyboardReport[];
#endif
#if defined(MOUSE_ENABLE) && !defined(MOUSE_SHARED_EP)
extern const USB_Descriptor_HIDReport_Datatype_t PROGMEM MouseReport[];
extern const uint8_t MouseReport_size;
#endif
#if defined(JOYSTICK_ENABLE) && !defined(JOYSTICK_SHARED_EP)
extern const USB_Descriptor_HIDReport_Datatype_t PROGMEM JoystickReport[];
extern const uint8_t JoystickReport_size;
#endif
#if defined(DIGITIZER_ENABLE) && !defined(DIGITIZER_SHARED_EP)
extern const USB_Descriptor_HIDReport_Datatype_t PROGMEM DigitizerReport[];
extern const uint8_t DigitizerReport_size;
#endif
#ifdef SHARED_EP_ENABLE
extern const uint8_t SharedReport_size;
extern const USB_Descriptor_HIDReport_Datatype_t PROGMEM SharedReport[];
#endif
#ifdef RAW_ENABLE
extern const uint8_t RawReport_size;
extern const USB_Descriptor_HIDReport_Datatype_t PROGMEM RawReport[];
#endif

#ifdef CONSOLE_ENABLE
extern const USB_Descriptor_HIDReport_Datatype_t PROGMEM ConsoleReport[];
extern const uint8_t ConsoleReport_size;
#endif

extern const USB_Descriptor_Device_t PROGMEM DeviceDescriptor;
extern const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor;

#ifdef SHARED_EP_ENABLE
extern const USB_Descriptor_Configuration_D1_t PROGMEM ConfigurationDescriptor_D1;
#endif

#if defined(RAW_ENABLE) || defined(CONSOLE_ENABLE)
extern const USB_Descriptor_Configuration_D2_t PROGMEM ConfigurationDescriptor_D2;

/* D2 interface/endpoint numbering — shared between usb_descriptor.c and usb_device.c */
#  ifdef RAW_ENABLE
#    define D2_RAW_INTERFACE_NUM  0
#    define D2_RAW_EPNUM          1  /* EP1 IN  (device→host) */
#    define D2_RAW_OUT_EPNUM      2  /* EP2 OUT (host→device) */
#    ifdef CONSOLE_ENABLE
#      define D2_CONSOLE_INTERFACE_NUM 1
#      define D2_CONSOLE_EPNUM         3
#      define D2_TOTAL_INTERFACES      2
#    else
#      define D2_TOTAL_INTERFACES      1
#    endif
#  else /* CONSOLE_ENABLE only */
#    define D2_CONSOLE_INTERFACE_NUM 0
#    define D2_CONSOLE_EPNUM         1
#    define D2_TOTAL_INTERFACES      1
#  endif
#endif

#ifdef __SDCC
/*
 * String descriptor with 16-bit characters. LUFA's USB_Descriptor_String_t has a wchar_t array when ARCH is
 * ARCH_AVR8, which LUFA sets when ARCH is not defined, and SDCC's wchar_t is 32 bits. Initialise it with
 * USBSTR(), which gives a u"" literal under SDCC.
 */
typedef struct {
    USB_Descriptor_Header_t Header;
    uint16_t                UnicodeString[];
} usb_string_descriptor_t;
#else
typedef USB_Descriptor_String_t usb_string_descriptor_t;
#endif

extern const usb_string_descriptor_t PROGMEM LanguageString;
extern const usb_string_descriptor_t PROGMEM ManufacturerString;
extern const usb_string_descriptor_t PROGMEM ProductString;
#if defined(SERIAL_NUMBER)
extern const usb_string_descriptor_t PROGMEM SerialNumberString;
#endif

extern const USB_Descriptor_Device_t PROGMEM HubDeviceDescriptor;
extern const USB_Descriptor_Hub_Configuration_t PROGMEM HubConfigurationDescriptor;
extern const USB_Descriptor_Hub_t PROGMEM HubDescriptor;

uint16_t get_usb_descriptor(const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, const void** const DescriptorAddress);
