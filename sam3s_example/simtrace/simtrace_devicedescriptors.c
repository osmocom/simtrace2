/*------------------------------------------------------------------------------
 *       USB String descriptors 
 *------------------------------------------------------------------------------*/

const unsigned char productStringDescriptor[] = {

    USBStringDescriptor_LENGTH(8),
    USBGenericDescriptor_STRING,
    USBStringDescriptor_UNICODE('S'),
    USBStringDescriptor_UNICODE('I'),
    USBStringDescriptor_UNICODE('M'),
    USBStringDescriptor_UNICODE('t'),
    USBStringDescriptor_UNICODE('r'),
    USBStringDescriptor_UNICODE('a'),
    USBStringDescriptor_UNICODE('c'),
    USBStringDescriptor_UNICODE('e'),
};

const unsigned char snifferConfigStringDescriptor[] = {

    USBStringDescriptor_LENGTH(15),
    USBGenericDescriptor_STRING,
    USBStringDescriptor_UNICODE('S'),
    USBStringDescriptor_UNICODE('I'),
    USBStringDescriptor_UNICODE('M'),
    USBStringDescriptor_UNICODE('t'),
    USBStringDescriptor_UNICODE('r'),
    USBStringDescriptor_UNICODE('a'),
    USBStringDescriptor_UNICODE('c'),
    USBStringDescriptor_UNICODE('e'),
    USBStringDescriptor_UNICODE('S'),
    USBStringDescriptor_UNICODE('n'),
    USBStringDescriptor_UNICODE('i'),
    USBStringDescriptor_UNICODE('f'),
    USBStringDescriptor_UNICODE('f'),
    USBStringDescriptor_UNICODE('e'),
    USBStringDescriptor_UNICODE('r'),
};

const unsigned char CCIDConfigStringDescriptor[] = {

    USBStringDescriptor_LENGTH(12),
    USBGenericDescriptor_STRING,
    USBStringDescriptor_UNICODE('S'),
    USBStringDescriptor_UNICODE('I'),
    USBStringDescriptor_UNICODE('M'),
    USBStringDescriptor_UNICODE('t'),
    USBStringDescriptor_UNICODE('r'),
    USBStringDescriptor_UNICODE('a'),
    USBStringDescriptor_UNICODE('c'),
    USBStringDescriptor_UNICODE('e'),
    USBStringDescriptor_UNICODE('C'),
    USBStringDescriptor_UNICODE('C'),
    USBStringDescriptor_UNICODE('I'),
    USBStringDescriptor_UNICODE('D'),
};

const unsigned char phoneConfigStringDescriptor[] = {

    USBStringDescriptor_LENGTH(13),
    USBGenericDescriptor_STRING,
    USBStringDescriptor_UNICODE('S'),
    USBStringDescriptor_UNICODE('I'),
    USBStringDescriptor_UNICODE('M'),
    USBStringDescriptor_UNICODE('t'),
    USBStringDescriptor_UNICODE('r'),
    USBStringDescriptor_UNICODE('a'),
    USBStringDescriptor_UNICODE('c'),
    USBStringDescriptor_UNICODE('e'),
    USBStringDescriptor_UNICODE('P'),
    USBStringDescriptor_UNICODE('h'),
    USBStringDescriptor_UNICODE('o'),
    USBStringDescriptor_UNICODE('n'),
    USBStringDescriptor_UNICODE('e'),
};


const unsigned char MITMConfigStringDescriptor[] = {

    USBStringDescriptor_LENGTH(12),
    USBGenericDescriptor_STRING,
    USBStringDescriptor_UNICODE('S'),
    USBStringDescriptor_UNICODE('I'),
    USBStringDescriptor_UNICODE('M'),
    USBStringDescriptor_UNICODE('t'),
    USBStringDescriptor_UNICODE('r'),
    USBStringDescriptor_UNICODE('a'),
    USBStringDescriptor_UNICODE('c'),
    USBStringDescriptor_UNICODE('e'),
    USBStringDescriptor_UNICODE('M'),
    USBStringDescriptor_UNICODE('I'),
    USBStringDescriptor_UNICODE('T'),
    USBStringDescriptor_UNICODE('M'),
};

enum strDescNum {
    NONE = 1, PRODUCT_STRING, SNIFFER_CONF_STR, CCID_CONF_STR, PHONE_CONF_STR, MITM_CONF_STR, STRING_DESC_CNT
};

/** List of string descriptors used by the device */
const unsigned char *stringDescriptors[] = {
/* FIXME: Is it true that I can't use the string desc #0, 
 * because 0 also stands for "no string desc"? 
 * on the other hand, dmesg output: 
 * "string descriptor 0 malformed (err = -61), defaulting to 0x0409" */
    0,
    productStringDescriptor,
    snifferConfigStringDescriptor,
    CCIDreaderConfigStringDescriptor,
    phoneConfigStringDescriptor,
    MITMConfigStringDescriptor
};

/* Endpoint numbers */
#define DATAOUT     1
#define DATAIN      2

/*------------------------------------------------------------------------------
 *       USB Device descriptors 
 *------------------------------------------------------------------------------*/

typedef struct _SIMTraceDriverConfigurationDescriptorSniffer {

    /** Standard configuration descriptor. */
    USBConfigurationDescriptor configuration;
    USBInterfaceDescriptor sniffer;
    USBEndpointDescriptor sniffer_dataOut;
    USBEndpointDescriptor sniffer_dataIn;

} __attribute__ ((packed)) SIMTraceDriverConfigurationDescriptorSniffer;

const SIMTraceDriverConfigurationDescriptorSniffer configurationDescriptorSniffer = {
    /* Standard configuration descriptor */
    {
        sizeof(USBConfigurationDescriptor),
        USBGenericDescriptor_CONFIGURATION,
        sizeof(SIMTraceDriverConfigurationDescriptorSniffer),
        1, /* There is one interface in this configuration */
        1, /* This is configuration #1 */
        SNIFFER_CONF_STR, /* string descriptor for this configuration */
        USBD_BMATTRIBUTES,
        USBConfigurationDescriptor_POWER(100)
    },
    /* Communication class interface standard descriptor */
    {
        sizeof(USBInterfaceDescriptor),
        USBGenericDescriptor_INTERFACE,
        0, /* This is interface #0 */
        0, /* This is alternate setting #0 for this interface */
        2, /* This interface uses 2 endpoints */
        0xff, /* Descriptor Class: Vendor specific */
        0, /* No subclass */
        0, /* No l */
        SNIFFER_CONF_STR  /* Third in string descriptor for this interface */
    },
    /* Bulk-OUT endpoint standard descriptor */
    {
        sizeof(USBEndpointDescriptor),
        USBGenericDescriptor_ENDPOINT,
        USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_OUT,
                                      DATAOUT),
        USBEndpointDescriptor_BULK,
        MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(DATAOUT),
            USBEndpointDescriptor_MAXBULKSIZE_FS),
        0 /* Must be 0 for full-speed bulk endpoints */
    },
    /* Bulk-IN endpoint descriptor */
    {
        sizeof(USBEndpointDescriptor),
        USBGenericDescriptor_ENDPOINT,
        USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN,
                                      DATAIN),
        USBEndpointDescriptor_BULK,
        MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(DATAIN),
            USBEndpointDescriptor_MAXBULKSIZE_FS),
        0 /* Must be 0 for full-speed bulk endpoints */
    }
};

/* FIXME: CCID descriptor: External C file */
typedef struct {

    USBConfigurationDescriptor configuration;
    USBInterfaceDescriptor     interface;
    CCIDDescriptor             ccid;
    USBEndpointDescriptor      bulkOut;
    USBEndpointDescriptor      bulkIn;
    USBEndpointDescriptor      interruptIn;
} __attribute__ ((packed)) CCIDDriverConfigurationDescriptorsCCID;

const CCIDDriverConfigurationDescriptorsCCID configurationDescriptorCCID = { 0 };

/* SIM card emulator */
typedef struct _SIMTraceDriverConfigurationDescriptorPhone {

    /** Standard configuration descriptor. */
    USBConfigurationDescriptor configuration;
    USBInterfaceDescriptor sniffer;
    USBEndpointDescriptor sniffer_dataOut;
    USBEndpointDescriptor sniffer_dataIn;

} __attribute__ ((packed)) SIMTraceDriverConfigurationDescriptorPhone;

const SIMTraceDriverConfigurationDescriptorPhone configurationDescriptorPhone = {
    /* Standard configuration descriptor */
    {
        sizeof(USBConfigurationDescriptor),
        USBGenericDescriptor_CONFIGURATION,
        sizeof(SIMTraceDriverConfigurationDescriptorSniffer),
        1, /* There is one interface in this configuration */
        1, /* This is configuration #1 */
        PHONE_CONF_STR, /* string descriptor for this configuration */
        USBD_BMATTRIBUTES,
        USBConfigurationDescriptor_POWER(100)
    },
    /* Communication class interface standard descriptor */
    {
        sizeof(USBInterfaceDescriptor),
        USBGenericDescriptor_INTERFACE,
        0, /* This is interface #0 */
        0, /* This is alternate setting #0 for this interface */
        2, /* This interface uses 2 endpoints */
        0xff, /* Descriptor Class: Vendor specific */
        0, /* No subclass */
        0, /* No l */
        PHONE_CONF_STR  /* Third in string descriptor for this interface */
    },
    /* Bulk-OUT endpoint standard descriptor */
    {
        sizeof(USBEndpointDescriptor),
        USBGenericDescriptor_ENDPOINT,
        USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_OUT,
                                      DATAOUT),
        USBEndpointDescriptor_BULK,
        MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(DATAOUT),
            USBEndpointDescriptor_MAXBULKSIZE_FS),
        0 /* Must be 0 for full-speed bulk endpoints */
    },
    /* Bulk-IN endpoint descriptor */
    {
        sizeof(USBEndpointDescriptor),
        USBGenericDescriptor_ENDPOINT,
        USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN,
                                      DATAIN),
        USBEndpointDescriptor_BULK,
        MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(DATAIN),
            USBEndpointDescriptor_MAXBULKSIZE_FS),
        0 /* Must be 0 for full-speed bulk endpoints */
    }
};


typedef struct _SIMTraceDriverConfigurationDescriptorMITM {

    /** Standard configuration descriptor. */
    USBConfigurationDescriptor configuration;
    USBInterfaceDescriptor simcard;
    USBEndpointDescriptor simcard_dataOut;
    USBEndpointDescriptor simcard_dataIn;
    USBInterfaceDescriptor phone;
    USBEndpointDescriptor phone_dataOut;
    USBEndpointDescriptor phone_dataIn;

} __attribute__ ((packed)) SIMTraceDriverConfigurationDescriptorMITM;

const SIMTraceDriverConfigurationDescriptorMITM configurationDescriptorsMITM = {
    /* Standard configuration descriptor */
    {
        sizeof(USBConfigurationDescriptor),
        USBGenericDescriptor_CONFIGURATION,
        sizeof(SIMTraceDriverConfigurationDescriptor),
        2, /* There are two interfaces in this configuration */
        4, /* This is configuration #4 */
        MITM_CONF_STR, /* string descriptor for this configuration */
        USBD_BMATTRIBUTES,
        USBConfigurationDescriptor_POWER(100)
    },
    /* Communication class interface standard descriptor */
    {
        sizeof(USBInterfaceDescriptor),
        USBGenericDescriptor_INTERFACE,
        0, /* This is interface #0 */
        0, /* This is alternate setting #0 for this interface */
        2, /* This interface uses 2 endpoints */
        //CDCCommunicationInterfaceDescriptor_CLASS,
        0xff,
//        CDCCommunicationInterfaceDescriptor_ABSTRACTCONTROLMODEL,
        0,
//        CDCCommunicationInterfaceDescriptor_NOPROTOCOL,
        0, 
        MITM_CONF_STR  /* string descriptor for this interface */
    },
    /* Bulk-OUT endpoint standard descriptor */
    {
        sizeof(USBEndpointDescriptor),
        USBGenericDescriptor_ENDPOINT,
        USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_OUT,
                                      DATAOUT),
        USBEndpointDescriptor_BULK,
        MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(DATAOUT),
            USBEndpointDescriptor_MAXBULKSIZE_FS),
        0 /* Must be 0 for full-speed bulk endpoints */
    },
    /* Bulk-IN endpoint descriptor */
    {
        sizeof(USBEndpointDescriptor),
        USBGenericDescriptor_ENDPOINT,
        USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN,
                                      DATAIN),
        USBEndpointDescriptor_BULK,
        MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(DATAIN),
            USBEndpointDescriptor_MAXBULKSIZE_FS),
        0 /* Must be 0 for full-speed bulk endpoints */
    }
    /* Communication class interface standard descriptor */
    {
        sizeof(USBInterfaceDescriptor),
        USBGenericDescriptor_INTERFACE,
        1, /* This is interface #1 */
        0, /* This is alternate setting #0 for this interface */
        2, /* This interface uses 2 endpoints */
        0xff,
        0,
        0, 
        0, /* FIXME: string descriptor for this interface */
    },
    /* Bulk-OUT endpoint standard descriptor */
    {
        sizeof(USBEndpointDescriptor),
        USBGenericDescriptor_ENDPOINT,
        USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_OUT,
                                      DATAOUT),
        USBEndpointDescriptor_BULK,
        MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(DATAOUT),
            USBEndpointDescriptor_MAXBULKSIZE_FS),
        0 /* Must be 0 for full-speed bulk endpoints */
    },
    /* Bulk-IN endpoint descriptor */
    {
        sizeof(USBEndpointDescriptor),
        USBGenericDescriptor_ENDPOINT,
        USBEndpointDescriptor_ADDRESS(USBEndpointDescriptor_IN,
                                      DATAIN),
        USBEndpointDescriptor_BULK,
        MIN(BOARD_USB_ENDPOINTS_MAXPACKETSIZE(DATAIN),
            USBEndpointDescriptor_MAXBULKSIZE_FS),
        0 /* Must be 0 for full-speed bulk endpoints */
    }
};


/** Standard USB device descriptor for the CDC serial driver */
const USBDeviceDescriptor deviceDescriptor = {

    sizeof(USBDeviceDescriptor),
    USBGenericDescriptor_DEVICE,
    USBDeviceDescriptor_USB2_00,
    0xff,
//    CDCDeviceDescriptor_CLASS,
    0xff,
//    CDCDeviceDescriptor_SUBCLASS,
    0xff,
//    CDCDeviceDescriptor_PROTOCOL,
    BOARD_USB_ENDPOINTS_MAXPACKETSIZE(0),
    ATMEL_VENDOR_ID,
    SIMTRACE_PRODUCT_ID,
    1, /* Release number */
    0, /* No string descriptor for manufacturer */
    PRODUCT_STRING, /* Index of product string descriptor */
    0, /* No string descriptor for serial number */
    4 /* Device has 4 possible configurations */
};

const SIMTraceDriverConfigurationDescriptor *configurationDescriptorsArr[] = {
    &configurationDescriptorSniffer,
    &configurationDescriptorCCID,
    &configurationDescriptorPhone,
    &configurationDescriptorMITM,
};


/* AT91SAM3S does only support full speed, but not high speed USB */
const USBDDriverDescriptors driverDescriptors = {
    &deviceDescriptor,
    (USBConfigurationDescriptor **) &(configurationDescriptorsArr),   /* first full-speed configuration descriptor */
    0, /* No full-speed device qualifier descriptor */
    0, /* No full-speed other speed configuration */
    0, /* No high-speed device descriptor */
    0, /* No high-speed configuration descriptor */
    0, /* No high-speed device qualifier descriptor */
    0, /* No high-speed other speed configuration descriptor */
    stringDescriptors,
    STRING_DESC_CNT-1 /* cnt string descriptors in list */
};

