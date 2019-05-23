This is the source code for SIMtrace 2 firmwares.

= Hardware

== Micro-Controller

The firmware is for Microchip (formerly Atmel) ATSAM3S4B micro-controllers (MCU).
Product page:  https://www.microchip.com/wwwproducts/en/ATSAM3S4B
Datasheet: http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-6500-32-bit-Cortex-M3-Microcontroller-SAM3S4-SAM3S2-SAM3S1_Datasheet.pdf

Note: The SAM3S is now not recommended for new designs.
It can be replaced by the pin-compatible SAM4S.
The MCU can be specified using the environment variable `CHIP` (set to `sam3s4` per default) for future MCU support.

== Boards

The SIMtrace 2 firmware supports multiple boards.
A board defines a given circuit board.
While compiling the firmware, set the target board using the `BOARD` environment variable (set to `qmod` per default).
The supported boards correspond to sub-folders under `libboard`.

Current boards supported are:

* `simtrace`: The good old Osmocom SIMtrace PCB with SAM3 instead of SAM7, open hardware.
* `qmod`: A sysmocom-proprietary quad mPCIe carrier board, publicly available
* `owhw`: An undisclosed sysmocom-internal board, not publicly available

= Firmware

== Library

The firmware uses the manufacturer provided Software Package (SoftPack) micro-controller library.
The original library is available at https://www.microchip.com/design-centers/32-bit/softpacks/legacy-softpacks .
Version 2.1 from 2001 is used: http://ww1.microchip.com/downloads/en/DeviceDoc/SAM3S_softpack_2.1_for_CodeSourcery_2010q1.zip
The SIMtrace 2 project uses the `libboard_sam3s-ek`, `libchip_sam3s`, and `usb` sub-libraries, saved in `atmel_softpack_libraries` (with local modifications).

Note: SoftPack is the legacy micro-controller library.
This library is now replaced by the Advanced Software Framework (ASF): https://www.microchip.com/avr-support/advanced-software-framework-(asf) .
The SAM3S ASF documentation is available at http://asf.atmel.com/docs/latest/sam3s/html/index.html .

== Applications

An application is a specific piece of software with given functionality.
While compiling the firmware, set the target application using the `APP` environment variable (set to `dfu` per default).
The supported applications correspond to sub-folder under `apps`.

Current applications supported are:

* `dfu`: The USB DFU bootloader to flash further main appliction firmwares.
* `ccid`: To use SIMtrace 2 as USB CCID smartcard reader.
* `cardem`: To provide remote SIM operation capabilities.
* `trace`:  To monitor the communication between a SIM card and a phone (corresponds to the functionality provide by the first SIMtrace)
* `triple_play`: To support the three previous functionalities, using USB configurations.

== Memories

Firmwares can be run from several memory locations:

* flash: Run natively from start of flash memory
* dfu: Run after a DFU bootloader from an offset after the first 16k of flash (the first 16k are reserved for the bootloader)
* ram: Run from within the RAM of the chip, downloaded via JTAG/SWD

== Building

A given firmware build is made for a specific combination of an application `APP` running in a certain memory `MEM` on a given board `BOARD`.
When building using `make`, set the target application using the `APP` environment variable and target board using the `BOARD`  environment variable, e.g.:

* make APP=cardem BOARD=qmod
* make APP=dfu BOARD=qmod

The Makefile will create output files in the format: `bin/$(BOARD)-$(APP)-$(MEM).{elf,bin}`

The level of debug messages can be altered at compile time:
```
$ make TRACE_LEVEL=4
```
Accepted values: 0 (NO_TRACE) to 5 (DEBUG)

The qmod specific option `ALLOW_PEER_ERASE` controls if the UART debug command to assert the peer SAM3S ERASE line is present in the code.
Per default this is set to 0 to prevent accidentally erasing all firmware, including the DFU bootloader, which would then need to be flashed using SAM-BA or JTAG/SWD.
Setting `ALLOW_PEER_ERASE` to 1 enables back the debug command and should be used only for debugging or development purposes.

= Flashing

To flash a firmware image follow the instructions provided in the [wiki](https://projects.osmocom.org/projects/simtrace2/wiki/).
