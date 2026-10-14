# Target architecture 
ARCH = MCS51
IRAM_SIZE  = 0x0100 #TODO upper 128 bytes only accessed through @Ri

ifneq (,$(filter $(MCU),ch552 ch554))

  # Microcontroller Settings
  XRAM_LOC   = 0x0100 #TODO pRAM is preserved for xSFR or smth, not sure
  XRAM_SIZE  = 0x0300
  CODE_SIZE  = 0x3800
  
  F_CPU      = 12000000
 #F_USB = 48000000 

  PROTOCOL = CH554

endif

ifneq (,$(filter $(MCU),ch555))

  # Microcontroller Settings
  # xRAM is 0x0000-0x1FFF, but the USB buffers are fixed at 0x1000-0x13FF (pUSB_BUF_DEV0 ..
  # pUSB_BUF_HUB in lib/ch555/ch555.h), so the linker only gets 0x0100-0x0FFF.
  # Below 0x0100: with EVISION_OEM_BOOTLOADER, each interrupt stub of that bootloader stores
  # 3 bytes in xdata between 0x0042 and 0x00FC. The xSFRs are at 0x2100-0x22FF, not here.
  XRAM_LOC   = 0x0100
  XRAM_SIZE  = 0x0f00
  CODE_SIZE  = 0xf000

  # Fsys; a keyboard's rules.mk may set it (CfgFsys() in lib/ch555/hal.c: 12, 16, 24 or 32 MHz)
  F_CPU      ?= 24000000
 #F_USB = 48000000 

  PROTOCOL = CH555

endif
