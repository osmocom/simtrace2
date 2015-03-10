#!/usr/bin/env python3

import usb.core
import usb.util
import sys

from pySim.transport.serial import SerialSimLink
from pySim.commands import SimCardCommands

dev = usb.core.find(idVendor=0x03eb, idProduct=0x6004)

if dev is None:
    raise ValueError("Device not found")
else:
    print("Found device")

print("dev.set_configuration(2)")
dev.set_configuration(2)

cfg = dev.get_active_configuration()
print("Active config: ")
print(cfg)

if len(sys.argv) == 2:
    device = sys.argv[1]
else:
    device='/dev/ttyUSB2'

baudrate='9600'


sl = SerialSimLink(device, baudrate)
scc = SimCardCommands(transport=sl)
sl.wait_for_card()

