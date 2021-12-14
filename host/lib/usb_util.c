/* usb_util - USB related utilities for SIMtrace 2 USB devices
 *
 * (C) 2016-2019 by Harald Welte <hwelte@hmw-consulting.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>

#include <osmocom/core/utils.h>

#include <osmocom/usb/libusb.h>
#include <osmocom/simtrace2/simtrace_usb.h>

/*! list of USB idVendor/idProduct tuples of devices using simtrace2 firmware */
const struct dev_id osmo_st2_compatible_dev_ids[] = {
	{ USB_VENDOR_OPENMOKO, USB_PRODUCT_OWHW_SAM3 },
	{ USB_VENDOR_OPENMOKO, USB_PRODUCT_QMOD_SAM3 },
	{ USB_VENDOR_OPENMOKO, USB_PRODUCT_SIMTRACE2 },
	{ USB_VENDOR_OPENMOKO, USB_PRODUCT_OCTSIMTEST },
	{ USB_VENDOR_OPENMOKO, USB_PRODUCT_NGFF_CARDEM },
	{ 0, 0 }
};
