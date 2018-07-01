#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <osmocom/core/utils.h>

#include "libusb_util.h"
#include "simtrace_usb.h"

static const struct dev_id compatible_dev_ids[] = {
	{ USB_VENDOR_OPENMOKO, USB_PRODUCT_OWHW_SAM3 },
	{ USB_VENDOR_OPENMOKO, USB_PRODUCT_QMOD_SAM3 },
	{ USB_VENDOR_OPENMOKO, USB_PRODUCT_SIMTRACE2 },
	{ 0, 0 }
};

static int find_devices(void)
{
	struct usb_interface_match ifm[16];
	int rc, i, num_interfaces;

	/* scan for USB devices matching SIMtrace USB ID with proprietary class */ 
	rc = usb_match_interfaces(NULL, compatible_dev_ids,
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
	libusb_init(NULL);
	find_devices();
}
