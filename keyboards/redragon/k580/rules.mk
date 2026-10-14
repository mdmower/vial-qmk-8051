ENCODER_ENABLE = yes

# The CH555 LED controller drives the matrix columns (its COM pins), so its interrupt
# (led_dma.c) reads the key rows and matrix.c takes them from there.
CUSTOM_MATRIX = lite
SRC += led_dma.c led_fx.c matrix.c
OPT_DEFS += -DCH555_LED_DMA_ENABLE
# Fsys 32 MHz, as stock 1.04 (CH555 default here: 24 MHz). The LED frame, and with it the key
# sampling period, is 304128 clocks: 9.5 ms at 32 MHz, 12.7 ms at 24 MHz.
F_CPU = 32000000
# Lighting settings saved in the data flash (flash_store.c)
SRC += flash_store.c
CH555_DATA_FLASH_ENABLE = yes
ENCODER_MAP_ENABLE = yes




CAPS_WORD_ENABLE = no
GRAVE_ESC_ENABLE = no # flash space: QK_GESC does nothing
MAGIC_ENABLE = no  #swap thingy
COMMAND_ENABLE = no #the old magic
DYNAMIC_MACRO_ENABLE = no
LEADER_ENABLE = no
SPACE_CADET_ENABLE = no # turn MOD key to other key when tap

KEY_OVERRIDE_ENABLE = no
COMBO_ENABLE = no #press multiple key at same time to produce other key
TAP_DANCE_ENABLE = no #rapid press a key to produce other key

BOOTMAGIC_ENABLE = no # hold Esc at power-on to jump to bootloader
QMK_SETTINGS = no





# No EEPROM (eeprom_none.c), for flash space: QMK's eeconfig reads zeros and its writes are dropped.
# Nothing here reads eeconfig back (no bootmagic, no magic keycodes); a feature that keeps its
# settings in eeconfig would get zeros. The transient driver held eeconfig in RAM only.
EEPROM_DRIVER = custom
SRC += eeprom_none.c
# Here, not in config.h: drivers/eeprom/eeprom_driver.c does not include config.h.
OPT_DEFS += -DEEPROM_SIZE=127


EXTRAKEY_ENABLE = yes
NKRO_ENABLE = yes
MOUSEKEY_ENABLE = no
RAW_ENABLE = yes

#JOYSTICK_ENABLE = no

#DIGITIZER_ENABLE = no #absolute mouse
#CONSOLE_ENABLE = no

# Evision OEM bootloader (0c45:7687), which the K580 ships with: QK_BOOT erases its boot marker page
# (0x27C0) and resets. Link with --code-loc 0x3027, then make the flash image with
# util/evision_oem_image.py.
EVISION_OEM_BOOTLOADER ?= no
ifeq ($(strip $(EVISION_OEM_BOOTLOADER)), yes)
    OPT_DEFS += -DEVISION_OEM_BOOTLOADER
endif

# USB serial number of the keyboard devices (the hub reports none), optional:
# qmk compile ... -e SERIAL_NUMBER=<serial>, or SERIAL_NUMBER=<serial> util/k580_build.sh.
# Letters, digits and . _ : - only (the value passes through the shell), at most 126 characters.
ifneq ($(strip $(SERIAL_NUMBER)),)
    OPT_DEFS += -DSERIAL_NUMBER=\"$(strip $(SERIAL_NUMBER))\"
endif

# Disabled for flash space (2026-10-12; image end 0xEB34 before). Remove a line to use the feature again.
# - Tap keys (LT, MT, TT, *_T, tap-toggle) and one-shot keys (OSM, OSL) do nothing; MO() still works.
#   In OPT_DEFS, not config.h: they change keyrecord_t, and some files do not include config.h.
OPT_DEFS += -DNO_ACTION_TAPPING -DNO_ACTION_ONESHOT
# - No send_string() / SEND_STRING() (macros, QK_MAKE).
SEND_STRING_ENABLE = no
# - keycode_config() and mod_config() return their argument: the swap settings of magic keycodes and
#   bootmagic are ignored. Remove before enabling either.
OPT_DEFS += -DKEYCODE_CONFIG_IDENTITY
