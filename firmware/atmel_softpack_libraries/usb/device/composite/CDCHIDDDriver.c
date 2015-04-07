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

/** \addtogroup usbd_composite_cdchid
 *@{
 */
/*---------------------------------------------------------------------------
 *      Headers
 *---------------------------------------------------------------------------*/

#include <USBLib_Trace.h>

#include <CDCHIDDDriver.h>
#include <CDCDSerial.h>
#include <HIDDKeyboard.h>

/*---------------------------------------------------------------------------
 *         Defines
 *---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 *         Types
 *---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 *         Internal variables
 *---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 *         Internal functions
 *---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 *         Exported functions
 *---------------------------------------------------------------------------*/

/**
 * Initializes the USB device composite device driver.
 */
void CDCHIDDDriver_Initialize(const USBDDriverDescriptors *pDescriptors)
{
    USBDDriver *pUsbd = USBD_GetDriver();

    /* Initialize the standard USB driver */
    USBDDriver_Initialize(pUsbd,
                          pDescriptors,
                          0);

    /* CDC */
    CDCDSerial_Initialize(pUsbd, CDCHIDDDriverDescriptors_CDC_INTERFACE);
    /* HID */
    HIDDKeyboard_Initialize(pUsbd, CDCHIDDDriverDescriptors_HID_INTERFACE);

    /* Initialize the USB driver */
    USBD_Init();
}

/**
 * Invoked whenever the configuration value of a device is changed by the host
 * \param cfgnum Configuration number.
 */
void CDCHIDDDriver_ConfigurationChangedHandler(uint8_t cfgnum)
{
    USBDDriver *pUsbd = USBD_GetDriver();
    USBConfigurationDescriptor *pDesc;
    if (cfgnum > 0) {
        pDesc = USBDDriver_GetCfgDescriptors(pUsbd, cfgnum);
        /* CDC */
        CDCDSerial_ConfigureFunction((USBGenericDescriptor*)pDesc,
                                      pDesc->wTotalLength);
        /* HID */
        HIDDKeyboard_ConfigureFunction((USBGenericDescriptor*)pDesc,
                                        pDesc->wTotalLength);
    }
}

/**
 * Handles composite-specific USB requests sent by the host, and forwards
 * standard ones to the USB device driver.
 * \param request Pointer to a USBGenericRequest instance.
 */
void CDCHIDDDriver_RequestHandler(const USBGenericRequest *request)
{
    USBDDriver *pUsbd = USBD_GetDriver();

    TRACE_INFO_WP("NewReq ");

    if (CDCDSerial_RequestHandler(request) == USBRC_SUCCESS)
        return;

    if (HIDDKeyboard_RequestHandler(request) == USBRC_SUCCESS)
        return;

    USBDDriver_RequestHandler(pUsbd, request);
}

/**
 * Starts a remote wake-up sequence if the host has explicitely enabled it
 * by sending the appropriate SET_FEATURE request.
 */
void CDCHIDDDriver_RemoteWakeUp(void)
{
    USBDDriver *pUsbd = USBD_GetDriver();

    /* Remote wake-up has been enabled */
    if (USBDDriver_IsRemoteWakeUpEnabled(pUsbd)) {

        USBD_RemoteWakeUp();
    }
}

/**@}*/
