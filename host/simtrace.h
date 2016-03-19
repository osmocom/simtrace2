#ifndef _SIMTRACE_H
#define _SIMTRACE_H

#define SIMTRACE_USB_VENDOR	0x1d50
#define SIMTRACE_USB_PRODUCT	0x60e3

/* FIXME: automatically determine those values based on the usb config /
 * interface descriptors */
#define SIMTRACE_OUT_EP	0x04
#define SIMTRACE_IN_EP	0x85
#define SIMTRACE_INT_EP	0x86

#endif
