ENCODER_ENABLE = no

#CUSTOM_MATRIX = lite  # matrix_scan_raw() in keyboards/redragon/k580/matrix.c reads LED ISR shadow buffer
#RGB_MATRIX_ENABLE = yes
#RGB_MATRIX_DRIVER = custom
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
