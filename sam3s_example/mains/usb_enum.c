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
#include "CDCDSerialDriver.h"

/*----------------------------------------------------------------------------
 *      Internal variables
 *----------------------------------------------------------------------------*/

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
    1, /* Index of product string descriptor is #1 */
    0, /* No string descriptor for serial number */
    1 /* Device has 1 possible configuration */
};

typedef struct _SIMTraceDriverConfigurationDescriptors {

    /** Standard configuration descriptor. */
    USBConfigurationDescriptor configuration;
    /** Data interface descriptor. */
    USBInterfaceDescriptor data;
    /** Data OUT endpoint descriptor. */
    USBEndpointDescriptor dataOut;
    /** Data IN endpoint descriptor. */
    USBEndpointDescriptor dataIn;

} __attribute__ ((packed)) SIMTraceDriverConfigurationDescriptor;

/** Standard USB configuration descriptor for the CDC serial driver */
const SIMTraceDriverConfigurationDescriptor configurationDescriptorsFS = {

    /* Standard configuration descriptor */
    {
        sizeof(USBConfigurationDescriptor),
        USBGenericDescriptor_CONFIGURATION,
        sizeof(SIMTraceDriverConfigurationDescriptor),
        2, /* There are two interfaces in this configuration */
        2, /* This is configuration #1 */
        2, /* Second string descriptor for this configuration */
        USBD_BMATTRIBUTES,
        USBConfigurationDescriptor_POWER(100)
    },
    /* Communication class interface standard descriptor */
    {
        sizeof(USBInterfaceDescriptor),
        USBGenericDescriptor_INTERFACE,
        0, /* This is interface #0 */
        0, /* This is alternate setting #0 for this interface */
        2, /* This interface uses 1 endpoint */
        //CDCCommunicationInterfaceDescriptor_CLASS,
        0xff,
//        CDCCommunicationInterfaceDescriptor_ABSTRACTCONTROLMODEL,
        0,
//        CDCCommunicationInterfaceDescriptor_NOPROTOCOL,
        0,
        1  /* Second in string descriptor for this interface */
    },
    /* Bulk-OUT endpoint standard descriptor */
#define DATAOUT     1
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
#define DATAIN      2
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

const unsigned char someString[] = {
    USBStringDescriptor_LENGTH(4),
    USBGenericDescriptor_STRING,
    USBStringDescriptor_UNICODE('S'),
    USBStringDescriptor_UNICODE('O'),
    USBStringDescriptor_UNICODE('M'),
    USBStringDescriptor_UNICODE('E'),
};

const unsigned char productStringDescriptor[] = {

    USBStringDescriptor_LENGTH(13),
    USBGenericDescriptor_STRING,
    USBStringDescriptor_UNICODE('S'),
    USBStringDescriptor_UNICODE('I'),
    USBStringDescriptor_UNICODE('M'),
    USBStringDescriptor_UNICODE('T'),
    USBStringDescriptor_UNICODE('R'),
    USBStringDescriptor_UNICODE('A'),
    USBStringDescriptor_UNICODE('C'),
    USBStringDescriptor_UNICODE('E'),
    USBStringDescriptor_UNICODE('~'),
    USBStringDescriptor_UNICODE('~'),
    USBStringDescriptor_UNICODE('~'),
    USBStringDescriptor_UNICODE('~'),
    USBStringDescriptor_UNICODE('~')
};

/** List of string descriptors used by the device */
const unsigned char *stringDescriptors[] = {
/* FIXME: Is it true that I can't use the string desc #0, 
 * because 0 also stands for "no string desc"? */
    0,
    productStringDescriptor,
    someString,
};

const USBDDriverDescriptors driverDescriptors = {

    &deviceDescriptor,
    (USBConfigurationDescriptor *) &(configurationDescriptorsFS),   /* full-speed configuration descriptor */
    0, /* No full-speed device qualifier descriptor */
    0, /* No full-speed other speed configuration */
    0, /* No high-speed device descriptor */
    0, /* No high-speed configuration descriptor */
    0, /* No high-speed device qualifier descriptor */
    0, /* No high-speed other speed configuration descriptor */
    stringDescriptors,
    3 /* 3 string descriptors in list */
};



/**
 * \brief Configure 48MHz Clock for USB
 */
static void _ConfigureUsbClock(void)
{
    /* Enable PLLB for USB */
// FIXME: are these the dividers I actually need?
// FIXME: I could just use PLLA, since it has a frequ of 48Mhz anyways?
    PMC->CKGR_PLLBR = CKGR_PLLBR_DIVB(5)
                    | CKGR_PLLBR_MULB(0xc)  /* MULT+1=0xd*/
                    | CKGR_PLLBR_PLLBCOUNT_Msk;
    while((PMC->PMC_SR & PMC_SR_LOCKB) == 0);
    /* USB Clock uses PLLB */
    PMC->PMC_USB = PMC_USB_USBDIV(0)       /* /1 (no divider)  */
                 | PMC_USB_USBS;           /* PLLB */
}


void SIMtraceDriver_Initialize( void )
{
    // Get std USB driver
    USBDDriver *pUsbd = USBD_GetDriver();

    TRACE_DEBUG(".");

    // Initialize standard USB driver
    USBDDriver_Initialize(pUsbd,
                          &driverDescriptors,
// FIXME: 2 interface settings supported in MITM mode
                          0); // Multiple interface settings not supported
    
    USBD_Init();
    TRACE_DEBUG("%s", "leaving");
}

/*
void USBDDriverCallbacks_ConfigurationChanged(unsigned char cfgnum)
{
    printf("Configuration change requested: %c\n\r", cfgnum);
}

void USBDCallbacks_RequestReceived(const USBGenericRequest *request)
{
    printf("RequestHandler called %d\n\r", USBGenericRequest_GetType(request));
}
*/
/*----------------------------------------------------------------------------
 *          Main
 *----------------------------------------------------------------------------*/

/**
 * \brief usb_cdc_serial Application entry point.
 *
 * Initializes drivers and start the USB <-> Serial bridge.
 */
int main(void)
{
    uint8_t isUsbConnected = 0;

    LED_Configure(LED_NUM_GREEN);
    LED_Set(LED_NUM_GREEN);

    /* Disable watchdog */
    WDT_Disable( WDT );

    PIO_InitializeInterrupts(0);
    
//    NVIC_EnableIRQ(UDP_IRQn);

    /* Enable UPLL for USB */
//    _ConfigureUsbClock();

    /* CDC serial driver initialization */
//    CDCDSerialDriver_Initialize(&driverDescriptors);
    SIMtraceDriver_Initialize();
//    CCIDDriver_Initialize();

    USBD_Connect();

    NVIC_EnableIRQ( UDP_IRQn );

    printf("**** Start");

    while (1) {
            /* Device is not configured */
        if (USBD_GetState() < USBD_STATE_CONFIGURED) {

            if (isUsbConnected) {
                isUsbConnected = 0;
//                TC_Stop(TC0, 0);
            }
        }
        else if (isUsbConnected == 0) {
            printf("USB is now configured\n\r");

            isUsbConnected = 1;
//            TC_Start(TC0, 0);
        }    
    }
}
