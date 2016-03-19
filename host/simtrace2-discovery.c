#include <stdint.h>

#include <libusb.h>

/*! \brief obtain the endpoint addresses for a given USB interface */
int get_usb_ep_addrs(libusb_device_handle *devh, unsigned int if_num,
		     uint8_t *out, uint8_t *in, uint8_t *irq)
{
	libusb_device *dev = libusb_get_device(devh);
	struct libusb_config_descriptor *cdesc;
	const struct libusb_interface_descriptor *idesc;
	const struct libusb_interface *iface;
	int rc, l;

	rc = libusb_get_active_config_descriptor(dev, &cdesc);
	if (rc < 0)
		return rc;

	iface = &cdesc->interface[if_num];
	/* FIXME: we assume there's no altsetting */
	idesc = &iface->altsetting[0];

	for (l = 0; l < idesc->bNumEndpoints; l++) {
		const struct libusb_endpoint_descriptor *edesc = &idesc->endpoint[l];
		switch (edesc->bmAttributes & 3) {
		case LIBUSB_TRANSFER_TYPE_BULK:
			if (edesc->bEndpointAddress & 0x80) {
				if (in)
					*in = edesc->bEndpointAddress;
			} else {
				if (out)
					*out = edesc->bEndpointAddress;
			}
			break;
		case LIBUSB_TRANSFER_TYPE_INTERRUPT:
			if (irq)
				*irq = edesc->bEndpointAddress;
			break;
		default:
			break;
		}
	}
	return 0;
}

#if 0
	struct libusb_device_descriptor ddesc;
	int rc, i, j, k;

	rc = libusb_get_device_descriptor(devh, &ddesc);
	if (rc < 0)
		return;

	for (i = 0; i < ddesc.bNumConfigurations; i++) {
		struct libusb_config_descriptor *cdesc;
		rc = libusb_get_config_descriptor(devh, i, &cdesc);
		if (rc < 0)
			return;

		for (j = 0; j < cdesc->bNumInterfaces; j++) {
			const struct libusb_interface *iface = cdesc->interface[j];
			for (k = 0; k < iface->num_altsetting; k++) {
				const struct libusb_interface_descriptor *idesc = iface->altsetting[k];
				/* make sure this is the interface we're looking for */
				if (idesc->bInterfaceClass != 0xFF ||
				    idesc->bInterfaceSubClass != if_class ||
				    idsec->bInterfaceProtocol != if_proto)
					continue;
				/* FIXME */
			}
		}

		libusb_free_config_descriptor(cdesc);
	}
#endif
