
== BOARDS

A board defines a given circuit board, i.e. SIMtrace, OWHW, QMOD

It defines the given hardware model for which the program is to be
compiled.

Current boards supported are:
* simtrace: The good old Osmocom SIMtrace PCB with SAM3 instead of
  SAM7, open hardware.
* qmod: A sysmocom-proprietary quad mPCIe carrier board, publicly available
* owhw: An undisclosed sysmocom-internal board, not publicly available

== APPLICATIONS

An application is a specific piece of software with given
functionality.

== ENVIRONMENTS

An environment is a runtime environment, typically defined by a linker
script.  The current runtime environments include
* flash: Run natively from start of flash memory
* dfu: Run after a DFU bootloader from an offset after the first 16k
  of flash (the first 16k are reserved for the bootloader)
* ram: Run from within the RAM of the chip, downloaded via JTAG/SWD


== Building

A given software build is made for a specific combination of an APP
running in a certain ENVIRONMENT on a given BOARD.

A Makefile is provided. It will create output files in the format
bin/$(BOARD)-$(APP)-$(ENV).{elf,bin}

You can specify the APP and BOARD to build when calling make, like
e.g.
* make APP=cardem BOARD=qmod
* make APP=dfu BOARD=qmod

The level of debug messages can be altered at compile time:
```
$ make TRACE_LEVEL=4
```
Accepted values: 0 (NO_TRACE) to 5 (DEBUG)

== Flashing

For flashing the firmware, there are at least two options.

=== Using JTAG + OpenOCD to flash the DFU bootloader

The first one is using openocd and a JTAG key.
For this option, a JTAG connector has to be soldered onto the board, which is not attached per default.

```
$ openocd -f openocd/openocd.cfg -c "init" -c "halt" -c "flash write_bank 0 ./bin/$(BOARD)-dfu-flash.bin 0" -c "reset" -c "shutdown"
```

=== Using bossac to flash the DFU bootloader

The second option is using rumba for flashing. No further hardware has to be provided for this option.

FIXME

=== Using DFU to flash application

FIXME
