/* simtrace2_usb - host PC application to list found SIMtrace 2 USB devices
 *
 * (C) 2016 by Harald Welte <hwelte@hmw-consulting.de>
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

#include <osmocom/core/utils.h>

#include <osmocom/usb/libusb.h>
#include <osmocom/simtrace2/simtrace_usb.h>
#include <osmocom/simtrace2/usb_util.h>

static int find_devices(void)
{
	struct usb_interface_match ifm[16];
	int rc, i, num_interfaces;

	/* scan for USB devices matching SIMtrace USB ID with proprietary class */
	rc = osmo_libusb_find_matching_interfaces(NULL, osmo_st2_compatible_dev_ids,
						  USB_CLASS_PROPRIETARY, -1, -1, ifm, ARRAY_SIZE(ifm));
	printf("USB matches: %d\n", rc);
	if (rc < 0)
		return rc;
	num_interfaces = rc;

	for (i = 0; i < num_interfaces; i++) {
		struct usb_interface_match *m = &ifm[i];
		libusb_device_handle *dev_handle;
		char strbuf[256];

		printf("\t%04x:%04x Addr=%u, Path=%s, Cfg=%u, Intf=%u, Alt=%u: %d/%d/%d ",
			m->vendor, m->product, m->addr, m->path,
			m->configuration, m->interface, m->altsetting,
			m->class, m->sub_class, m->protocol);

		rc = libusb_open(m->usb_dev, &dev_handle);
		if (rc < 0) {
			printf("\n");
			perror("Cannot open device");
			continue;
		}
		rc = libusb_get_string_descriptor_ascii(dev_handle, m->string_idx,
					(unsigned char *)strbuf, sizeof(strbuf));
		libusb_close(dev_handle);
		if (rc < 0) {
			printf("\n");
			perror("Cannot read string");
			continue;
		}
		printf("(%s)\n", strbuf);
#if 0
		dev_handle = usb_open_claim_interface(NULL, m);
		printf("dev_handle=%p\n", dev_handle);
		libusb_close(dev_handle);
#endif
	}

	return num_interfaces;
}

int main(int argc, char **argv)
{
	osmo_libusb_init(NULL);
	find_devices();
	return 0;
}
