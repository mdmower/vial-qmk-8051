# Hey Emacs, this is a -*- makefile -*-
##############################################################################

# Compiler settings
#
CC = sdcc
PACK_HEX   = packihx
OBJCOPY = objcopy
HEX =
EEP =
BIN =

# warning  85: unreferenced function argument
# warning 115: unsupported pragma
# warning 117: unknown compiler option
# warning 190: empty translation unit (a file, probably)
CFLAGS  = -mmcs51 --model-large --disable-warning 115 --disable-warning 85 --disable-warning 117 --disable-warning 190
CFLAGS += --iram-size $(IRAM_SIZE) --xram-size $(XRAM_SIZE) --xram-loc $(XRAM_LOC) --code-size $(CODE_SIZE)
CFLAGS += -DF_CPU=$(F_CPU)
#CFLAGS += --opt-code-size
CFLAGS += --opt-code-speed
#CFLAGS += --xstack --xstack-loc 0x0000 

# use stdin as preprocessed c code:
# preprocess only: -E. compile only: -S. compile & assemble only: -c
#CFLAGS += --c1mode
#CFLAGS += -E
#CFLAGS += --dump-ast --dump-i-code --dump-graphs --i-code-in-asm --fverbose-asm
#CFLAGS += --i-code-in-asm

# List any extra directories to look for libraries here.
#     Each directory must be seperated by a space.
#     Use forward slashes for directory separators.
#     For a directory that has spaces, enclose it in quotes.
EXTRALIBDIRS =

#============================================================================

# Convert hex to bin.
bin: $(BUILD_DIR)/$(TARGET).ihx
	$(OBJCOPY) -Iihex -Obinary $(BUILD_DIR)/$(TARGET).ihx $(BUILD_DIR)/$(TARGET).bin
	$(COPY) $(BUILD_DIR)/$(TARGET).bin $(TARGET).bin;

