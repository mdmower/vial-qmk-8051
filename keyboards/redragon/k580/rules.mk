ENCODER_ENABLE = yes

# The CH555 LED controller drives the matrix columns (its COM pins), so its interrupt
# (led_dma.c) reads the key rows and matrix.c takes them from there.
CUSTOM_MATRIX = lite
SRC += led_dma.c matrix.c
OPT_DEFS += -DCH555_LED_DMA_ENABLE
ENCODER_MAP_ENABLE = yes




CAPS_WORD_ENABLE = no
#GRAVE_ESC_ENABLE = no
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





EEPROM_DRIVER = transient
#EEPROM_DRIVER = wear_leveling


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
