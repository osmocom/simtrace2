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
 *  \addtogroup usbd_msd
 *@{
 *  Implement a single interface device with single MS function in.
 */

/*------------------------------------------------------------------------------
 *      Includes
 *------------------------------------------------------------------------------*/

#include <MSDDriver.h>
#include <MSDFunction.h>
#include <USBLib_Trace.h>
#include <USBD.h>
#include <USBD_HAL.h>
#include <USBDDriver.h>

/*-----------------------------------------------------------------------------
 *         Internal variables
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *      Internal functions
 *-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 *      Exported functions
 *-----------------------------------------------------------------------------*/

/**
 * Initializes the MSD driver and the associated USB driver.
 * \param  pDescriptors Pointer to Descriptors list for MSD Device.
 * \param  pLuns        Pointer to a list of LUNs
 * \param  numLuns      Number of LUN in list
 * \see MSDLun
 */
void MSDDriver_Initialize(
    const USBDDriverDescriptors *pDescriptors,
    MSDLun *pLuns, unsigned char numLuns)
{
    USBDDriver *pUsbd = USBD_GetDriver();
    USBDDriver_Initialize(pUsbd, pDescriptors, 0);
    MSDFunction_Initialize(pUsbd, 0, pLuns, numLuns);
    USBD_Init();
}

/**
 * Invoked when the configuration of the device changes. Resets the mass
 * storage driver.
 * \param  pMsdDriver  Pointer to MSDDriver instance.
 * \param  cfgnum      New configuration number.
 */
void MSDDriver_ConfigurationChangeHandler(
    uint8_t cfgnum)
{
    USBDDriver *pUsbd = USBD_GetDriver();
    USBConfigurationDescriptor *pDesc;
    if (cfgnum) {
        pDesc = USBDDriver_GetCfgDescriptors(pUsbd, cfgnum);
        MSDFunction_Configure((USBGenericDescriptor*)pDesc,
                              pDesc->wTotalLength);
    }
}

/**
 * Handler for incoming SETUP requests on default Control endpoint 0.
 *
 * Standard requests are forwarded to the USBDDriver_RequestHandler
 * method.
 * \param  pMsdDriver  Pointer to MSDDriver instance.
 * \param  request Pointer to a USBGenericRequest instance
 */
void MSDDriver_RequestHandler(
    const USBGenericRequest *request)
{
    USBDDriver *pUsbd = USBD_GetDriver();
    TRACE_INFO_WP("NewReq ");
    if (MSDFunction_RequestHandler(request)) {
        USBDDriver_RequestHandler(pUsbd, request);
    }
}

/**@}*/

