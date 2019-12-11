/* DFU related USB Descriptors */
/* (C) 2006-2017 Harald Welte <hwelte@hmw-consulting.de> */

#include <unistd.h>

#include "board.h"
#include "utils.h"

#include <usb/include/USBDescriptors.h>

#include <usb/include/USBDDriver.h>

#include <usb/common/dfu/usb_dfu.h>
#include <usb/device/dfu/dfu.h>

#include "usb_strings_generated.h"

enum {
	STR_MANUF	= 1,
	STR_PROD,
	STR_CONFIG,
	// strings for the first alternate interface (e.g. DFU)
	_STR_FIRST_ALT,
	// serial string
	STR_SERIAL	= (_STR_FIRST_ALT + BOARD_DFU_NUM_IF),
	// version string (on additional interface)
	VERSION_CONF_STR,
	VERSION_STR,
	// count
	STRING_DESC_CNT,
};

/* string used to replace one of both DFU flash partition atlsettings */
static const unsigned char usb_string_notavailable[] = {
	USBStringDescriptor_LENGTH(13),
	USBGenericDescriptor_STRING,
	USBStringDescriptor_UNICODE('n'),
	USBStringDescriptor_UNICODE('o'),
	USBStringDescriptor_UNICODE('t'),
	USBStringDescriptor_UNICODE(' '),
	USBStringDescriptor_UNICODE('a'),
	USBStringDescriptor_UNICODE('v'),
	USBStringDescriptor_UNICODE('a'),
	USBStringDescriptor_UNICODE('i'),
	USBStringDescriptor_UNICODE('l'),
	USBStringDescriptor_UNICODE('a'),
	USBStringDescriptor_UNICODE('b'),
	USBStringDescriptor_UNICODE('l'),
	USBStringDescriptor_UNICODE('e'),
};

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
/** array of static (from usb_strings) and runtime (serial, version) USB strings */
static const unsigned char *usb_strings_extended[STRING_DESC_CNT];

static const USBDeviceDescriptor fsDevice = {
	.bLength = 		sizeof(USBDeviceDescriptor),
	.bDescriptorType = 	USBGenericDescriptor_DEVICE,
	.bcdUSB = 		USBDeviceDescriptor_USB2_00,
	.bDeviceClass =		0,
	.bDeviceSubClass = 	0,
	.bDeviceProtocol = 	0,
	.bMaxPacketSize0 = 	USBEndpointDescriptor_MAXCTRLSIZE_FS,
	.idVendor =		BOARD_USB_VENDOR_ID,
	.idProduct = 		BOARD_DFU_USB_PRODUCT_ID,
	.bcdDevice = 		BOARD_USB_RELEASE,
	.iManufacturer =	STR_MANUF,
	.iProduct =		STR_PROD,
	.iSerialNumber =	STR_SERIAL,
	.bNumConfigurations =	2, // DFU + version configurations
};

/* Alternate Interface Descriptor, we use one per partition/memory type */
#define DFU_IF(ALT)								\
	{									\
		.bLength =		sizeof(USBInterfaceDescriptor),		\
		.bDescriptorType = 	USBGenericDescriptor_INTERFACE,		\
		.bInterfaceNumber =	0,					\
		.bAlternateSetting =	ALT,					\
		.bNumEndpoints =	0,					\
		.bInterfaceClass =	0xfe,					\
		.bInterfaceSubClass =	1,					\
		.iInterface =		(_STR_FIRST_ALT + ALT),			\
		.bInterfaceProtocol =	2,					\
	}

/* overall descriptor for the DFU configuration, including all
 * descriptors for alternate interfaces */
const struct dfu_desc dfu_cfg_descriptor = {
	.ucfg = {
		.bLength =	   sizeof(USBConfigurationDescriptor),
		.bDescriptorType = USBGenericDescriptor_CONFIGURATION,
		.wTotalLength =	   sizeof(struct dfu_desc),
		.bNumInterfaces =  1,
		.bConfigurationValue = 1,
		.iConfiguration =  STR_CONFIG,
		.bmAttributes =	   BOARD_USB_BMATTRIBUTES,
		.bMaxPower =	   100,
	},
	.uif[0] = DFU_IF(0),
#if BOARD_DFU_NUM_IF > 1
	.uif[1] = DFU_IF(1),
#endif
#if BOARD_DFU_NUM_IF > 2
	.uif[2] = DFU_IF(2),
#endif
#if BOARD_DFU_NUM_IF > 3
	.uif[3] = DFU_IF(3),
#endif
#if BOARD_DFU_NUM_IF > 4
	.uif[4] = DFU_IF(4),
#endif
	.func_dfu = DFU_FUNC_DESC
};

void set_usb_serial_str(void)
{
	unsigned int i;

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
#if defined(ENVIRONMENT_dfu)
	usb_strings_extended[_STR_FIRST_ALT + 1] = usb_string_notavailable;
#elif defined(ENVIRONMENT_flash)
	usb_strings_extended[_STR_FIRST_ALT + 2] = usb_string_notavailable;
#endif
	usb_strings_extended[STR_SERIAL] = usb_string_serial;
	usb_strings_extended[VERSION_CONF_STR] = usb_string_version_conf;
	usb_strings_extended[VERSION_STR] = usb_string_version;
}

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
		.bConfigurationValue	= 2,
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

static const USBConfigurationDescriptor *conf_desc_arr[] = {
	&dfu_cfg_descriptor.ucfg,
	&configurationDescriptorVersion.configuration,
};

const USBDDriverDescriptors dfu_descriptors = {
	.pFsDevice = &fsDevice,
	.pFsConfiguration = (const USBConfigurationDescriptor **)&conf_desc_arr,
	/* DFU only supports FS for now */
	.pFsQualifier = NULL,
	.pFsOtherSpeed = NULL,
	.pHsDevice = NULL,
	.pHsConfiguration = NULL,
	.pHsQualifier = NULL,
	.pHsOtherSpeed = NULL,
	.pStrings = usb_strings_extended,
	.numStrings = ARRAY_SIZE(usb_strings_extended),
};
