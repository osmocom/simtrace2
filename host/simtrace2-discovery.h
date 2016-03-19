#pragma once

#include <stdint.h>
#include <libusb.h>

int get_usb_ep_addrs(libusb_device_handle *devh, unsigned int if_num,
		     uint8_t *out, uint8_t *in, uint8_t *irq);
