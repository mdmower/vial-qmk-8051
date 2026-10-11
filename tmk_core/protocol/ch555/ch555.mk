CH555_DIR = protocol/ch555

# Path to the CH555 library
CH555_PATH = $(LIB_PATH)/ch555


CH555_SRC_USB  = $(CH555_PATH)/gpio.c
ifeq ($(strip $(RGB_MATRIX_ENABLE)), yes)
CH555_SRC_USB += $(CH555_PATH)/rgb_matrix_ch555.c
endif
CH555_SRC_USB += $(CH555_PATH)/flash.c
CH555_SRC_USB += $(CH555_PATH)/hal.c
#CH555_SRC_USB += $(CH555_DIR)/usb_handler.c
CH555_SRC_USB += $(CH555_DIR)/usb_device.c
#CH555_SRC_USB += $(CH555_DIR)/usb_desc.c

CH555_SRC = ch555_usb_stack.c \
	usb_descriptor.c \
	$(CH555_SRC_USB)

ifeq ($(strip $(MIDI_ENABLE)), yes)
	CH555_SRC += $(CH555_ROOT_PATH)/Drivers/USB/Class/Device/MIDIClassDevice.c
endif

ifeq ($(strip $(VIRTSER_ENABLE)), yes)
	CH555_SRC += $(CH555_ROOT_PATH)/Drivers/USB/Class/Device/CDCClassDevice.c
endif

SRC += $(CH555_SRC)
SRC += $(CH555_DIR)/usb_util.c

# Search Path
VPATH += $(TMK_PATH)/$(CH555_DIR)
VPATH += $(CH555_PATH)
VPATH += $(TMK_PATH)/$(CH555_DIR)/lufa_utils

# CH555 library compile-time options and predefined tokens
#CH555_OPTS  = -DUSB_DEVICE_ONLY
#CH555_OPTS += -DUSE_FLASH_DESCRIPTORS
#CH555_OPTS += -DUSE_STATIC_OPTIONS="(USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)"
CH555_OPTS += -DFIXED_CONTROL_ENDPOINT_SIZE=64
CH555_OPTS += -DFIXED_NUM_CONFIGURATIONS=1

OPT_DEFS += -DF_USB=$(F_USB)UL
OPT_DEFS += -DARCH=ARCH_$(ARCH)
OPT_DEFS += $(CH555_OPTS)

# This indicates using CH555 stack
OPT_DEFS += -DPROTOCOL_CH555
