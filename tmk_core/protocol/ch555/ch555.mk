CH555_DIR = protocol/ch555

# Path to the CH555 library
CH555_PATH = $(LIB_PATH)/ch555


CH555_SRC_USB  = $(CH555_PATH)/gpio.c
# Data flash driver
ifeq ($(strip $(CH555_DATA_FLASH_ENABLE)), yes)
CH555_SRC_USB += $(CH555_PATH)/data_flash.c
endif
CH555_SRC_USB += $(CH555_PATH)/hal.c
CH555_SRC_USB += $(CH555_DIR)/usb_device.c

CH555_SRC = ch555_usb_stack.c \
	usb_descriptor.c \
	$(CH555_SRC_USB)

SRC += $(CH555_SRC)
SRC += $(CH555_DIR)/usb_util.c

# Search Path
VPATH += $(TMK_PATH)/$(CH555_DIR)
VPATH += $(CH555_PATH)
VPATH += $(TMK_PATH)/$(CH555_DIR)/lufa_utils

# CH555 library compile-time options and predefined tokens
CH555_OPTS += -DFIXED_CONTROL_ENDPOINT_SIZE=64
CH555_OPTS += -DFIXED_NUM_CONFIGURATIONS=1

# USB serial number from the chip ID (get_hardware_id()) when SERIAL_NUMBER is not set;
# SERIAL_NUMBER_USE_HARDWARE_ID = no in a keyboard's rules.mk leaves the device without a serial number.
SERIAL_NUMBER_USE_HARDWARE_ID ?= yes
ifeq ($(strip $(SERIAL_NUMBER_USE_HARDWARE_ID)), yes)
    CH555_OPTS += -DSERIAL_NUMBER_USE_HARDWARE_ID
endif

OPT_DEFS += -DARCH=ARCH_$(ARCH)
OPT_DEFS += $(CH555_OPTS)

# This indicates using CH555 stack
OPT_DEFS += -DPROTOCOL_CH555
