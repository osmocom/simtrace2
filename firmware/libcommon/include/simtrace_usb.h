/* SIMtrace 2 USB definitions
 *
 * (C) 2018 by sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA
 */
/* SIMtrace USB IDs */
#define USB_VENDOR_OPENMOKO			0x1d50
#define USB_PRODUCT_OWHW_SAM3_DFU	0x4001	/* was 0x4000 */
#define USB_PRODUCT_OWHW_SAM3		0x4001
#define USB_PRODUCT_QMOD_HUB		0x4002
#define USB_PRODUCT_QMOD_SAM3_DFU	0x4004	/* was 0x4003 */
#define USB_PRODUCT_QMOD_SAM3		0x4004
#define USB_PRODUCT_SIMTRACE2_DFU	0x60e3	/* was 0x60e2 */
#define USB_PRODUCT_SIMTRACE2		0x60e3
#define USB_PRODUCT_OCTSIMTEST		0x616d

/* USB proprietary class */
#define USB_CLASS_PROPRIETARY			0xff

/* SIMtrace USB sub-classes */
/*! Sniffer USB sub-class */
#define SIMTRACE_SNIFFER_USB_SUBCLASS	1
/*! Card-emulation USB sub-class */
#define SIMTRACE_CARDEM_USB_SUBCLASS	2

/* Generic USB endpoint numbers */
/*! Card-side USB data out (host to device) endpoint number */
#define SIMTRACE_USB_EP_CARD_DATAOUT	1
/*! Card-side USB data in (device to host) endpoint number */
#define SIMTRACE_USB_EP_CARD_DATAIN		2
/*! Card-side USB interrupt endpoint number */
#define SIMTRACE_USB_EP_CARD_INT		3
/*! Phone-side USB data out (host to device) endpoint number */
#define SIMTRACE_USB_EP_PHONE_DATAOUT	4
/*! Phone-side USB data in (device to host) endpoint number */
#define SIMTRACE_USB_EP_PHONE_DATAIN	5
/*! Phone-side USB interrupt endpoint number */
#define SIMTRACE_USB_EP_PHONE_INT		6

/* Card-emulation USB endpoint numbers */
/*! USIM1 USB data out (host to device) endpoint number */
#define SIMTRACE_CARDEM_USB_EP_USIM1_DATAOUT	4
/*! USIM1 USB data in (device to host) endpoint number */
#define SIMTRACE_CARDEM_USB_EP_USIM1_DATAIN		5
/*! USIM1 USB interrupt endpoint number */
#define SIMTRACE_CARDEM_USB_EP_USIM1_INT		6
/*! USIM2 USB data out (host to device) endpoint number */
#define SIMTRACE_CARDEM_USB_EP_USIM2_DATAOUT	1
/*! USIM2 USB data in (device to host) endpoint number */
#define SIMTRACE_CARDEM_USB_EP_USIM2_DATAIN		2
/*! USIM2 USB interrupt endpoint number */
#define SIMTRACE_CARDEM_USB_EP_USIM2_INT		3

/*! Maximum number of endpoints */
#define BOARD_USB_NUMENDPOINTS		7 /* 0 (control) + 2 (interfaces) * 3 (endpoints) */
