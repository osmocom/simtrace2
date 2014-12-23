/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support 
 * ----------------------------------------------------------------------------
 * Copyright (c) 2008, Atmel Corporation
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
/** \file */
/** \addtogroup usbd_composite_cdccdc
 *@{
 */
/*---------------------------------------------------------------------------
 *      Headers
 *---------------------------------------------------------------------------*/

/*  GENERAL */
#include <USBLib_Trace.h>

/*  USB */
#include <USBD.h>
#include <USBD_HAL.h>
#include <USBDDriver.h>

/* - DUALCDC */
#include <DUALCDCDDriver.h>

/*---------------------------------------------------------------------------
 *         Defines
 *---------------------------------------------------------------------------*/

/** Number of CDC serial ports */
#define NUM_PORTS       2

/** Interface setting spaces (4 byte aligned) */
#define NUM_INTERFACES  ((DUALCDCDDriverDescriptors_NUMINTERFACE+3)&0xFC)

/*---------------------------------------------------------------------------
 *         Types
 *---------------------------------------------------------------------------*/

/** Dual-CDC-Serial device driver struct */
typedef struct _DualCdcdSerialDriver {
    /** CDC Serial Port List */
    CDCDSerialPort cdcdSerialPort[NUM_PORTS];
} DualCdcdSerialDriver;

/*---------------------------------------------------------------------------
 *         Internal variables
 *---------------------------------------------------------------------------*/

/** Dual CDC Serial device driver instance */
DualCdcdSerialDriver dualcdcdDriver;

/*---------------------------------------------------------------------------
 *         Internal functions
 *---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 *         Exported functions
 *---------------------------------------------------------------------------*/

/**
 * Initializes the USB device composite device driver.
 * \param  pDescriptors Pointer to Descriptors list for CDC Serial Device.
 */
void DUALCDCDDriver_Initialize(const USBDDriverDescriptors *pDescriptors)
{
    USBDDriver *pUsbd = USBD_GetDriver();
    CDCDSerialPort *pCdcd = &dualcdcdDriver.cdcdSerialPort[0];

    TRACE_INFO("DUALCDCDDriver_Initialize\n\r");

    pCdcd = &dualcdcdDriver.cdcdSerialPort[0];
    CDCDSerialPort_Initialize(pCdcd, pUsbd,
                              0,
                              0,
                              DUALCDCDDriverDescriptors_INTERFACENUM0, 2);

    pCdcd = &dualcdcdDriver.cdcdSerialPort[1];
    CDCDSerialPort_Initialize(pCdcd, pUsbd,
                              0,
                              0,
                              DUALCDCDDriverDescriptors_INTERFACENUM1, 2);

    /*  Initialize the standard USB driver */
    USBDDriver_Initialize(pUsbd,
                          pDescriptors,
                          0);
    /*  Initialize the USB driver */
    USBD_Init();
}

/**
 * Invoked whenever the active configuration of device is changed by the
 * host.
 * \param cfgnum Configuration number.
 */
void DUALCDCDDriver_ConfigurationChangeHandler(uint8_t cfgnum)
{
    CDCDSerialPort *pCdcd = &dualcdcdDriver.cdcdSerialPort[0];
    USBDDriver *pUsbd = pCdcd->pUsbd;
    USBConfigurationDescriptor *pDesc;
    USBGenericDescriptor *pD;
    uint32_t i, len;

    if (cfgnum > 0) {

        /* Parse endpoints for data & notification */
        pDesc = USBDDriver_GetCfgDescriptors(pUsbd, cfgnum);

        pD = (USBGenericDescriptor *)pDesc;
        len = pDesc->wTotalLength;

        for (i = 0; i < NUM_PORTS; i ++) {
            pCdcd = &dualcdcdDriver.cdcdSerialPort[i];
            pD = CDCDSerialPort_ParseInterfaces(pCdcd, pD, len);
            len = pDesc->wTotalLength - ((uint32_t)pD - (uint32_t)pDesc);
        }
    }
}


/**
 * Handles composite-specific USB requests sent by the host, and forwards
 * standard ones to the USB device driver.
 * \param request Pointer to a USBGenericRequest instance.
 */
void DUALCDCDDriver_RequestHandler(const USBGenericRequest *request)
{
    CDCDSerialPort *pCdcd = 0;
    USBDDriver *pUsbd = 0;
    uint32_t rc, i;

    TRACE_INFO_WP("NewReq ");

    for (i = 0; i < NUM_PORTS; i ++) {
        pCdcd = &dualcdcdDriver.cdcdSerialPort[i];
        rc = CDCDSerialPort_RequestHandler(pCdcd, request);
        if (rc == USBRC_SUCCESS)
            break;
    }

    /* Not handled by CDC Serial */
    if (rc != USBRC_SUCCESS) {
        if (USBGenericRequest_GetType(request) == USBGenericRequest_STANDARD) {
            pUsbd = pCdcd->pUsbd;
            USBDDriver_RequestHandler(pUsbd, request);
        }
        else {
            TRACE_WARNING(
              "DUALCDCDDriver_RequestHandler: Unsupported request (%d,%d)\n\r",
              USBGenericRequest_GetType(request),
              USBGenericRequest_GetRequest(request));
            USBD_Stall(0);
        }
    }

}

/**
 * Return CDCDSerialPort for serial port operations.
 * \param port Port number.
 */
CDCDSerialPort *DUALCDCDDriver_GetSerialPort(uint32_t port)
{
    if (port < NUM_PORTS)
        return &dualcdcdDriver.cdcdSerialPort[port];

    return 0;
}

/**@}*/

