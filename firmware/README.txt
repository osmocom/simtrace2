
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
