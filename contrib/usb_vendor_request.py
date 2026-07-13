#!/usr/bin/env python3
#
# Copyright (C) 2026 sysmocom -s.f.m.c. GmbH, Author: Alexander Couzens <lynxis@fe80.eu>
# License: MIT

import sys
import usb.util
from usb.core import USBError

TIMEOUT = 3

# simtrace2 vendor requets
SIMTRACER_VENDOR_REQ_GET_DEBUG_INFO = 0

def vendor_get_debug(device: usb.core.Device):
    return vendor_req_in(device, bRequest = SIMTRACER_VENDOR_REQ_GET_DEBUG_INFO, wValue = 0, wIndex = 0, length = 32)

def vendor_req_in(device: usb.core.Device, bRequest: int, wValue: int, wIndex: int, length: int = 8):
    """ do a vendor request in (device -> host)
    @param bRequest: usb single byte
    @param wIndex: short index
    """
    return device.ctrl_transfer(
        bmRequestType=usb.util.ENDPOINT_IN
                      | usb.util.CTRL_TYPE_VENDOR
                      | usb.util.CTRL_RECIPIENT_DEVICE,
        bRequest=bRequest,
        wValue=wValue,
        wIndex=wIndex,
        data_or_wLength=length,
        timeout=TIMEOUT,
    )

def find_simtrace(vendor = 0x1d50, product = 0x60e3, find_all = False):
    return usb.core.find(find_all=find_all, idVendor=vendor, idProduct=product)

def get_path(device: usb.core.Device):
    ports = ".".join([str(x) for x in device.port_numbers])
    return f"{device.bus}-{ports}"

def per_device(device: usb.core.Device, args: argparse.Namespace):
    if args.get_debug:
            result = vendor_get_debug(device)
            print(f"{get_path(device)} / {device.serial_number}: {result}")

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(prog='simtrace-util')
    # actions
    parser.add_argument(
        "--get-debug",
        action='store_true',
        help="Get debug")

    # device selection
    parser.add_argument(
        "-a",
        "--all",
        action='store_true',
        help="Run the request against all simtrace2 devices.")
    parser.add_argument(
        "-s",
        "--serial",
        help="Select the simtrace2 by serial")
    parser.add_argument(
        "--vendor",
        type=int,
        help="Use a different USB vendor. Default Openmoko 0x1d50.")
    parser.add_argument(
        "-p",
        "--product",
        type=int,
        help="Use a different USB product. Default simtrace2 0x60e3.\n"
             "qmod: 0x4004")

    args = parser.parse_args()
    # no action given
    if not args.get_debug:
        parser.print_usage()
        sys.exit(1)

    vendor = 0x1d50
    product = 0x60e3
    if args.product:
        product = args.product
    if args.vendor:
        vendor = args.vendor
    if args.serial:
        device = find_simtrace(vendor=vendor, product=product, find_all=args.all, serial_number=args.serial_number)
    else:
        device = find_simtrace(vendor=vendor, product=product, find_all=args.all)

    if not device:
        print("Could not find a single board", file=sys.stderr)
        sys.exit(1)

    if type(device) == usb.core.Device:
        per_device(device, args)
    else:
        for dev in device:
            per_device(dev, args)
