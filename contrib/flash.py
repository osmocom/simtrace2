#!/usr/bin/env python
# encoding: utf-8
# python: 3.8.1

# library to enumerate USB devices
import usb.core
from usb.util import *
# more elegant structure
from typing import NamedTuple
# regular expressions utilities
import re
# open utilities to handle files
import os, sys
# to download the firmwares
import urllib.request
# to flash using DFU-util
import subprocess

# SIMtrace 2 device information
class Device(NamedTuple):
	usb_vendor_id: int
	usb_product_id: int
	name: str
	url: dict # 1: sniff/trace firmware, 2: card emulation firmware

# SIMtrace 2 devices definitions
DEVICE_SIMTRACE = Device(usb_vendor_id=0x1d50, usb_product_id=0x60e3, name="SIMtrace 2", url={"trace": "https://ftp.osmocom.org/binaries/simtrace2/firmware/latest/simtrace-trace-dfu-latest.bin", "cardem": "https://osmocom.org/attachments/download/3868/simtrace-cardem-dfu.bin"})
DEVICE_QMOD = Device(usb_vendor_id=0x1d50, usb_product_id=0x4004, name="sysmoQMOD (Quad Modem)", url={"cardem": "https://ftp.osmocom.org/binaries/simtrace2/firmware/latest/qmod-cardem-dfu-latest.bin"})
DEVICE_OWHW = Device(usb_vendor_id=0x1d50, usb_product_id=0x4001, name="OWHW", url={"cardem": "https://ftp.osmocom.org/binaries/simtrace2/firmware/latest/owhw-cardem-dfu-latest.bin"})
DEVICE_OCTSIMTEST = Device(usb_vendor_id=0x1d50, usb_product_id=0x616d, name="OCTSIMTEST", url={"cardem": "https://ftp.osmocom.org/binaries/simtrace2/firmware/latest/octsimtest-cardem-dfu-latest.bin"})
DEVICE_NGFF_CARDEM = Device(usb_vendor_id=0x1d50, usb_product_id=0x616e, name="ngff-cardem", url={"cardem": "https://ftp.osmocom.org/binaries/simtrace2/firmware/latest/ngff_cardem-cardem-dfu-latest.bin"})
DEVICES = [DEVICE_SIMTRACE, DEVICE_QMOD, DEVICE_OCTSIMTEST, DEVICE_NGFF_CARDEM]

# which firmware does the SIMtrace USN interface subclass correspond
FIRMWARE_SUBCLASS = {1: "trace", 2: "cardem"}

def print_help():
	print("this script will flash SIMtrace 2 - based devices")
	print("when no argument is provided, it will try to flash the application firmware of all SIMtrace 2 devices connected to USB with the latest version")
	print("to flash a specific firmware, provide the name as argument")
	print("the possible firmwares are: trace, cardem")
	print("to list all devices connected to USB, provide the argument \"list\"")

# the firmware to flash
to_flash = None

# parse command line argument
if len(sys.argv) == 2:
  to_flash = sys.argv[1]
if to_flash not in ["list", "trace", "cardem"] and len(sys.argv) > 1:
	print_help()
	exit(0)

# get all USB devices
devices = []
devices_nb = 0
updated_nb = 0
usb_devices = usb.core.find(find_all=True)
for usb_device in usb_devices:
	# find SIMtrace devices
	definitions = list(filter(lambda x: x.usb_vendor_id == usb_device.idVendor and x.usb_product_id == usb_device.idProduct, DEVICES))
	if 1 != len(definitions):
		continue
	devices_nb += 1
	definition = definitions[0]
	serial = usb_device.serial_number or "unknown"
	usb_path = str(usb_device.bus) + "-" + ".".join(map(str, usb_device.port_numbers))
	print("found " + definition.name + " device (chip ID " + serial + ") at USB path " + usb_path)
	# determine if we are running DFU (in most cases the bootloader, but could also be the application)
	dfu_interface = None
	for configuration in usb_device:
		# get DFU interface descriptor
		dfu_interface = dfu_interface or find_descriptor(configuration, bInterfaceClass=254, bInterfaceSubClass=1)
	if (None == dfu_interface):
		print("no DFU USB interface found")
		continue
	dfu_mode = (2 == dfu_interface.bInterfaceProtocol) # InterfaceProtocol 1 is runtime mode, 2 is DFU mode
	# determine firmware type (when not in DFU mode)
	firmware = None
	simtrace_interface = None
	for configuration in usb_device:
		simtrace_interface = simtrace_interface or find_descriptor(configuration, bInterfaceClass=255)
	if simtrace_interface and simtrace_interface.bInterfaceSubClass in FIRMWARE_SUBCLASS:
		firmware = firmware or FIRMWARE_SUBCLASS[simtrace_interface.bInterfaceSubClass]
	if dfu_mode:
		firmware = 'dfu'
	if firmware:
		print("installed firmware: " + firmware)
	else:
		print("unknown installed firmware")
		continue
	# determine version of the application/bootloader firmware
	version = None
	version_interface = None
	for configuration in usb_device:
		# get custom interface with string
		version_interface = version_interface or find_descriptor(configuration, bInterfaceClass=255, bInterfaceSubClass=255)
	if version_interface and version_interface.iInterface and version_interface.iInterface > 0 and get_string(usb_device, version_interface.iInterface):
		version = get_string(usb_device, version_interface.iInterface)
	if not version:
		# the USB serial is set (in the application) since version 0.5.1.34-e026 from 2019-08-06
		# https://git.osmocom.org/simtrace2/commit/?id=e0265462d8c05ebfa133db2039c2fbe3ebbd286e
		# the USB serial is set (in the bootloader) since version 0.5.1.45-ac7e from 2019-11-18
		# https://git.osmocom.org/simtrace2/commit/?id=5db9402a5f346e30288db228157f71c29aefce5a
		# the firmware version is set (in the application) since version 0.5.1.37-ede8 from 2019-08-13
		# https://git.osmocom.org/simtrace2/commit/?id=ede87e067dadd07119f24e96261b66ac92b3af6f
		# the firmware version is set (in the bootloader) since version 0.5.1.45-ac7e from 2019-11-18
		# https://git.osmocom.org/simtrace2/commit/?id=5db9402a5f346e30288db228157f71c29aefce5a
		if dfu_mode:
			if serial:
				version = "< 0.5.1.45-ac7e"
			else:
				versoin = "< 0.5.1.45-ac7e"
		else:
			if serial:
				version = "< 0.5.1.37-ede8"
			else:
				versoin = "< 0.5.1.34-e026"
	print("device firmware version: " + version)
	# flash latest firmware
	if to_flash == "list": # we just want to list the devices, not flash them
		continue
	# check the firmware exists
	if firmware == "dfu" and to_flash is None:
		print("device is currently in DFU mode. you need to specify which firmware to flash")
		continue
	to_flash = to_flash or firmware
	if to_flash not in definition.url.keys():
		print("no firmware image available for " + firmware + " firmware")
		continue
	# download firmware
	try:
		dl_path, header = urllib.request.urlretrieve(definition.url[to_flash])
	except:
		print("could not download firmware " + definition.url[to_flash])
		continue
	dl_file = open(dl_path, "rb")
	dl_data = dl_file.read()
	dl_file.close()
	# compare versions
	dl_version = re.search(b'firmware \d+\.\d+\.\d+\.\d+-[0-9a-fA-F]{4}', dl_data)
	if dl_version is None:
		print("could not get version from downloaded firmware image")
		os.remove(dl_path)
		continue
	dl_version = dl_version.group(0).decode("utf-8").split(" ")[1]
	print("latest firmware version: " + dl_version)
	versions = list(map(lambda x: int(x), version.split(" ")[-1].split("-")[0].split(".")))
	dl_versions = list(map(lambda x: int(x), dl_version.split("-")[0].split(".")))
	dl_newer = (versions[0] < dl_versions[0] or (versions[0] == dl_versions[0] and versions[1] < dl_versions[1]) or (versions[0] == dl_versions[0] and versions[1] == dl_versions[1] and versions[2] < dl_versions[2]) or (versions[0] == dl_versions[0] and versions[1] == dl_versions[1] and versions[2] == dl_versions[2] and versions[3] < dl_versions[3]))
	if not dl_newer:
		print("no need to flash latest version")
		os.remove(dl_path)
		continue
	print("flashing latest version")
	dfu_result = subprocess.run(["dfu-util", "--device", hex(definition.usb_vendor_id) + ":" + hex(definition.usb_product_id), "--path", usb_path, "--cfg", "1", "--alt", "1", "--reset", "--download", dl_path])
	os.remove(dl_path)
	if 0 != dfu_result.returncode:
		printf("flashing firmware using dfu-util failed. ensure dfu-util is installed and you have the permissions to access this USB device")
		continue
	updated_nb += 1

print(str(devices_nb)+ " SIMtrace 2 device(s) found")
print(str(updated_nb)+ " SIMtrace 2 device(s) updated")
