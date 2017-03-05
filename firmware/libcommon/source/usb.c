/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2009, Atmel Corporation
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
#include "utils.h"

#include <cciddriverdescriptors.h>
#include <usb/common/dfu/usb_dfu.h>
#include <usb/device/dfu/dfu.h>

/*------------------------------------------------------------------------------
 *       USB String descriptors 
 *------------------------------------------------------------------------------*/
#include "usb_strings_generated.h"
enum strDescNum {
	PRODUCT_STRING = 1,
	MANUF_STR,
	SNIFFER_CONF_STR,
	CCID_CONF_STR,
	PHONE_CONF_STR,
	MITM_CONF_STR,
	CARDEM_USIM1_INTF_STR,
	CARDEM_USIM2_INTF_STR,
	STRING_DESC_CNT
};

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
	{
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
	{
		.bLength = sizeof(USBInterfaceDescriptor),
		.bDescriptorType 	= USBGenericDescriptor_INTERFACE,
		.bInterfaceNumber	= 0,
		.bAlternateSetting	= 0,
		.bNumEndpoints		= 3,
		.bInterfaceClass	= 0xff,
		.bInterfaceSubClass	= 0,
		.bInterfaceProtocol	= 0,
		.iInterface		= SNIFFER_CONF_STR,
	},
	/* Bulk-OUT endpoint standard descriptor */
	{
		.bLength 		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_OUT,
						PHONE_DATAOUT),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize	= MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(
							PHONE_DATAOUT),
				      USBEndpointDescriptor_MAXBULKSIZE_FS),
		.bInterval		= 0,
	},
	/* Bulk-IN endpoint descriptor */
	{
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_IN,
						PHONE_DATAIN),
		.bmAttributes		= USBEndpointDescriptor_BULK,
		.wMaxPacketSize = MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(
								PHONE_DATAIN),
				      USBEndpointDescriptor_MAXBULKSIZE_FS),
		.bInterval		= 0,
	},
	// Notification endpoint descriptor
	{
		.bLength		= sizeof(USBEndpointDescriptor),
		.bDescriptorType	= USBGenericDescriptor_ENDPOINT,
		.bEndpointAddress	= USBEndpointDescriptor_ADDRESS(
						USBEndpointDescriptor_IN,
						PHONE_INT),
		.bmAttributes		= USBEndpointDescriptor_INTERRUPT,
		.wMaxPacketSize	= MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(
								PHONE_INT),
				      USBEndpointDescriptor_MAXINTERRUPTSIZE_FS),
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
		.wMaxPacketSize	= MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(
					CCID_EPT_DATA_OUT),
		    			USBEndpointDescriptor_MAXBULKSIZE_FS),
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
		.wMaxPacketSize	= MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(
					CCID_EPT_DATA_IN),
		    			USBEndpointDescriptor_MAXBULKSIZE_FS),
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
		.wMaxPacketSize = MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(
					CCID_EPT_NOTIFICATION),
					USBEndpointDescriptor_MAXINTERRUPTSIZE_FS),
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
	{
		sizeof(USBConfigurationDescriptor),
		USBGenericDescriptor_CONFIGURATION,
		sizeof(SIMTraceDriverConfigurationDescriptorPhone),
#ifdef CARDEMU_SECOND_UART
		2+DFURT_NUM_IF,
#else
		1+DFURT_NUM_IF,	/* There is one interface in this configuration */
#endif
		CFG_NUM_PHONE,		/* configuration number */
		PHONE_CONF_STR,	/* string descriptor for this configuration */
		USBD_BMATTRIBUTES,
		USBConfigurationDescriptor_POWER(100)
	},
	/* Communication class interface standard descriptor */
	{
		sizeof(USBInterfaceDescriptor),
		USBGenericDescriptor_INTERFACE,
		0,	/* This is interface #0 */
		0,	/* This is alternate setting #0 for this interface */
		3,	/* Number of endpoints */
		0xff,	/* Descriptor Class: Vendor specific */
		0,	/* No subclass */
		0,	/* No l */
		CARDEM_USIM1_INTF_STR
	},
	/* Bulk-OUT endpoint standard descriptor */
	{
		sizeof(USBEndpointDescriptor),
		USBGenericDescriptor_ENDPOINT,
		USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_OUT,
					      PHONE_DATAOUT),
		USBEndpointDescriptor_BULK,
		MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(PHONE_DATAOUT),
		    USBEndpointDescriptor_MAXBULKSIZE_FS),
		0	/* Must be 0 for full-speed bulk endpoints */
	},
	/* Bulk-IN endpoint descriptor */
	{
		sizeof(USBEndpointDescriptor),
		USBGenericDescriptor_ENDPOINT,
		USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN,
					      PHONE_DATAIN),
		USBEndpointDescriptor_BULK,
		MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(PHONE_DATAIN),
		USBEndpointDescriptor_MAXBULKSIZE_FS),
		0	/* Must be 0 for full-speed bulk endpoints */
	},
	/* Notification endpoint descriptor */
	{
		sizeof(USBEndpointDescriptor),
		USBGenericDescriptor_ENDPOINT,
		USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN,
					      PHONE_INT),
		USBEndpointDescriptor_INTERRUPT,
		MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(PHONE_INT),
		USBEndpointDescriptor_MAXINTERRUPTSIZE_FS),
		0x10
	},
#ifdef CARDEMU_SECOND_UART
	/* Communication class interface standard descriptor */
	{
		sizeof(USBInterfaceDescriptor),
		USBGenericDescriptor_INTERFACE,
		1,	/* This is interface #1 */
		0,	/* This is alternate setting #0 for this interface */
		3,	/* Number of endpoints */
		0xff,	/* Descriptor Class: Vendor specific */
		0,	/* No subclass */
		0,	/* No l */
		CARDEM_USIM2_INTF_STR
	},
	/* Bulk-OUT endpoint standard descriptor */
	{
		sizeof(USBEndpointDescriptor),
		USBGenericDescriptor_ENDPOINT,
		USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_OUT,
					      CARDEM_USIM2_DATAOUT),
		USBEndpointDescriptor_BULK,
		MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CARDEM_USIM2_DATAOUT),
		    USBEndpointDescriptor_MAXBULKSIZE_FS),
		0	/* Must be 0 for full-speed bulk endpoints */
	}
	,
	/* Bulk-IN endpoint descriptor */
	{
		sizeof(USBEndpointDescriptor),
		USBGenericDescriptor_ENDPOINT,
		USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN,
					      CARDEM_USIM2_DATAIN),
		USBEndpointDescriptor_BULK,
		MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CARDEM_USIM2_DATAIN),
		    USBEndpointDescriptor_MAXBULKSIZE_FS),
		0	/* Must be 0 for full-speed bulk endpoints */
	},
	/* Notification endpoint descriptor */
	{
		sizeof(USBEndpointDescriptor),
		USBGenericDescriptor_ENDPOINT,
		USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN,
					      CARDEM_USIM2_INT),
		USBEndpointDescriptor_INTERRUPT,
		MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(CARDEM_USIM2_INT),
		    USBEndpointDescriptor_MAXINTERRUPTSIZE_FS),
		0x10
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
	{
		sizeof(USBConfigurationDescriptor),
		USBGenericDescriptor_CONFIGURATION,
		sizeof(SIMTraceDriverConfigurationDescriptorMITM),
		2+DFURT_NUM_IF,	/* There are two interfaces in this configuration */
		CFG_NUM_MITM,	/* configuration number */
		MITM_CONF_STR,	/* string descriptor for this configuration */
		USBD_BMATTRIBUTES,
		USBConfigurationDescriptor_POWER(100)
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
		.wMaxPacketSize	= MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(
					CCID_EPT_DATA_OUT),
		    			USBEndpointDescriptor_MAXBULKSIZE_FS),
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
		.wMaxPacketSize	= MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(
					CCID_EPT_DATA_IN),
		    			USBEndpointDescriptor_MAXBULKSIZE_FS),
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
		.wMaxPacketSize = MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(
					CCID_EPT_NOTIFICATION),
					USBEndpointDescriptor_MAXINTERRUPTSIZE_FS),
		.bInterval = 0x10,
	},

	/* Communication class interface standard descriptor */
	{
	 sizeof(USBInterfaceDescriptor),
	 USBGenericDescriptor_INTERFACE,
	 1,			/* This is interface #1 */
	 0,			/* This is alternate setting #0 for this interface */
	 3,			/* Number of endpoints */
	 0xff,
	 0,
	 0,
	 PHONE_CONF_STR,	/* string descriptor for this interface */
	 }
	,
	/* Bulk-OUT endpoint standard descriptor */
	{
	 sizeof(USBEndpointDescriptor),
	 USBGenericDescriptor_ENDPOINT,
	 USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_OUT,
				       PHONE_DATAOUT),
	 USBEndpointDescriptor_BULK,
	 MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(PHONE_DATAOUT),
	     USBEndpointDescriptor_MAXBULKSIZE_FS),
	 0			/* Must be 0 for full-speed bulk endpoints */
	 }
	,
	/* Bulk-IN endpoint descriptor */
	{
	 sizeof(USBEndpointDescriptor),
	 USBGenericDescriptor_ENDPOINT,
	 USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN,
				       PHONE_DATAIN),
	 USBEndpointDescriptor_BULK,
	 MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(PHONE_DATAIN),
	     USBEndpointDescriptor_MAXBULKSIZE_FS),
	 0			/* Must be 0 for full-speed bulk endpoints */
	 }
	,
	/* Notification endpoint descriptor */
	{
	 sizeof(USBEndpointDescriptor),
	 USBGenericDescriptor_ENDPOINT,
	 USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN, PHONE_INT),
	 USBEndpointDescriptor_INTERRUPT,
	 MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(PHONE_INT),
	     USBEndpointDescriptor_MAXINTERRUPTSIZE_FS),
	 0x10},
	DFURT_IF_DESCRIPTOR(2, 0),
};
#endif /* HAVE_CARDEM */

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
};

/** Standard USB device descriptor for the CDC serial driver */
const USBDeviceDescriptor deviceDescriptor = {
	.bLength		= sizeof(USBDeviceDescriptor),
	.bDescriptorType	= USBGenericDescriptor_DEVICE,
	.bcdUSB			= USBDeviceDescriptor_USB2_00,
	.bDeviceClass		= 0xff,
	.bDeviceSubClass	= 0xff,
	.bDeviceProtocol	= 0xff,
	.bMaxPacketSize0	= BOARD_USB_ENDPOINTS_MAXPACKETSIZE(0),
	.idVendor		= SIMTRACE_VENDOR_ID,
	.idProduct		= SIMTRACE_PRODUCT_ID,
	.bcdDevice		= 2,	/* Release number */
	.iManufacturer		= MANUF_STR,
	.iProduct		= PRODUCT_STRING,
	.iSerialNumber		= 0,
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
	usb_strings,
	ARRAY_SIZE(usb_strings),/* cnt string descriptors in list */
};

/*----------------------------------------------------------------------------
 *        Functions
 *----------------------------------------------------------------------------*/

void SIMtrace_USB_Initialize(void)
{
	// Get std USB driver
	USBDDriver *pUsbd = USBD_GetDriver();

	TRACE_DEBUG(".");

	// Initialize standard USB driver
	USBDDriver_Initialize(pUsbd, &driverDescriptors, 0);	// Multiple interface settings not supported
	USBD_Init();
	USBD_Connect();

	NVIC_EnableIRQ(UDP_IRQn);
}
