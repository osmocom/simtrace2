/* libusb utilities
 * 
 * (C) 2010-2019 by Harald Welte <hwelte@hmw-consulting.de>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <libusb.h>

#include <osmocom/simtrace2/libusb_util.h>

static char path_buf[USB_MAX_PATH_LEN];

static char *get_path(libusb_device *dev)
{
#if (defined(LIBUSB_API_VERSION) && LIBUSB_API_VERSION >= 0x01000102) || (defined(LIBUSBX_API_VERSION) && LIBUSBX_API_VERSION >= 0x01000102)
	uint8_t path[8];
	int r,j;
	r = libusb_get_port_numbers(dev, path, sizeof(path));
	if (r > 0) {
		sprintf(path_buf,"%d-%d",libusb_get_bus_number(dev),path[0]);
		for (j = 1; j < r; j++){
			sprintf(path_buf+strlen(path_buf),".%d",path[j]);
		};
	}
	return path_buf;
#else
# warning "libusb too old - building without USB path support!"
	return NULL;
#endif
}

static int match_dev_id(const struct libusb_device_descriptor *desc, const struct dev_id *id)
{
	if ((desc->idVendor == id->vendor_id) && (desc->idProduct == id->product_id))
		return 1;
	return 0;
}


static int match_dev_ids(const struct libusb_device_descriptor *desc, const struct dev_id *ids)
{
	const struct dev_id *id;

	for (id = ids; id->vendor_id || id->product_id; id++) {
		if (match_dev_id(desc, id))
			return 1;
	}
	return 0;
}

libusb_device **find_matching_usb_devs(const struct dev_id *dev_ids)
{
	libusb_device **list;
	libusb_device **out = calloc(256, sizeof(libusb_device *));
	libusb_device **cur = out;
	unsigned int i;
	int rc;

	if (!out)
		return NULL;

	rc = libusb_get_device_list(NULL, &list);
	if (rc <= 0) {
		perror("No USB devices found");
		free(out);
		return NULL;
	}

	for (i = 0; list[i] != NULL; i++) {
		struct libusb_device_descriptor dev_desc;
		libusb_device *dev = list[i];

		rc = libusb_get_device_descriptor(dev, &dev_desc);
		if (rc < 0) {
			perror("Couldn't get device descriptor\n");
			libusb_unref_device(dev);
			continue;
		}

		if (match_dev_ids(&dev_desc, dev_ids)) {
			*cur = dev;
			cur++;
			/* FIXME: overflow check */
		} else
			libusb_unref_device(dev);
	}
	if (cur == out) {
		libusb_free_device_list(list, 1);
		free(out);
		return NULL;
	}

	libusb_free_device_list(list, 0);
	return out;
}

int dev_find_matching_interfaces(libusb_device *dev, int class, int sub_class, int protocol,
				 struct usb_interface_match *out, unsigned int out_len)
{
	struct libusb_device_descriptor dev_desc;
	int rc, i, out_idx = 0;
	uint8_t addr;
	char *path;

	rc = libusb_get_device_descriptor(dev, &dev_desc);
	if (rc < 0) {
		perror("Couldn't get device descriptor\n");
		return -EIO;
	}

	addr = libusb_get_device_address(dev);
	path = get_path(dev);

	/* iterate over all configurations */
	for (i = 0; i < dev_desc.bNumConfigurations; i++) {
		struct libusb_config_descriptor *conf_desc;
		int j;

		rc = libusb_get_config_descriptor(dev, i, &conf_desc);
		if (rc < 0) {
			fprintf(stderr, "Couldn't get config descriptor %u\n", i);
			continue;
		}
		/* iterate over all interfaces */
		for (j = 0; j < conf_desc->bNumInterfaces; j++) {
			const struct libusb_interface *intf = &conf_desc->interface[j];
			int k;
			/* iterate over all alternate settings */
			for (k = 0; k < intf->num_altsetting; k++) {
				const struct libusb_interface_descriptor *if_desc;
				if_desc = &intf->altsetting[k];
				if (class >= 0 && if_desc->bInterfaceClass != class)
					continue;
				if (sub_class >= 0 && if_desc->bInterfaceSubClass != sub_class)
					continue;
				if (protocol >= 0 && if_desc->bInterfaceProtocol != protocol)
					continue;
				/* MATCH! */
				out[out_idx].usb_dev = dev;
				out[out_idx].vendor = dev_desc.idVendor;
				out[out_idx].product = dev_desc.idProduct;
				out[out_idx].addr = addr;
				strncpy(out[out_idx].path, path, sizeof(out[out_idx].path)-1);
				out[out_idx].path[sizeof(out[out_idx].path)-1] = '\0';
				out[out_idx].configuration = conf_desc->bConfigurationValue;
				out[out_idx].interface = if_desc->bInterfaceNumber;
				out[out_idx].altsetting = if_desc->bAlternateSetting;
				out[out_idx].class = if_desc->bInterfaceClass;
				out[out_idx].sub_class = if_desc->bInterfaceSubClass;
				out[out_idx].protocol = if_desc->bInterfaceProtocol;
				out[out_idx].string_idx = if_desc->iInterface;
				out_idx++;
				if (out_idx >= out_len)
					return out_idx;
			}
		}
	}
	return out_idx;
}

int usb_match_interfaces(libusb_context *ctx, const struct dev_id *dev_ids,
			 int class, int sub_class, int protocol,
			 struct usb_interface_match *out, unsigned int out_len)
{
	struct usb_interface_match *out_cur = out;
	unsigned int out_len_remain = out_len;
	libusb_device **list;
	libusb_device **dev;

	list = find_matching_usb_devs(dev_ids);
	if (!list)
		return 0;

	for (dev = list; *dev; dev++) {
		int rc;

#if 0
		struct libusb_device_descriptor dev_desc;
		uint8_t ports[8];
		uint8_t addr;
		rc = libusb_get_device_descriptor(*dev, &dev_desc);
		if (rc < 0) {
			perror("Cannot get device descriptor");
			continue;
		}

		addr = libusb_get_device_address(*dev);

		rc = libusb_get_port_numbers(*dev, ports, sizeof(ports));
		if (rc < 0) {
			perror("Cannot get device path");
			continue;
		}

		printf("Found USB Device %04x:%04x at address %d\n",
			dev_desc.idVendor, dev_desc.idProduct, addr);
#endif

		rc = dev_find_matching_interfaces(*dev, class, sub_class, protocol, out_cur, out_len_remain);
		if (rc < 0)
			continue;
		out_cur += rc;
		out_len_remain -= rc;

	}
	return out_len - out_len_remain;
}

libusb_device_handle *usb_open_claim_interface(libusb_context *ctx,
					       const struct usb_interface_match *ifm)
{
	int rc, config;
	struct dev_id dev_ids[] = { { ifm->vendor, ifm->product }, { 0, 0 } };
	libusb_device **list;
	libusb_device **dev;
	libusb_device_handle *usb_devh = NULL;

	list = find_matching_usb_devs(dev_ids);
	if (!list) {
		perror("No USB device with matching VID/PID");
		return NULL;
	}

	for (dev = list; *dev; dev++) {
		int addr;
		char *path;

		addr = libusb_get_device_address(*dev);
		path = get_path(*dev);
		if ((ifm->addr && addr == ifm->addr) ||
		    (strlen(ifm->path) && !strcmp(path, ifm->path))) {
			rc = libusb_open(*dev, &usb_devh);
			if (rc < 0) {
				fprintf(stderr, "Cannot open device: %s\n", libusb_error_name(rc));
				usb_devh = NULL;
				break;
			}
			rc = libusb_get_configuration(usb_devh, &config);
			if (rc < 0) {
				fprintf(stderr, "Cannot get current configuration: %s\n", libusb_error_name(rc));
				libusb_close(usb_devh);
				usb_devh = NULL;
				break;
			}
			if (config != ifm->configuration) {
				rc = libusb_set_configuration(usb_devh, ifm->configuration);
				if (rc < 0) {
					fprintf(stderr, "Cannot set configuration: %s\n", libusb_error_name(rc));
					libusb_close(usb_devh);
					usb_devh = NULL;
					break;
				}
			}
			rc = libusb_claim_interface(usb_devh, ifm->interface);
			if (rc < 0) {
				fprintf(stderr, "Cannot claim interface: %s\n", libusb_error_name(rc));
				libusb_close(usb_devh);
				usb_devh = NULL;
				break;
			}
			rc = libusb_set_interface_alt_setting(usb_devh, ifm->interface, ifm->altsetting);
			if (rc < 0) {
				fprintf(stderr, "Cannot set interface altsetting: %s\n", libusb_error_name(rc));
				libusb_release_interface(usb_devh, ifm->interface);
				libusb_close(usb_devh);
				usb_devh = NULL;
				break;
			}
		}
	}

	/* unref / free list */
	for (dev = list; *dev; dev++)
		libusb_unref_device(*dev);
	free(list);

	return usb_devh;
}

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
