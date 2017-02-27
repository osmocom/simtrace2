# ----------------------------------------------------------------------------
#         ATMEL Microcontroller Software Support 
# ----------------------------------------------------------------------------
# Copyright (c) 2010, Atmel Corporation
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# - Redistributions of source code must retain the above copyright notice,
# this list of conditions and the disclaimer below.
#
# Atmel's name may not be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
# DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# ----------------------------------------------------------------------------

#   Makefile for compiling the Getting Started with SAM3S Microcontrollers project

#-------------------------------------------------------------------------------
#        User-modifiable options
#-------------------------------------------------------------------------------

# Chip & board used for compilation
# (can be overriden by adding CHIP=chip and BOARD=board to the command-line)
CHIP  ?= sam3s4
BOARD ?= qmod

# Defines which are the available memory targets for the SAM3S-EK board.
MEMORIES ?= flash dfu

# Output file basename
APP ?= dfu

# Output directories
OUTPUT = $(BOARD)-$(APP)
BIN = bin
OBJ = obj/$(BOARD)

#-------------------------------------------------------------------------------
#		Tools
#-------------------------------------------------------------------------------

AT91LIB = ./atmel_softpack_libraries

AT91LIB_USB_COMMON_CORE_PATH = $(AT91LIB)/usb/common/core
AT91LIB_USB_CORE_PATH = $(AT91LIB)/usb/device/core
AT91LIB_USB_DFU_PATH = $(AT91LIB)/usb/device/dfu

# Tool suffix when cross-compiling
CROSS_COMPILE = arm-none-eabi-

LIBS = -Wl,--start-group -lgcc -Wl,--end-group -nostdlib

# Compilation tools
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
SIZE = $(CROSS_COMPILE)size
STRIP = $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy
GDB = $(CROSS_COMPILE)gdb
NM = $(CROSS_COMPILE)nm

TOP=..
GIT_VERSION=$(shell $(TOP)/git-version-gen $(TOP)/.tarvers)

#-------------------------------------------------------------------------------
#		Files
#-------------------------------------------------------------------------------

# Directories where source files can be found

USB_PATHS = $(AT91LIB_USB_CORE_PATH) $(AT91LIB_USB_DFU_PATH) $(AT91LIB_USB_COMMON_CORE_PATH)

VPATH += $(USB_PATHS)
VPATH += $(AT91LIB)/libchip_sam3s/source/ $(AT91LIB)/libchip_sam3s/cmsis
VPATH += libboard/common/source libboard/$(BOARD)/source
VPATH += libcommon/source
VPATH += libosmocore/source src_board src_simtrace
VPATH += apps/$(APP)

# Objects built from C source files
C_OSMOCORE = $(notdir $(wildcard libosmocore/source/*.c))
C_LIBCHIP  = $(notdir $(wildcard $(AT91LIB)/libchip_sam3s/source/*.c) $(wildcard $(AT91LIB)/libchip_sam3s/cmsis/*.c))

C_LIBUSB     = USBDescriptors.c USBRequests.c USBD.c USBDCallbacks.c USBDDriver.c USBDDriverCallbacks.c
C_LIBUSB_RT  = dfu.c dfu_desc.c dfu_runtime.c
C_LIBUSB_DFU = dfu.c dfu_desc.c dfu_driver.c
C_LIBCOMMON  = string.c stdio.c fputs.c req_ctx.c ringbuffer.c

C_BOARD      = $(notdir $(wildcard libboard/common/source/*.c))
C_BOARD	    += $(notdir $(wildcard libboard/$(BOARD)/source/*.c))

C_APPLEVEL = $(notdir $(wildcard apps/$(APP)/*.c))

C_FILES  = $(C_OSMOCORE) $(C_LIBCHIP) $(C_LIBUSB) $(C_LIBCOMMON) $(C_BOARD) $(C_APPLEVEL)

-include apps/$(APP)/Makefile

C_OBJECTS  = $(C_FILES:%.c=%.o)

# Trace level used for compilation
# (can be overriden by adding TRACE_LEVEL=#number to the command-line)
# TRACE_LEVEL_DEBUG      5
# TRACE_LEVEL_INFO       4
# TRACE_LEVEL_WARNING    3
# TRACE_LEVEL_ERROR      2
# TRACE_LEVEL_FATAL      1
# TRACE_LEVEL_NO_TRACE   0
TRACE_LEVEL ?= 4

DEBUG_PHONE_SNIFF?=0

#CFLAGS+=-DUSB_NO_DEBUG=1

# Optimization level, put in comment for debugging
OPTIMIZATION ?= -O0



# Flags
INCLUDES_USB = -I$(AT91LIB)/usb/include -I$(AT91LIB)

INCLUDES = $(INCLUDES_USB)
INCLUDES += -I$(AT91LIB)/libchip_sam3s -I$(AT91LIB)/libchip_sam3s/include
INCLUDES += -I$(AT91LIB)/libchip_sam3s/cmsis
INCLUDES += -Ilibboard/common/include -Ilibboard/$(BOARD)/include
INCLUDES += -Ilibcommon/include
INCLUDES += -Ilibosmocore/include
INCLUDES += -Iinclude_board
INCLUDES += -Isrc_simtrace -Iinclude

CFLAGS += -Wall -Wchar-subscripts -Wcomment -Wimplicit-int #-Wformat=2
CFLAGS += -Werror-implicit-function-declaration -Wmain -Wparentheses
CFLAGS += -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs #-Wunused
CFLAGS += -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef
CFLAGS += -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings
CFLAGS += -Wsign-compare -Waggregate-return
CFLAGS += -Wformat=0
CFLAGS += -Wmissing-format-attribute -Wno-deprecated-declarations
CFLAGS += #-Wpacked
CFLAGS += -Wredundant-decls -Wnested-externs -Winline #-Wlong-long
CFLAGS += -Wunreachable-code
#CFLAGS += -Wcast-align
#CFLAGS += -std=c11
CFLAGS += -Wmissing-noreturn
#CFLAGS += -Wconversion
CFLAGS += -Wno-unused-but-set-variable -Wno-unused-variable
CFLAGS += -Wno-suggest-attribute=noreturn

# -mlong-calls  -Wall
#CFLAGS += -save-temps -fverbose-asm
#CFLAGS += -Wa,-a,-ad
CFLAGS += -D__ARM
CFLAGS += --param max-inline-insns-single=500 -mcpu=cortex-m3 -mthumb # -mfix-cortex-m3-ldrd
CFLAGS += -ffunction-sections -g $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) -DTRACE_LEVEL=$(TRACE_LEVEL) -DDEBUG_PHONE_SNIFF=$(DEBUG_PHONE_SNIFF)
CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\" -DBOARD=\"$(BOARD)\" -DAPPLICATION=\"$(APP)\"
ASFLAGS = -mcpu=cortex-m3 -mthumb -Wall -g $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) -D__ASSEMBLY__
LDFLAGS = -mcpu=cortex-m3 -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--entry=ResetException -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align -Wl,--warn-unresolved-symbols $(LIB)
#LD_OPTIONAL=-Wl,--print-gc-sections -Wl,--stats


# Append OBJ and BIN directories to output filename
OUTPUT := $(BIN)/$(OUTPUT)

#-------------------------------------------------------------------------------
#		Rules
#-------------------------------------------------------------------------------

all: $(BIN) $(OBJ) $(MEMORIES)

$(BIN) $(OBJ):
	mkdir $@

define RULES
C_OBJECTS_$(1) = $(addprefix $(OBJ)/$(1)_, $(C_OBJECTS))
ASM_OBJECTS_$(1) = $(addprefix $(OBJ)/$(1)_, $(ASM_OBJECTS))

$(1): $$(ASM_OBJECTS_$(1)) $$(C_OBJECTS_$(1))
	@$(CC) $(LDFLAGS) $(LD_OPTIONAL) -T"libboard/common/resources/$(CHIP)/$$@.ld" -Wl,-Map,$(OUTPUT)-$$@.map -o $(OUTPUT)-$$@.elf $$^ $(LIBS)
	@$(NM) $(OUTPUT)-$$@.elf >$(OUTPUT)-$$@.elf.txt
	@$(OBJCOPY) -O binary $(OUTPUT)-$$@.elf $(OUTPUT)-$$@.bin
	@$(SIZE) $$^ $(OUTPUT)-$$@.elf

$$(C_OBJECTS_$(1)): $(OBJ)/$(1)_%.o: %.c Makefile $(OBJ) $(BIN)
	@echo [COMPILING $$<]
	@$(CC) $(CFLAGS) -D$(1) -DENVIRONMENT=\"$(1)\" -Wa,-ahlms=$(BIN)/$$*.lst -c -o $$@ $$< 

$$(ASM_OBJECTS_$(1)): $(OBJ)/$(1)_%.o: %.S Makefile $(OBJ) $(BIN)
	@echo [ASSEMBLING $$@]
	@$(CC) $(ASFLAGS) -D$(1) -DENVIRONMENT=\"$(1)\" -c -o $$@ $$<

debug_$(1): $(1)
	$(GDB) -x "$(BOARD_LIB)/resources/gcc/$(BOARD)_$(1).gdb" -ex "reset" -readnow -se $(OUTPUT)-$(1).elf
endef

$(foreach MEMORY, $(MEMORIES), $(eval $(call RULES,$(MEMORY))))

program:
	openocd -f openocd/openocd.cfg -c "init" -c "halt" -c "flash write_bank 0 ./bin/project-flash.bin 0" -c "reset" -c "shutdown"

SERIAL ?= /dev/ttyUSB0
log:
	stty -F $(SERIAL) 115200
	lsof $(SERIAL) && echo "log is already opened" || ( sed -u "s/\r//" $(SERIAL) | ts )

clean:
	-rm -fR $(OBJ)/*.o $(BIN)/*.bin $(BIN)/*.elf $(BIN)/*.elf.txt $(BIN)/*.map $(BIN)/*.lst
