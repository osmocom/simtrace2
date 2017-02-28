#ifndef _USB_DEV_DFU_H
#define _USB_DEV_DFU_H

#include <stdint.h>
#include <board.h>
#include <usb/include/USBDescriptors.h>
#include <usb/include/USBDDriver.h>

#if 0
/* This is valid for CCID */
#define CONFIG_DFU_NUM_APP_IF	1
#define CONFIG_DFU_NUM_APP_STR	4
#else
/* This is valid for CDC-Serial */
#define CONFIG_DFU_NUM_APP_IF	2
#define CONFIG_DFU_NUM_APP_STR	2
#endif

struct USBStringDescriptor {
	USBGenericDescriptor hdr;
	unsigned short wData[];
} __attribute__((packed));


#ifdef BOARD_USB_DFU

#include <usb/common/dfu/usb_dfu.h>

/* for board-specific config */
#include <board.h>

struct dfu_desc {
	USBConfigurationDescriptor ucfg;
	USBInterfaceDescriptor uif[BOARD_DFU_NUM_IF];
	struct usb_dfu_func_descriptor func_dfu;
} __attribute__ ((packed));

/* USB DFU functional descriptor */
#define DFU_FUNC_DESC  {						\
	.bLength		= USB_DT_DFU_SIZE,			\
	.bDescriptorType	= USB_DT_DFU,				\
	.bmAttributes		= USB_DFU_CAN_UPLOAD | USB_DFU_CAN_DOWNLOAD, \
	.wDetachTimeOut		= 0xff00,				\
	.wTransferSize		= BOARD_DFU_PAGE_SIZE,			\
	.bcdDFUVersion		= 0x0100,				\
}

/* Number of DFU interface during runtime mode */
#define DFURT_NUM_IF		1

/* to be used by the runtime as part of its USB descriptor structure
 * declaration */
#define DFURT_IF_DESCRIPTOR_STRUCT		\
	USBInterfaceDescriptor	dfu_rt;		\
	struct usb_dfu_func_descriptor func_dfu;

/* to be used by the runtime as part of its USB Dsecriptor structure
 * definition */
#define DFURT_IF_DESCRIPTOR(dfuIF, dfuSTR)					\
	.dfu_rt = {								\
		.bLength 		= sizeof(USBInterfaceDescriptor),	\
		.bDescriptorType	= USBGenericDescriptor_INTERFACE,	\
		.bInterfaceNumber	= dfuIF,				\
		.bAlternateSetting	= 0,					\
		.bNumEndpoints		= 0,					\
		.bInterfaceClass	= 0xFE,					\
		.bInterfaceSubClass	= 0x01,					\
		.bInterfaceProtocol	= 0x01,					\
		.iInterface		= dfuSTR,				\
	},									\
	.func_dfu = DFU_FUNC_DESC						\

/* provided by dfu_desc.c */
extern const struct dfu_desc dfu_cfg_descriptor;
extern const USBDDriverDescriptors dfu_descriptors;

#else /* BOARD_USB_DFU */

/* no DFU bootloader is being used */
#define DFURT_NUM_IF	0
#define DFURT_IF_DESCRIPTOR_STRUCT(a, b)
#define DFURT_IF_DESCRIPTOR

#endif /* BOARD_USB_DFU */

/* magic value we use during boot to detect if we should start in DFU
 * mode or runtime mode */
#define USB_DFU_MAGIC	0xDFDFDFDF
/* RAM address for this magic value above */
#define USB_DFU_MAGIC_ADDR	IRAM_ADDR

/* The API between the core DFU handler and the board/soc specific code */

struct dfudata {
	uint8_t status;
	uint32_t state;
	int past_manifest;
	unsigned int total_bytes;
};

extern struct dfudata *g_dfu;

void set_usb_serial_str(const uint8_t *serial_usbstr);

void DFURT_SwitchToDFU(void);

/* call-backs from DFU USB function driver to the board/SOC */
extern int USBDFU_handle_dnload(uint8_t altif, unsigned int offset,
				uint8_t *data, unsigned int len);
extern int USBDFU_handle_upload(uint8_t altif, unsigned int offset,
				uint8_t *data, unsigned int req_len);

/* function to be called at end of EP0 handler during runtime */
void USBDFU_Runtime_RequestHandler(const USBGenericRequest *request);

/* function to be called at end of EP0 handler during DFU mode */
void USBDFU_DFU_RequestHandler(const USBGenericRequest *request);

/* initialization of USB DFU driver (in DFU mode */
void USBDFU_Initialize(const USBDDriverDescriptors *pDescriptors);

/* USBD tells us to switch from DFU mode to application mode */
void USBDFU_SwitchToApp(void);

/* Return values to be used by USBDFU_handle_{dn,up}load */
#define DFU_RET_NOTHING	0
#define DFU_RET_ZLP	1
#define DFU_RET_STALL	2

#endif
