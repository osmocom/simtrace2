/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2009, Atmel Corporation
 * Copyright (c) 2018-2019, sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------
 *         Headers
 *----------------------------------------------------------------------------*/

#include "board.h"
#include "simtrace.h"
#include "simtrace_usb.h"
#include "utils.h"
#include "USBD_HAL.h"

#include <cciddriverdescriptors.h>
#include <usb/common/dfu/usb_dfu.h>
#include <usb/device/dfu/dfu.h>

/*------------------------------------------------------------------------------
 *       USB String descriptors 
 *------------------------------------------------------------------------------*/
#include "usb_strings_generated.h"

// the index of the strings (must match the order in usb_strings.txt)
enum strDescNum {
	// static strings from usb_strings
	MANUF_STR = 1,
	PRODUCT_STRING,
	SNIFFER_CONF_STR,
	CCID_CONF_STR,
	PHONE_CONF_STR,
	MITM_CONF_STR,
	CARDEM_USIM1_INTF_STR,
	CARDEM_USIM2_INTF_STR,
	CARDEM_USIM3_INTF_STR,
	CARDEM_USIM4_INTF_STR,
	// runtime strings
	SERIAL_STR,
	VERSION_CONF_STR,
	VERSION_STR,
	// count
	STRING_DESC_CNT
};

/** array of static (from usb_strings) and runtime (serial, version) USB strings
 */
static const unsigned char *usb_strings_extended[ARRAY_SIZE(usb_strings) + 3];

/* USB string for the serial (using 128-bit device ID) */
static unsigned char usb_string_serial[] = {
	USBStringDescriptor_LENGTH(32),
	USBGenericDescriptor_STRING,
	USBStringDescriptor_UNICODE('0'),
	USBStringDescriptor_UNICODE('0'),
	USBStringDescriptor_UNICODE('1'),
	USBStringDescriptor_UNICODE('1'),
	USBStringDescriptor_UNICODE('2'),
	USBStringDescriptor_UNICODE('2'),
	USBStringDescriptor_UNICODE('3'),
	USBStringDescriptor_UNICODE('3'),
	USBStringDescriptor_UNICODE('4'),
	USBStringDescriptor_UNICODE('4'),
	USBStringDescriptor_UNICODE('5'),
	USBStringDescriptor_UNICODE('5'),
	USBStringDescriptor_UNICODE('6'),
	USBStringDescriptor_UNICODE('6'),
	USBStringDescriptor_UNICODE('7'),
	USBStringDescriptor_UNICODE('7'),
	USBStringDescriptor_UNICODE('8'),
	USBStringDescriptor_UNICODE('8'),
	USBStringDescriptor_UNICODE('9'),
	USBStringDescriptor_UNICODE('9'),
	USBStringDescriptor_UNICODE('a'),
	USBStringDescriptor_UNICODE('a'),
	USBStringDescriptor_UNICODE('b'),
	USBStringDescriptor_UNICODE('b'),
	USBStringDescriptor_UNICODE('c'),
	USBStringDescriptor_UNICODE('c'),
	USBStringDescriptor_UNICODE('d'),
	USBStringDescriptor_UNICODE('d'),
	USBStringDescriptor_UNICODE('e'),
	USBStringDescriptor_UNICODE('e'),
	USBStringDescriptor_UNICODE('f'),
	USBStringDescriptor_UNICODE('f'),
};

/* USB string for the version */
static const unsigned char usb_string_version_conf[] = {
	USBStringDescriptor_LENGTH(16),
	USBGenericDescriptor_STRING,
	USBStringDescriptor_UNICODE('f'),
	USBStringDescriptor_UNICODE('i'),
	USBStringDescriptor_UNICODE('r'),
	USBStringDescriptor_UNICODE('m'),
	USBStringDescriptor_UNICODE('w'),
	USBStringDescriptor_UNICODE('a'),
	USBStringDescriptor_UNICODE('r'),
	USBStringDescriptor_UNICODE('e'),
	USBStringDescriptor_UNICODE(' '),
	USBStringDescriptor_UNICODE('v'),
	USBStringDescriptor_UNICODE('e'),
	USBStringDescriptor_UNICODE('r'),
	USBStringDescriptor_UNICODE('s'),
	USBStringDescriptor_UNICODE('i'),
	USBStringDescriptor_UNICODE('o'),
	USBStringDescriptor_UNICODE('n'),
};
static const char git_version[] = GIT_VERSION;
static unsigned char usb_string_version[2 + ARRAY_SIZE(git_version) * 2 - 2];

/*------------------------------------------------------------------------------
 *       USB Device descriptors 
 *------------------------------------------------------------------------------*/

#ifdef HAVE_SNIFFER
typedef struct _SIMTraceDriverConfigurationDescriptorSniffer {

	/** Standard configuration descriptor. */
	USBConfigurationDescriptor configuration;
	USBInterfaceDescriptor sniffer;
	USBEndpointDescriptor sniffer_dataOut;
	USBEndpointDescriptor sniffer_dataIn;
	USBEndpointDescriptor sniffer_interruptIn;
	DFURT_IF_DESCRIPTOR_STRUCT;
} __attribute__ ((packed)) SIMTraceDriverConfigurationDescriptorSniffer;

static const SIMTraceDriverConfigurationDescriptorSniffer
				configurationDescriptorSniffer = {
	/* Standard configuration descriptor */
	.configuration = {
		.bLength 		= sizeof(USBConfigurationDescriptor),
		.bDescriptorType	= USBGenericDescriptor_CONFIGURATION,
		.wTotalLength		= sizeof(SIMTraceDriverConfigurationDescriptorSniffer),
		.bNumInterfaces		= 1+DFURT_NUM_IF,
		.bConfigurationValue	= CFG_NUM_SNIFF,
		.iConfiguration		= SNIFFER_CONF_STR,
		.bmAttributes		= USBD_BMATTRIBUTES,
		.bMaxPower		= USBConfigurationDescriptor_POWER(100),
	},
	/* Communication class interface standard descriptor */
	.sniffer = {
		.bLength = sizeof(USBInterfaceDescriptor),
		.bDescriptorType 	= USBGenericDescriptor_INTERFACE,
		.bInterfaceNumber	= 0,
		.bAlternateSetting	= 0,
		.bNumEndpoints		= 3,
		.bInterfaceClass	= USB_CLASS_PROPRIETARY,
		.bInterfaceSubClass	= SIMTRACE_SNIFFER_USB_SUBCLASS,
		.bInterfaceProtocol	= 0,
		.iInterface		= SNIFFER_CONF_STR,
	},
	/* Bulk-OUT endpoint standard descriptor */
	.sniffer_dataOut = {
		.bLength 		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_OUT,
						SIMTRACE_USB_EP_CARD_DATAOUT),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize	= USBEndpointDescriptor_MAXBULKSIZE_FS,
		.bInterval		= 0,
	},
	/* Bulk-IN endpoint descriptor */
	.sniffer_dataIn = {
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_IN,
						SIMTRACE_USB_EP_CARD_DATAIN),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize = USBEndpointDescriptor_MAXBULKSIZE_FS,
		.bInterval		= 0,
	},
	// Notification endpoint descriptor
	.sniffer_interruptIn = {
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_IN,
						SIMTRACE_USB_EP_CARD_INT),
		.bmAttributes		= USBEndpointDescriptor_INTERRUPT,
		.wMaxPacketSize	= USBEndpointDescriptor_MAXINTERRUPTSIZE_FS,
		.bInterval = 0x10,
	},
	DFURT_IF_DESCRIPTOR(1, 0),
};
#endif /* HAVE_SNIFFER */

#ifdef HAVE_CCID
static const CCIDDriverConfigurationDescriptors configurationDescriptorCCID = {
	// Standard USB configuration descriptor
	{
		.bLength		= sizeof(USBConfigurationDescriptor),
		.bDescriptorType	= USBGenericDescriptor_CONFIGURATION,
		.wTotalLength		= sizeof(CCIDDriverConfigurationDescriptors),
		.bNumInterfaces		= 1+DFURT_NUM_IF,
		.bConfigurationValue	= CFG_NUM_CCID,
		.iConfiguration		= CCID_CONF_STR,
		.bmAttributes		= BOARD_USB_BMATTRIBUTES,
		.bMaxPower		= USBConfigurationDescriptor_POWER(100),
	},
	// CCID interface descriptor
	// Table 4.3-1 Interface Descriptor
	// Interface descriptor
	{
		.bLength		= sizeof(USBInterfaceDescriptor),
		.bDescriptorType	= USBGenericDescriptor_INTERFACE,
		.bInterfaceNumber	= 0,
		.bAlternateSetting	= 0,
		.bNumEndpoints		= 3,
		.bInterfaceClass	= SMART_CARD_DEVICE_CLASS,
		.bInterfaceSubClass	= 0,
		.bInterfaceProtocol	= 0,
		.iInterface		= CCID_CONF_STR,
	},
	{
		.bLength		= sizeof(CCIDDescriptor),
		.bDescriptorType	= CCID_DECRIPTOR_TYPE,
		.bcdCCID		= CCID1_10,	// CCID version
		.bMaxSlotIndex		= 0,	// 1 slot 
		.bVoltageSupport	= VOLTS_3_0,
		.dwProtocols		= (1 << PROTOCOL_TO),
		.dwDefaultClock		= 3580,
		.dwMaximumClock		= 3580,
		.bNumClockSupported	= 0,
		.dwDataRate		= 9600,
		.dwMaxDataRate		= 9600,
		.bNumDataRatesSupported = 0,
		.dwMaxIFSD		= 0xfe,
		.dwSynchProtocols	= 0,
		.dwMechanical		= 0,
		.dwFeatures = CCID_FEATURES_AUTO_CLOCK | CCID_FEATURES_AUTO_BAUD |
			      CCID_FEATURES_AUTO_PCONF | CCID_FEATURES_AUTO_PNEGO |
			      CCID_FEATURES_EXC_TPDU,
		.dwMaxCCIDMessageLength	= 271,	/* For extended APDU
						   level the value shall
						   be between 261 + 10 */
		.bClassGetResponse	= 0xFF,	// Echoes the class of the APDU
		.bClassEnvelope		= 0xFF,	// Echoes the class of the APDU
		.wLcdLayout		= 0,	// wLcdLayout: no LCD
		.bPINSupport		= 0,	// bPINSupport: No PIN
		.bMaxCCIDBusySlots	= 1,
	},
	// Bulk-OUT endpoint descriptor
	{
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	=
			USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_OUT,
				      		      CCID_EPT_DATA_OUT),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize	= USBEndpointDescriptor_MAXBULKSIZE_FS,
		.bInterval		= 0x00,
	},
	// Bulk-IN endpoint descriptor
	{
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	=
			USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN,
						      CCID_EPT_DATA_IN),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize	= USBEndpointDescriptor_MAXBULKSIZE_FS),
		.bInterval		= 0x00,
	},
	// Notification endpoint descriptor
	{
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	=
			USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN,
						      CCID_EPT_NOTIFICATION),
		.bmAttributes		= USBEndpointDescriptor_INTERRUPT,
		.wMaxPacketSize = USBEndpointDescriptor_MAXINTERRUPTSIZE_FS,
		.bInterval = 0x10,
	},
	DFURT_IF_DESCRIPTOR(1, 0),
};
#endif /* HAVE_CCID */

#ifdef HAVE_CARDEM
/* SIM card emulator */
typedef struct _SIMTraceDriverConfigurationDescriptorPhone {
	/* Standard configuration descriptor. */
	USBConfigurationDescriptor configuration;
	USBInterfaceDescriptor phone;
	USBEndpointDescriptor phone_dataOut;
	USBEndpointDescriptor phone_dataIn;
	USBEndpointDescriptor phone_interruptIn;
#ifdef CARDEMU_SECOND_UART
	USBInterfaceDescriptor usim2;
	USBEndpointDescriptor usim2_dataOut;
	USBEndpointDescriptor usim2_dataIn;
	USBEndpointDescriptor usim2_interruptIn;
#endif
	DFURT_IF_DESCRIPTOR_STRUCT;
} __attribute__ ((packed)) SIMTraceDriverConfigurationDescriptorPhone;

static const SIMTraceDriverConfigurationDescriptorPhone
				configurationDescriptorPhone = {
	/* Standard configuration descriptor */
	.configuration = {
		.bLength		= sizeof(USBConfigurationDescriptor),
		.bDescriptorType	= USBGenericDescriptor_CONFIGURATION,
		.wTotalLength		= sizeof(SIMTraceDriverConfigurationDescriptorPhone),
#ifdef CARDEMU_SECOND_UART
		.bNumInterfaces		= 2+DFURT_NUM_IF,
#else
		.bNumInterfaces		= 1+DFURT_NUM_IF,
#endif
		.bConfigurationValue	= CFG_NUM_PHONE,
		.iConfiguration		= PHONE_CONF_STR,
		.bmAttributes		= USBD_BMATTRIBUTES,
		.bMaxPower		= USBConfigurationDescriptor_POWER(100)
	},
	/* Communication class interface standard descriptor */
	.phone = {
		.bLength		= sizeof(USBInterfaceDescriptor),
		.bDescriptorType	= USBGenericDescriptor_INTERFACE,
		.bInterfaceNumber	= 0,
		.bAlternateSetting	= 0,
		.bNumEndpoints		= 3,
		.bInterfaceClass	= USB_CLASS_PROPRIETARY,
		.bInterfaceSubClass	= SIMTRACE_CARDEM_USB_SUBCLASS,
		.bInterfaceProtocol	= 0,
		.iInterface		= CARDEM_USIM1_INTF_STR,
	},
	/* Bulk-OUT endpoint standard descriptor */
	.phone_dataOut = {
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_OUT,
						SIMTRACE_CARDEM_USB_EP_USIM1_DATAOUT),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize		= USBEndpointDescriptor_MAXBULKSIZE_FS,
		.bInterval		= 0	/* Must be 0 for full-speed bulk endpoints */
	},
	/* Bulk-IN endpoint descriptor */
	.phone_dataIn = {
		.bLength 		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_IN,
						SIMTRACE_CARDEM_USB_EP_USIM1_DATAIN),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize		= USBEndpointDescriptor_MAXBULKSIZE_FS,
		.bInterval		= 0	/* Must be 0 for full-speed bulk endpoints */
	},
	/* Notification endpoint descriptor */
	.phone_interruptIn = {
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_IN,
						SIMTRACE_CARDEM_USB_EP_USIM1_INT),
		.bmAttributes		= USBEndpointDescriptor_INTERRUPT,
		.wMaxPacketSize		= USBEndpointDescriptor_MAXINTERRUPTSIZE_FS,
		.bInterval		= 0x10
	},
#ifdef CARDEMU_SECOND_UART
	/* Communication class interface standard descriptor */
	.usim2 = {
		.bLength		= sizeof(USBInterfaceDescriptor),
		.bDescriptorType	= USBGenericDescriptor_INTERFACE,
		.bInterfaceNumber	= 1,
		.bAlternateSetting	= 0,
		.bNumEndpoints		= 3,
		.bInterfaceClass	= USB_CLASS_PROPRIETARY,
		.bInterfaceSubClass	= SIMTRACE_CARDEM_USB_SUBCLASS,
		.bInterfaceProtocol	= 0,
		.iInterface		= CARDEM_USIM2_INTF_STR,
	},
	/* Bulk-OUT endpoint standard descriptor */
	.usim2_dataOut = {
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_OUT,
						SIMTRACE_CARDEM_USB_EP_USIM2_DATAOUT),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize		= USBEndpointDescriptor_MAXBULKSIZE_FS,
		.bInterval		= 0	/* Must be 0 for full-speed bulk endpoints */
	}
	,
	/* Bulk-IN endpoint descriptor */
	.usim2_dataIn = {
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_IN,
						SIMTRACE_CARDEM_USB_EP_USIM2_DATAIN),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize		= USBEndpointDescriptor_MAXBULKSIZE_FS,
		.bInterval		= 0	/* Must be 0 for full-speed bulk endpoints */
	},
	/* Notification endpoint descriptor */
	.usim2_interruptIn = {
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_IN,
						SIMTRACE_CARDEM_USB_EP_USIM2_INT),
		.bmAttributes		= USBEndpointDescriptor_INTERRUPT,
		.wMaxPacketSize		= USBEndpointDescriptor_MAXINTERRUPTSIZE_FS,
		.bInterval		= 0x10,
	},
	DFURT_IF_DESCRIPTOR(2, 0),
#else
	DFURT_IF_DESCRIPTOR(1, 0),
#endif
};
#endif /* HAVE_CARDEM */

#ifdef HAVE_MITM
typedef struct _SIMTraceDriverConfigurationDescriptorMITM {
	/* Standard configuration descriptor. */
	USBConfigurationDescriptor configuration;
	USBInterfaceDescriptor simcard;
	/// CCID descriptor
	CCIDDescriptor ccid;
	/// Bulk OUT endpoint descriptor
	USBEndpointDescriptor simcard_dataOut;
	/// Bulk IN endpoint descriptor
	USBEndpointDescriptor simcard_dataIn;
	/// Interrupt OUT endpoint descriptor
	USBEndpointDescriptor simcard_interruptIn;

	USBInterfaceDescriptor phone;
	USBEndpointDescriptor phone_dataOut;
	USBEndpointDescriptor phone_dataIn;
	USBEndpointDescriptor phone_interruptIn;

	DFURT_IF_DESCRIPTOR_STRUCT;

} __attribute__ ((packed)) SIMTraceDriverConfigurationDescriptorMITM;

static const SIMTraceDriverConfigurationDescriptorMITM
				configurationDescriptorMITM = {
	/* Standard configuration descriptor */
	.configuration = {
		.bLength		= sizeof(USBConfigurationDescriptor),
		.bDescriptorType	= USBGenericDescriptor_CONFIGURATION,
		.wTotalLength		= sizeof(SIMTraceDriverConfigurationDescriptorMITM),
		.bNumInterfaces		= 2+DFURT_NUM_IF,
		.bConfigurationValue	= CFG_NUM_MITM,
		.iConfiguration		= MITM_CONF_STR,
		.bmAttributes		= USBD_BMATTRIBUTES,
		.bMaxPower		= USBConfigurationDescriptor_POWER(100),
	},
	// CCID interface descriptor
	// Table 4.3-1 Interface Descriptor
	// Interface descriptor
	.simcard = {
		.bLength		= sizeof(USBInterfaceDescriptor),
		.bDescriptorType	= USBGenericDescriptor_INTERFACE,
		.bInterfaceNumber	= 0,
		.bAlternateSetting	= 0,
		.bNumEndpoints		= 3,
		.bInterfaceClass	= SMART_CARD_DEVICE_CLASS,
		.bInterfaceSubClass	= 0,
		.bInterfaceProtocol	= 0,
		.iInterface		= CCID_CONF_STR,
	},
	.ccid = {
		.bLength		= sizeof(CCIDDescriptor),
		.bDescriptorType	= CCID_DECRIPTOR_TYPE,
		.bcdCCID		= CCID1_10,	// CCID version
		.bMaxSlotIndex		= 0,	// 1 slot 
		.bVoltageSupport	= VOLTS_3_0,
		.dwProtocols		= (1 << PROTOCOL_TO),
		.dwDefaultClock		= 3580,
		.dwMaximumClock		= 3580,
		.bNumClockSupported	= 0,
		.dwDataRate		= 9600,
		.dwMaxDataRate		= 9600,
		.bNumDataRatesSupported = 0,
		.dwMaxIFSD		= 0xfe,
		.dwSynchProtocols	= 0,
		.dwMechanical		= 0,
		.dwFeatures = CCID_FEATURES_AUTO_CLOCK | CCID_FEATURES_AUTO_BAUD |
			      CCID_FEATURES_AUTO_PCONF | CCID_FEATURES_AUTO_PNEGO |
			      CCID_FEATURES_EXC_TPDU,
		.dwMaxCCIDMessageLength	= 271,	/* For extended APDU
						   level the value shall
						   be between 261 + 10 */
		.bClassGetResponse	= 0xFF,	// Echoes the class of the APDU
		.bClassEnvelope		= 0xFF,	// Echoes the class of the APDU
		.wLcdLayout		= 0,	// wLcdLayout: no LCD
		.bPINSupport		= 0,	// bPINSupport: No PIN
		.bMaxCCIDBusySlots	= 1,
	},
	// Bulk-OUT endpoint descriptor
	.simcard_dataOut = {
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	=
			USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_OUT,
				      		      CCID_EPT_DATA_OUT),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize	= USBEndpointDescriptor_MAXBULKSIZE_FS,
		.bInterval		= 0x00,
	},
	// Bulk-IN endpoint descriptor
	.simcard_dataIn = {
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	=
			USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN,
						      CCID_EPT_DATA_IN),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize	= USBEndpointDescriptor_MAXBULKSIZE_FS,
		.bInterval		= 0x00,
	},
	// Notification endpoint descriptor
	.simcard_interruptIn = {
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	=
			USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN,
						      CCID_EPT_NOTIFICATION),
		.bmAttributes		= USBEndpointDescriptor_INTERRUPT,
		.wMaxPacketSize = USBEndpointDescriptor_MAXINTERRUPTSIZE_FS,
		.bInterval = 0x10,
	},

	/* Communication class interface standard descriptor */
	.phone = {
		.bLength		= sizeof(USBInterfaceDescriptor),
		.bDescriptorType	= USBGenericDescriptor_INTERFACE,
		.bInterfaceNumber	= 1,
		.bAlternateSetting	= 0,
		.bNumEndpoints		= 3,
		.bInterfaceClass	= 0xff,
		.bInterfaceSubClass	= SIMTRACE_SUBCLASS_CARDEM,
		.bInterfaceProtocol	= 0,
		.iInterface		= PHONE_CONF_STR,
	},
	/* Bulk-OUT endpoint standard descriptor */
	.phone_dataOut = {
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_OUT,
						SIMTRACE_USB_EP_PHONE_DATAOUT),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize		= USBEndpointDescriptor_MAXBULKSIZE_FS,
		.bInterval		= 0, /* Must be 0 for full-speed bulk endpoints */
	},
	/* Bulk-IN endpoint descriptor */
	.phone_dataIn = {
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_IN,
						SIMTRACE_USB_EP_PHONE_DATAIN),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize		= USBEndpointDescriptor_MAXBULKSIZE_FS,
		.bInterval		= 0, /* Must be 0 for full-speed bulk endpoints */
	},
	/* Notification endpoint descriptor */
	{
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_IN,
						SIMTRACE_USB_EP_PHONE_INT),
		.bmAttributes		= USBEndpointDescriptor_INTERRUPT,
		.wMaxPacketSize		= USBEndpointDescriptor_MAXINTERRUPTSIZE_FS,
		.bInterval		= 0x10
	},
	DFURT_IF_DESCRIPTOR(2, 0),
};
#endif /* HAVE_CARDEM */

/* USB descriptor just to show the version */
typedef struct _SIMTraceDriverConfigurationDescriptorVersion {
	/** Standard configuration descriptor. */
	USBConfigurationDescriptor configuration;
	USBInterfaceDescriptor version;
} __attribute__ ((packed)) SIMTraceDriverConfigurationDescriptorVersion;

static const SIMTraceDriverConfigurationDescriptorVersion
				configurationDescriptorVersion = {
	/* Standard configuration descriptor for the interface descriptor*/
	.configuration = {
		.bLength 		= sizeof(USBConfigurationDescriptor),
		.bDescriptorType	= USBGenericDescriptor_CONFIGURATION,
		.wTotalLength		= sizeof(SIMTraceDriverConfigurationDescriptorVersion),
		.bNumInterfaces		= 1,
		.bConfigurationValue	= CFG_NUM_VERSION,
		.iConfiguration		= VERSION_CONF_STR,
		.bmAttributes		= USBD_BMATTRIBUTES,
		.bMaxPower		= USBConfigurationDescriptor_POWER(100),
	},
	/* Interface standard descriptor just holding the version information */
	.version = {
		.bLength = sizeof(USBInterfaceDescriptor),
		.bDescriptorType 	= USBGenericDescriptor_INTERFACE,
		.bInterfaceNumber	= 0,
		.bAlternateSetting	= 0,
		.bNumEndpoints		= 0,
		.bInterfaceClass	= USB_CLASS_PROPRIETARY,
		.bInterfaceSubClass	= 0xff,
		.bInterfaceProtocol	= 0,
		.iInterface		= VERSION_STR,
	},
};

const USBConfigurationDescriptor *configurationDescriptorsArr[] = {
#ifdef HAVE_SNIFFER
	&configurationDescriptorSniffer.configuration,
#endif
#ifdef HAVE_CCID
	&configurationDescriptorCCID.configuration,
#endif
#ifdef HAVE_CARDEM
	&configurationDescriptorPhone.configuration,
#endif
#ifdef HAVE_MITM
	&configurationDescriptorMITM.configuration,
#endif
	&configurationDescriptorVersion.configuration,
};

/** Standard USB device descriptor for the CDC serial driver */
const USBDeviceDescriptor deviceDescriptor = {
	.bLength		= sizeof(USBDeviceDescriptor),
	.bDescriptorType	= USBGenericDescriptor_DEVICE,
	.bcdUSB			= USBDeviceDescriptor_USB2_00,
	.bDeviceClass		= 0,
	.bDeviceSubClass	= 0,
	.bDeviceProtocol	= 0,
	.bMaxPacketSize0	= 64,
	.idVendor		= BOARD_USB_VENDOR_ID,
	.idProduct		= BOARD_USB_PRODUCT_ID,
	.bcdDevice		= 2,	/* Release number */
	.iManufacturer		= MANUF_STR,
	.iProduct		= PRODUCT_STRING,
	.iSerialNumber		= SERIAL_STR,
	.bNumConfigurations	= ARRAY_SIZE(configurationDescriptorsArr),
};

/* AT91SAM3S only supports full speed, but not high speed USB */
static const USBDDriverDescriptors driverDescriptors = {
	&deviceDescriptor,
	(const USBConfigurationDescriptor **)&(configurationDescriptorsArr),	/* first full-speed configuration descriptor */
	0,			/* No full-speed device qualifier descriptor */
	0,			/* No full-speed other speed configuration */
	0,			/* No high-speed device descriptor */
	0,			/* No high-speed configuration descriptor */
	0,			/* No high-speed device qualifier descriptor */
	0,			/* No high-speed other speed configuration descriptor */
	usb_strings_extended,
	ARRAY_SIZE(usb_strings_extended),/* cnt string descriptors in list */
};

/*----------------------------------------------------------------------------
 *        Functions
 *----------------------------------------------------------------------------*/

void SIMtrace_USB_Initialize(void)
{
	unsigned int i;
	/* Signal USB reset by disabling the pull-up on USB D+ for at least 10 ms */
#ifdef PIN_USB_PULLUP
	const Pin usb_dp_pullup = PIN_USB_PULLUP;
	PIO_Configure(&usb_dp_pullup, 1);
	PIO_Set(&usb_dp_pullup);
#endif
	USBD_HAL_Suspend();
	mdelay(20);
#ifdef PIN_USB_PULLUP
	PIO_Clear(&usb_dp_pullup);
#endif
	USBD_HAL_Activate();

	// Get std USB driver
	USBDDriver *pUsbd = USBD_GetDriver();

	// put device ID into USB serial number description
	unsigned int device_id[4];
	EEFC_ReadUniqueID(device_id);
	char device_id_string[32 + 1];
	snprintf(device_id_string, ARRAY_SIZE(device_id_string), "%08x%08x%08x%08x",
		device_id[0], device_id[1], device_id[2], device_id[3]);
	for (i = 0; i < ARRAY_SIZE(device_id_string) - 1; i++) {
		usb_string_serial[2 + 2 * i] = device_id_string[i];
	}

	// put version into USB string
	usb_string_version[0] = USBStringDescriptor_LENGTH(ARRAY_SIZE(git_version) - 1);
	usb_string_version[1] = USBGenericDescriptor_STRING;
	for (i = 0; i < ARRAY_SIZE(git_version) - 1; i++) {
		usb_string_version[2 + i * 2 + 0] = git_version[i];
		usb_string_version[2 + i * 2 + 1] = 0;
	}

	// fill extended USB strings
	for (i = 0; i < ARRAY_SIZE(usb_strings) && i < ARRAY_SIZE(usb_strings_extended); i++) {
		usb_strings_extended[i] = usb_strings[i];
	}
	usb_strings_extended[SERIAL_STR] = usb_string_serial;
	usb_strings_extended[VERSION_CONF_STR] = usb_string_version_conf;
	usb_strings_extended[VERSION_STR] = usb_string_version;

	// Initialize standard USB driver
	USBDDriver_Initialize(pUsbd, &driverDescriptors, 0);	// Multiple interface settings not supported
	USBD_Init();
	USBD_Connect();

	NVIC_EnableIRQ(UDP_IRQn);
}
