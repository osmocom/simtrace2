# SIMtrace v2.0
The SIMtrace software together with the corresponding hardware provides a means to trace the communication between a SIM card and a mobile phone, and intercept it starting with SIMtrace software version 2.0 (together with SIMtrace board version 1.5).
Furthermore, it provides a SIM card emulation and CCID reader mode.

## How to compile
A Makefile is provided. It created an image under bin/project-flash.bin, which can directly be flashed on the board (see section "How to flash").

The level of debug messages can be altered at compile time:
```
$ make TRACE_LEVEL=4
```
Accepted values: 0 (NO_TRACE) to 5 (DEBUG)

## How to flash
For flashing the firmware, there are at least two options.
The first one is using openocd and a JTAG key.
For this option, a JTAG connector has to be soldered onto the board, which is not attached per default.

The Makefile already provides an option for that:
```
$ make program
```
This command will call the following command:
```
$ openocd -f openocd/openocd.cfg -c "init" -c "halt" -c "flash write_bank 0 ./bin/project-flash.bin 0" -c "reset" -c "shutdown"
```

The second option is using rumba for flashing. No further hardware has to be provided for this option.
The software can be obtained with the following shell command:
```
$ git clone git://git.osmocom.org/osmo-sdr.git
```

Flashing the compiled firmware can be done with the following command:
```
$ $OSMO_SDR_DIR/utils/rumba /dev/ttyACM0 flashmcu $FIRMWARE_DIR/bin/project-flash.bin
```

## How to set udev rules
The next step is defining the udev rules for simtrace.
Open the file /etc/udev/rules.d/90-simtrace.rules and enter those four lines:

```
# Temporary VID and PID
SUBSYSTEM=="usb", ATTR{idVendor}=="03eb", ATTR{idProduct}=="6004", MODE="666"
# Future SIMtrace VID and PID
SUBSYSTEM=="usb", ATTR{idVendor}=="16c0", ATTR{idProduct}=="0762)", MODE="666"
```

After reloading the udev rules, SIMtrace should be recognized by the operating system.

## How to use

After flashing the firmware and defining the udev rules, the python program simtrace.py can be used in order to command the board.
First, the configuration has to be set using the -C option, which has to be passed a number determining the mode:
1: Sniffer mode
2: CCID reader mode
3: Mobile phone emulation mode
4: MITM mode

For example, setting the device into MITM mode can be achieved with the following command:
```
$ simtrace.py -C4
```

After setting the configuration, one of the following functionalities can be started:
```
  -s, --sniff           Sniff communication!
  -S, --select_file     Transmit SELECT cmd!
  -p, --phone           Emulates simcard
  -m, --mitm            Intercept communication (MITM)
```

For example, in order to use simtrace in sniffer mode, the following command can be executed:
```
$ simtrace.py -C1 -s
```
For more information, execute the following command:
```
$ simtrace.py -h
```


## Logging
The Makefile furthermore provides an easy option for reading the log messages.
SIMtrace sends out log messages over the serial interface, using a connector with a 2.5mm jack.
Using a serial to USB converter, the log messages can be read using the following command:

```
$ make log SERIAL=/dev/ttyUSB*
```

If no SERIAL is defined, /dev/ttyUSB0 is taken by default.


## Known issues
* If there is an error, it might result from a missing instruction byte in the list of instructions that expect data from the simcard.
It can be updated in the file in apdu_split.py
Especially, if the sniffer mode works well, but the mitm mode fails, that's a good place to start looking.
The array is of the following form:
`INS_data_expected = [0xC0, 0xB0, 0x12]`

* For interacting with the SIM card (CCID reader and MITM mode), pcscd has to be
started on the computer.

* The maximum operating frequency of the device and hardware is not determined yet.
The function for changing the FIDI is not tested yet because no device could be obtained, which would change the FIDI in the middle of the communication.
Most devices stick with the default FIDI.

* The software assumes a master-slave-protocol: The master sends a command, the slave answers this.
If this premise is not met, the software will not operate properly.
This should be taken into account when programming the Mobile phone emulator or MITM mode.
