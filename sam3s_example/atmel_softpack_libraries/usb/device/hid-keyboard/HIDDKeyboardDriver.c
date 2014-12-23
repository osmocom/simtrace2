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

/** \file
 * \addtogroup usbd_hid_key
 *@{
 * Implement a USB device that only have HID Keyboard Function.
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <HIDDKeyboardDriver.h>
#include <HIDDFunction.h>

#include <USBLib_Trace.h>

#include <USBRequests.h>
#include <HIDDescriptors.h>
#include <HIDRequests.h>
#include <HIDReports.h>
#include <HIDUsages.h>

#include <USBD.h>
#include <USBD_HAL.h>
#include <USBDDriver.h>

/*------------------------------------------------------------------------------
 *         Internal types
 *------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *         Internal functions
 *------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *      Exported functions
 *------------------------------------------------------------------------------*/

/**
 * Initializes the HID keyboard device driver.
 */
void HIDDKeyboardDriver_Initialize(const USBDDriverDescriptors *pDescriptors)
{
    USBDDriver *pUsbd = USBD_GetDriver();
    USBDDriver_Initialize(pUsbd, pDescriptors, 0);
    HIDDKeyboard_Initialize(pUsbd, 0);
    USBD_Init();
}

/**
 * Handles configureation changed event.
 * \param cfgnum New configuration number
 */
void HIDDKeyboardDriver_ConfigurationChangedHandler(uint8_t cfgnum)
{
    USBDDriver *pUsbd = USBD_GetDriver();
    const USBDDriverDescriptors * pDescriptors = pUsbd->pDescriptors;
    USBConfigurationDescriptor *pDesc;

    if (cfgnum > 0) {
        if (USBD_HAL_IsHighSpeed() && pDescriptors->pHsConfiguration)
            pDesc = (USBConfigurationDescriptor*)pDescriptors->pHsConfiguration;
        else
            pDesc = (USBConfigurationDescriptor*)pDescriptors->pFsConfiguration;
        HIDDKeyboard_ConfigureFunction((USBGenericDescriptor*)pDesc,
                                       pDesc->wTotalLength);
    }
}

/**
 * Handles HID-specific SETUP request sent by the host.
 * \param request Pointer to a USBGenericRequest instance.
 */
void HIDDKeyboardDriver_RequestHandler(const USBGenericRequest *request)
{
    USBDDriver *pUsbd = USBD_GetDriver();

    TRACE_INFO_WP("NewReq ");

    /* Process HID requests */
    if (USBRC_SUCCESS == HIDDKeyboard_RequestHandler(request)) {
        return;
    }
    /* Process STD requests */
    else {
        USBDDriver_RequestHandler(pUsbd, request);
    }
}

/**@}*/

