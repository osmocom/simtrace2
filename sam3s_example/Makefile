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
CHIP  = sam3s2
BOARD = simtrace

# Defines which are the available memory targets for the SAM3S-EK board.
MEMORIES = flash

# Trace level used for compilation
# (can be overriden by adding TRACE_LEVEL=#number to the command-line)
# TRACE_LEVEL_DEBUG      5
# TRACE_LEVEL_INFO       4
# TRACE_LEVEL_WARNING    3
# TRACE_LEVEL_ERROR      2
# TRACE_LEVEL_FATAL      1
# TRACE_LEVEL_NO_TRACE   0
TRACE_LEVEL = 4

# Optimization level, put in comment for debugging
OPTIMIZATION = -O0

# Output file basename
OUTPUT = project

# Output directories
BIN = bin
OBJ = obj

#-------------------------------------------------------------------------------
#		Tools
#-------------------------------------------------------------------------------

# Tool suffix when cross-compiling
CROSS_COMPILE = arm-none-eabi-

LIBC_PATH=../Baselibc

LIBS = -Wl,--start-group -lgcc -lc -Wl,--end-group
#LIB_PATH+=-L=/lib/thumb2
#LIB_PATH+=-L=/../lib/gcc/arm-none-eabi/4.4.1/thumb2
LIB+=-L=$(LIBC_PATH) -lc

# Compilation tools
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
SIZE = $(CROSS_COMPILE)size
STRIP = $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy
GDB = $(CROSS_COMPILE)gdb
NM = $(CROSS_COMPILE)nm

# Flags
INCLUDES  = -Iinclude -Isam3s_examples_include
INCLUDES += -Icmsis
INCLUDES += -I$(LIBC_PATH)/include
#INCLUDES += -I$(LIBRARIES)

CFLAGS += -Wall -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int
CFLAGS += -Werror-implicit-function-declaration -Wmain -Wparentheses
CFLAGS += -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused
CFLAGS += -Wuninitialized -Wunknown-pragmas -Wfloat-equal -Wundef
CFLAGS += -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings
CFLAGS += -Wsign-compare -Waggregate-return
CFLAGS += -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations
CFLAGS += -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wlong-long
CFLAGS += -Wunreachable-code
CFLAGS += -Wcast-align
#CFLAGS += -Wmissing-noreturn
#CFLAGS += -Wconversion

# To reduce application size use only integer printf function.
CFLAGS += -Dprintf=iprintf

# -mlong-calls  -Wall
#CFLAGS += -save-temps -fverbose-asm
#CFLAGS += -Wa,-a,-ad
CFLAGS += --param max-inline-insns-single=500 -mcpu=cortex-m3 -mthumb # -mfix-cortex-m3-ldrd
CFLAGS += -ffunction-sections -g $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) -DTRACE_LEVEL=$(TRACE_LEVEL)
ASFLAGS = -mcpu=cortex-m3 -mthumb -Wall -g $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) -D__ASSEMBLY__
LDFLAGS = -mcpu=cortex-m3 -mthumb -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--entry=ResetException -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align -Wl,--warn-unresolved-symbols $(LIB)
#LD_OPTIONAL=-Wl,--print-gc-sections -Wl,--stats

#-------------------------------------------------------------------------------
#		Files
#-------------------------------------------------------------------------------

# Directories where source files can be found

VPATH += src_board src_sam3s cmsis 

# Objects built from C source files
C_CMSIS    = core_cm3.o
C_LOWLEVEL = board_cstartup_gnu.o board_lowlevel.o syscalls.o exceptions.o
C_LIBLEVEL = spi.o pio.o pmc.o usart.o
C_APPLEVEL = main.o
C_OBJECTS  = $(C_CMSIS) $(C_LOWLEVEL) $(C_LIBLEVEL) $(C_APPLEVEL)

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
	@$(CC) $(LDFLAGS) $(LD_OPTIONAL) -T"$$@.ld" -Wl,-Map,$(OUTPUT)-$$@.map -o $(OUTPUT)-$$@.elf $$^ $(LIBS)
	$(NM) $(OUTPUT)-$$@.elf >$(OUTPUT)-$$@.elf.txt
	$(OBJCOPY) -O binary $(OUTPUT)-$$@.elf $(OUTPUT)-$$@.bin
	$(SIZE) $$^ $(OUTPUT)-$$@.elf

$$(C_OBJECTS_$(1)): $(OBJ)/$(1)_%.o: %.c Makefile $(OBJ) $(BIN)
	@echo [COMPILING $$@]
	@$(CC) $(CFLAGS) -D$(1) -Wa,-ahlms=$(BIN)/$$*.lst -c -o $$@ $$<  

$$(ASM_OBJECTS_$(1)): $(OBJ)/$(1)_%.o: %.S Makefile $(OBJ) $(BIN)
	@echo [ASSEMBLING $$@]
	@$(CC) $(ASFLAGS) -D$(1) -c -o $$@ $$<

debug_$(1): $(1)
	$(GDB) -x "$(BOARD_LIB)/resources/gcc/$(BOARD)_$(1).gdb" -ex "reset" -readnow -se $(OUTPUT)-$(1).elf
endef

$(foreach MEMORY, $(MEMORIES), $(eval $(call RULES,$(MEMORY))))

program:
	openocd -f /home/chrysh/ba_thesis/thesis/openocd/openocd.cfg -c "init" -c "halt" -c "flash write_bank 0 ./bin/project-flash.bin 0" -c "reset" -c "shutdown"

clean:
	-rm -fR $(OBJ)/*.o $(BIN)/*.bin $(BIN)/*.elf $(BIN)/*.elf.txt $(BIN)/*.map $(BIN)/*.lst

rmbak: 
	-rm *~ $(BIN)/*~
