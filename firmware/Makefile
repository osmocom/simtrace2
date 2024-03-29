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

GIT_VERSION=$(shell $(TOP)/git-version-gen $(TOP)/.tarball-version)

CFLAGS = \
	 -Werror=format-security \
	 -Wformat \
	 -g \
	 $(NULL)

#-------------------------------------------------------------------------------
#        User-modifiable options
#-------------------------------------------------------------------------------

# verbosity
V ?= 0
ifneq ("$(V)","0")
SILENT :=
else
SILENT := @
endif

# Chip & board used for compilation
# (can be overriden by adding CHIP=chip and BOARD=board to the command-line)
CHIP  ?= sam3s4
BOARD ?= qmod
APP ?= dfu

# Defines which are the available memory targets for the SAM3S-EK board.
ifeq ($(APP), dfu)
MEMORIES ?= flash
TRACE_LEVEL ?= 0
else
MEMORIES ?= dfu
endif

# Output directories and filename
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

USE_CLANG ?= 0
ifneq ("$(USE_CLANG)","0")
# --target=thumbv7m-none-eabi
CC = clang
LD = ld.lld
SIZE = llvm-size
LIBS = -nostdlib
else
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
SIZE = $(CROSS_COMPILE)size
LIBS = -Wl,--start-group -lgcc -Wl,--end-group -nostdlib
endif

STRIP = $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy
GDB = $(CROSS_COMPILE)gdb
NM = $(CROSS_COMPILE)nm

TOP=..

#-------------------------------------------------------------------------------
#		Files
#-------------------------------------------------------------------------------

# Directories where source files can be found

USB_PATHS = $(AT91LIB_USB_CORE_PATH) $(AT91LIB_USB_DFU_PATH) $(AT91LIB_USB_COMMON_CORE_PATH)

VPATH += $(USB_PATHS)
VPATH += $(AT91LIB)/libchip_sam3s/source/ $(AT91LIB)/libchip_sam3s/cmsis
VPATH += libboard/common/source libboard/$(BOARD)/source
VPATH += libcommon/source
VPATH += libosmocore/source 
VPATH += apps/$(APP)

# Objects built from C source files
C_OSMOCORE = $(notdir $(wildcard libosmocore/source/*.c))
C_LIBCHIP  = $(notdir $(wildcard $(AT91LIB)/libchip_sam3s/source/*.c) $(wildcard $(AT91LIB)/libchip_sam3s/cmsis/*.c))

C_LIBUSB     = USBDescriptors.c USBRequests.c USBD.c USBDCallbacks.c USBDDriver.c USBDDriverCallbacks.c
C_LIBUSB_RT  = dfu.c dfu_runtime.c
C_LIBUSB_DFU = dfu.c dfu_desc.c dfu_driver.c
C_LIBCOMMON  = string.c stdio.c fputs.c usb_buf.c ringbuffer.c pseudo_talloc.c host_communication.c \
	       main_common.c stack_check.c crcstub.c

C_BOARD      = $(notdir $(wildcard libboard/common/source/*.c))
C_BOARD     += $(notdir $(wildcard libboard/$(BOARD)/source/*.c))

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

# allow asserting the peer SAM3S ERASE signal to completely erase the flash
# only applicable for qmod board
ALLOW_PEER_ERASE?=0

#CFLAGS+=-DUSB_NO_DEBUG=1

# Optimization level, put in comment for debugging
ifneq ("$(USE_CLANG)","0")
OPTIMIZATION ?= -Oz
else
OPTIMIZATION ?= -Os
endif

# Flags
INCLUDES_USB = -I$(AT91LIB)/usb/include -I$(AT91LIB)

INCLUDES = $(INCLUDES_USB)
INCLUDES += -I$(AT91LIB)/libchip_sam3s -I$(AT91LIB)/libchip_sam3s/include
INCLUDES += -I$(AT91LIB)/libchip_sam3s/cmsis
INCLUDES += -Ilibboard/common/include -Ilibboard/$(BOARD)/include
INCLUDES += -Ilibcommon/include
INCLUDES += -Ilibosmocore/include
INCLUDES += -Isrc_simtrace -Iinclude
INCLUDES += -Iapps/$(APP)

ifneq ("$(USE_CLANG)","0")
CFLAGS += -Wno-unknown-warning-option -Wno-empty-body -fno-exceptions -fno-unwind-tables --config armv7em_soft_nofp_nosys
else
CFLAGS += -Wno-suggest-attribute=noreturn --param max-inline-insns-single=500
endif
CFLAGS += -Wall -Wchar-subscripts -Wcomment -Wimplicit-int -Wformat=2
CFLAGS += -Werror-implicit-function-declaration -Wmain -Wparentheses
CFLAGS += -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs #-Wunused
CFLAGS += -Wuninitialized -Wunknown-pragmas -Wfloat-equal #-Wundef
CFLAGS += -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings
CFLAGS += -Waggregate-return #-Wsign-compare
CFLAGS += -Wmissing-format-attribute -Wno-deprecated-declarations
CFLAGS += #-Wpacked
CFLAGS += -Wredundant-decls -Wnested-externs #-Winline -Wlong-long
CFLAGS += -Wunreachable-code
#CFLAGS += -Wcast-align
#CFLAGS += -std=c11
#CFLAGS += -Wmissing-noreturn
#CFLAGS += -Wconversion
CFLAGS += -Wno-unused-but-set-variable -Wno-unused-variable


# -mlong-calls  -Wall
#CFLAGS += -save-temps -fverbose-asm
#CFLAGS += -Wa,-a,-ad
CFLAGS += -D__ARM -fno-builtin
CFLAGS += -mcpu=cortex-m3 -mthumb # -mfix-cortex-m3-ldrd
CFLAGS += -ffunction-sections -g $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) -DTRACE_LEVEL=$(TRACE_LEVEL) -DALLOW_PEER_ERASE=$(ALLOW_PEER_ERASE)
CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
CFLAGS += -DBOARD=\"$(BOARD)\" -DBOARD_$(BOARD)
CFLAGS += -DAPPLICATION=\"$(APP)\" -DAPPLICATION_$(APP)

# Disable stack protector by default (OS#5081)
ifeq ($(STACK_PROTECTOR), 1)
CFLAGS += -fstack-protector
else
CFLAGS += -fno-stack-protector
endif

ASFLAGS = -mcpu=cortex-m3 -mthumb -Wall -g $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) -D__ASSEMBLY__
LDFLAGS = -mcpu=cortex-m3 -mthumb -Wl,--check-sections -Wl,--gc-sections -Wl,--entry=ResetException -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--no-undefined $(LIB)
ifeq ("$(USE_CLANG)","0")
LDFLAGS += -Wl,--warn-section-align -Wl,--print-memory-usage
endif
#LD_OPTIONAL=-Wl,--print-gc-sections -Wl,--stats

# Append BIN directories to output filename
OUTPUT := $(BIN)/$(OUTPUT)

#-------------------------------------------------------------------------------
#		Rules
#-------------------------------------------------------------------------------

all: $(BIN) $(OBJ) $(MEMORIES)

combined: $(OUTPUT)-combined.bin

$(BIN)/$(BOARD)-dfu-flash-padded.bin: $(BIN)/$(BOARD)-dfu-flash.bin
	dd if=/dev/zero bs=16384 count=1 of=$@
	dd if=$< conv=notrunc of=$@

$(OUTPUT)-combined.bin: $(BIN)/$(BOARD)-dfu-flash-padded.bin $(OUTPUT)-dfu.bin
	cat $^ > $@

$(BIN) $(OBJ): apps/$(APP)/usb_strings_generated.h misc/crctool
	mkdir -p $@

usbstring/usbstring: usbstring/usbstring.c
	gcc $^ -o $@

misc/crctool: misc/crctool.c
	gcc $^ -o $@

.PHONY: apps/$(APP)/usb_strings.txt.patched
apps/$(APP)/usb_strings.txt.patched: apps/$(APP)/usb_strings.txt
	sed "s/PRODUCT_STRING/$(shell cat libboard/$(BOARD)/product_string.txt)/" $< > $@

apps/$(APP)/usb_strings_generated.h: apps/$(APP)/usb_strings.txt.patched usbstring/usbstring
	cat $< | usbstring/usbstring > $@

define RULES
C_OBJECTS_$(1) = $(addprefix $(OBJ)/$(1)_, $(C_OBJECTS))
ASM_OBJECTS_$(1) = $(addprefix $(OBJ)/$(1)_, $(ASM_OBJECTS))
EXTRA_OBJECTS_$(1) = $(addprefix $(OBJ)/$(1)_, $(EXTRA_OBJECTS))

build_$(1): $$(ASM_OBJECTS_$(1)) $$(C_OBJECTS_$(1)) $$(EXTRA_OBJECTS_$(1))
	$(SILENT)$(CC) $(CFLAGS) $(LDFLAGS) $(LD_OPTIONAL) -T"libboard/common/resources/$(CHIP)/$(1).ld" -Wl,-Map,$(OUTPUT)-$(1).map -o $(OUTPUT)-$(1).elf $$^ $(LIBS)
	$(SILENT)$(NM) $(OUTPUT)-$(1).elf >$(OUTPUT)-$(1).elf.txt
	$(SILENT)$(OBJCOPY) -O binary $(OUTPUT)-$(1).elf $(OUTPUT)-$(1).bin

$$(C_OBJECTS_$(1)): $(OBJ)/$(1)_%.o: %.c Makefile $(OBJ) $(BIN)
	@echo [COMPILING $$<]
	$(SILENT)$(CC) $(CFLAGS) -DENVIRONMENT_$(1) -DENVIRONMENT=\"$(1)\" -c -o $$@ $$<

$$(ASM_OBJECTS_$(1)): $(OBJ)/$(1)_%.o: %.S Makefile $(OBJ) $(BIN)
	@echo [ASSEMBLING $$@]
	$(SILENT)$(CC) $(ASFLAGS) -DENVIRONMENT_$(1) -DENVIRONMENT=\"$(1)\" -c -o $$@ $$<

endef

ALL_MEMORIES = flash ram
$(foreach MEMORY, $(ALL_MEMORIES), $(eval $(call RULES,$(MEMORY))))

# files with those names do exist..
.PHONY: dfu
dfu: $(OUTPUT)-dfu.bin
.PHONY: ram
ram: build_ram
.PHONY: flash
flash: build_flash
#alternate way of embedding: obj file
#ifeq ($(APP), dfu)
#	$(info bootloader bin file to obj..)
#	$(SILENT)$(OBJCOPY) --rename-section .data=.fwupdate -I binary -O elf32-littlearm bin/$(BOARD)-dfu-flash.bin $(OBJ)/flash_fwupdate.o
#endif

C_OBJECTS_dfu = $(addprefix $(OBJ)/dfu_, $(C_OBJECTS))
ASM_OBJECTS_dfu = $(addprefix $(OBJ)/dfu_, $(ASM_OBJECTS))
EXTRA_OBJECTS_dfu = $(addprefix $(OBJ)/dfu_, $(EXTRA_OBJECTS))

$(OUTPUT)-dfu.bin: $(OUTPUT)-dfu_nocrcstub.bin
	$(info updating app with crc..)
	$(SILENT)cp $< $@.temp
	$(SILENT)misc/crctool 512 $@.temp
	$(SILENT)mv $@.temp $@

$(OUTPUT)-dfu_nocrcstub.bin: $(OUTPUT)-dfu_nocrcstub.elf
ifeq ($(APP), blupdate)
	$(info updating updater section with padded bootloader file..)
	$(SILENT)dd status=none if=/dev/zero bs=16384 count=1 of=$(BIN)/$(BOARD)-dfu-flash-padded.bin
	$(SILENT)dd status=none if=$(BIN)/$(BOARD)-dfu-flash.bin conv=notrunc of=$(BIN)/$(BOARD)-dfu-flash-padded.bin
	$(SILENT)$(OBJCOPY) --update-section .blupdate=bin/$(BOARD)-dfu-flash-padded.bin $<
endif
	$(SILENT)$(OBJCOPY) -O binary $< $@

$(OUTPUT)-dfu_nocrcstub.elf: $(ASM_OBJECTS_dfu) $(C_OBJECTS_dfu) $(EXTRA_OBJECTS_dfu)
	$(SILENT)$(CC) $(CFLAGS) $(LDFLAGS) $(LD_OPTIONAL) -T"libboard/common/resources/$(CHIP)/dfu.ld" -Wl,-Map,$(OUTPUT)-dfu_nocrcstub.map -o $@ $^ $(LIBS)
	$(SILENT)$(NM) $@ >$@.txt

$(C_OBJECTS_dfu): $(OBJ)/dfu_%.o: %.c Makefile $(OBJ) $(BIN)
	@echo [COMPILING $<]
	$(SILENT)$(CC) $(CFLAGS) -DENVIRONMENT_dfu -DENVIRONMENT=\"dfu\" -c -o $@ $<

$(ASM_OBJECTS_dfu): $(OBJ)/dfu_%.o: %.S Makefile $(OBJ) $(BIN)
	@echo [ASSEMBLING $@]
	$(SILENT)$(CC) $(ASFLAGS) -DENVIRONMENT_dfu -DENVIRONMENT=\"dfu\" -c -o$@ $<


SERIAL ?= /dev/ttyUSB0
log:
	stty -F $(SERIAL) 921600
	lsof $(SERIAL) && echo "log is already opened" || ( sed -u "s/\r//" $(SERIAL) | ts )

clean:
	-rm -f apps/$(APP)/usb_strings.txt.patched
	-rm -fR $(OBJ)/*.o $(BIN)/*.bin $(BIN)/*.elf $(BIN)/*.elf.txt $(BIN)/*.map $(BIN)/*.lst `find . -name \*.p`

install:
	mkdir -p $(DESTDIR)/usr/share/simtrace2
	cp $(BIN)/*.bin $(BIN)/*.elf $(BIN)/*.elf.txt $(BIN)/*.map $(DESTDIR)/usr/share/simtrace2
