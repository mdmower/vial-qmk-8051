#pragma once

#ifdef __SDCC

#define __attribute__(x)
#include "info_config.h" // sdcc dont have option -include
#include "config.h"      // same as above
// expose the USB ISR to main, so it is called in interrupt vector table:
#include "usb_device.h" 
// same for the LED controller ISR of a keyboard that drives the CH555 LED-DMA
#ifdef CH555_LED_DMA_ENABLE
#include "led_dma.h"
#endif

#else

#define __xdata
#define __data

#endif
