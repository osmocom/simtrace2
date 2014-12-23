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
 *  Implements Massstorage Function for USB device.
 */

/*------------------------------------------------------------------------------
 *      Includes
 *------------------------------------------------------------------------------*/

#include <MSDescriptors.h>

#include <MSDDriver.h>
#include <USBLib_Trace.h>
#include <USBD.h>
#include <USBD_HAL.h>
#include <USBDDriver.h>

/*-----------------------------------------------------------------------------
 *         Internal Types
 *-----------------------------------------------------------------------------*/

/** Parse data extension */
typedef struct _MSDParseData {
    /** Pointer to driver instance */
    MSDDriver *pMsdd;
    /** Pointer to currently processed interface descriptor */
    USBInterfaceDescriptor *pIf;
} MSDParseData;


/*-----------------------------------------------------------------------------
 *         Internal variables
 *-----------------------------------------------------------------------------*/

/** MSD Driver instance for device function */
static MSDDriver msdFunction;

/*-----------------------------------------------------------------------------
 *      Internal functions
 *-----------------------------------------------------------------------------*/

/**
 * Parse descriptors: Bulk EP IN/OUT.
 * \param desc Pointer to current processed descriptor.
 * \param arg  Pointer to data extention struct for parsing.
 */
static uint8_t MSDFunction_Parse(USBGenericDescriptor* desc, MSDParseData* arg)
{
    MSDDriver *pMsdd = arg->pMsdd;
    USBInterfaceDescriptor *pIf;

    /* Not a valid descriptor */
    if (desc->bLength == 0) {
        return USBD_STATUS_INVALID_PARAMETER;
    }
    /* Find interface descriptor */
    if (desc->bDescriptorType == USBGenericDescriptor_INTERFACE) {
        pIf = (USBInterfaceDescriptor*)desc;
        if (pIf->bInterfaceClass == MSInterfaceDescriptor_CLASS) {
            /* First found IF */
            if (pMsdd->interfaceNb == 0xFF) {
                pMsdd->interfaceNb = pIf->bInterfaceNumber;
                arg->pIf = pIf;
            }
            /* Specific IF */
            else if (pMsdd->interfaceNb == pIf->bInterfaceNumber) {
                arg->pIf = pIf;
            }
            
        }
    }
    /* Start parse endpoints */
    if (arg->pIf) {
        if (desc->bDescriptorType == USBGenericDescriptor_ENDPOINT) {
            USBEndpointDescriptor *pEP = (USBEndpointDescriptor*)desc;
            if (pEP->bmAttributes == USBEndpointDescriptor_BULK) {
                if (pEP->bEndpointAddress & 0x80)
                    pMsdd->commandState.pipeIN = pEP->bEndpointAddress & 0x7F;
                else
                    pMsdd->commandState.pipeOUT = pEP->bEndpointAddress;
            }
        }

        /* Finish when found all pipes */
        if (pMsdd->commandState.pipeIN != 0
            && pMsdd->commandState.pipeOUT != 0) {
            return USBRC_FINISHED;
        }
    }
    return 0;
}

/**
 * Resets the state of the MSD driver
 */
static void MSDFunction_Reset(void)
{
    MSDDriver *pMsdd = &msdFunction;

    TRACE_INFO_WP("MSDReset ");

    pMsdd->state = MSDD_STATE_READ_CBW;
    pMsdd->waitResetRecovery = 0;
    pMsdd->commandState.state = 0;
}

/*-----------------------------------------------------------------------------
 *      Exported functions
 *-----------------------------------------------------------------------------*/

/**
 * Initializes the MSD driver and the associated USB driver.
 * \param  pUsbd        Pointer to USBDDriver instance.
 * \param  bInterfaceNb Interface number for the function.
 * \param  pLuns        Pointer to a list of LUNs
 * \param  numLuns      Number of LUN in list
 * \see MSDLun
 */
void MSDFunction_Initialize(
    USBDDriver *pUsbd, uint8_t bInterfaceNb,
    MSDLun *pLuns, uint8_t numLuns)
{
    MSDDriver *pMsdDriver = &msdFunction;

    TRACE_INFO("MSDFun init\n\r");

    /* Driver instance */
    pMsdDriver->pUsbd = pUsbd;
    pMsdDriver->interfaceNb = bInterfaceNb;

    /* Command state initialization */
    pMsdDriver->commandState.state = 0;
    pMsdDriver->commandState.postprocess = 0;
    pMsdDriver->commandState.length = 0;
    pMsdDriver->commandState.transfer.semaphore = 0;

    /* LUNs */
    pMsdDriver->luns = pLuns;
    pMsdDriver->maxLun = (uint8_t) (numLuns - 1);

    /* Reset BOT driver */
    MSDFunction_Reset();
}

/**
 * Invoked when the configuration of the device changes. 
 * Pass endpoints and resets the mass storage function.
 * \pDescriptors Pointer to the descriptors for function configure.
 * \wLength      Length of descriptors in number of bytes.
 */
void MSDFunction_Configure(USBGenericDescriptor *pDescriptors,
                           uint16_t wLength)
{
    MSDDriver *pMsdDriver = &msdFunction;
    MSDParseData parseData;

    TRACE_INFO_WP("MSDFunCfg ");

    pMsdDriver->state = MSDD_STATE_READ_CBW;
    pMsdDriver->waitResetRecovery = 0;
    pMsdDriver->commandState.state = 0;

    parseData.pIf = 0;
    parseData.pMsdd = pMsdDriver;
    USBGenericDescriptor_Parse((USBGenericDescriptor*)pDescriptors, wLength,
                (USBDescriptorParseFunction)MSDFunction_Parse, &parseData);

    MSDFunction_Reset();
}

/**
 * Handler for incoming SETUP requests on default Control endpoint 0.
 *
 * Standard requests are forwarded to the USBDDriver_RequestHandler
 * method.
 * \param  pMsdDriver  Pointer to MSDDriver instance.
 * \param  request Pointer to a USBGenericRequest instance
 */
uint32_t MSDFunction_RequestHandler(
    const USBGenericRequest *request)
{
    MSDDriver *pMsdDriver = &msdFunction;
    uint32_t reqCode = (USBGenericRequest_GetType(request) << 8)
                     | (USBGenericRequest_GetRequest(request));

    TRACE_INFO_WP("Msdf ");

    /* Handle requests */
    switch (reqCode) {
    /*--------------------- */
    case USBGenericRequest_CLEARFEATURE:
    /*--------------------- */
        TRACE_INFO_WP("ClrFeat ");

        switch (USBFeatureRequest_GetFeatureSelector(request)) {

        /*--------------------- */
        case USBFeatureRequest_ENDPOINTHALT:
        /*--------------------- */
            TRACE_INFO_WP("Hlt ");

            /* Do not clear the endpoint halt status if the device is waiting */
            /* for a reset recovery sequence */
            if (!pMsdDriver->waitResetRecovery) {

                /* Forward the request to the standard handler */
                USBDDriver_RequestHandler(USBD_GetDriver(), request);
            }
            else {

                TRACE_INFO_WP("No ");
            }

            USBD_Write(0, 0, 0, 0, 0);

            return USBRC_SUCCESS; /* Handled */

        }
        break;

    /*------------------- */
    case (USBGenericRequest_CLASS<<8)|MSD_GET_MAX_LUN:
    /*------------------- */
        TRACE_INFO_WP("gMaxLun ");

        /* Check request parameters */
        if ((request->wValue == 0)
            && (request->wIndex == pMsdDriver->interfaceNb)
            && (request->wLength == 1)) {

            USBD_Write(0, &(pMsdDriver->maxLun), 1, 0, 0);

        }
        else {

            TRACE_WARNING(
                "MSDDriver_RequestHandler: GetMaxLUN(%d,%d,%d)\n\r",
                request->wValue, request->wIndex, request->wLength);
            USBD_Stall(0);
        }
        return USBRC_SUCCESS; /* Handled */

    /*----------------------- */
    case (USBGenericRequest_CLASS<<8)|MSD_BULK_ONLY_RESET:
    /*----------------------- */
        TRACE_INFO_WP("Rst ");

        /* Check parameters */
        if ((request->wValue == 0)
            && (request->wIndex == pMsdDriver->interfaceNb)
            && (request->wLength == 0)) {

            /* Reset the MSD driver */
            MSDFunction_Reset();
            USBD_Write(0, 0, 0, 0, 0);
        }
        else {

            TRACE_WARNING(
                "MSDDriver_RequestHandler: Reset(%d,%d,%d)\n\r",
                request->wValue, request->wIndex, request->wLength);
            USBD_Stall(0);
        }
        return USBRC_SUCCESS; /* Handled */
    }

    return USBRC_PARAM_ERR;
}

/**
 * State machine for the MSD driver
 */
void MSDFunction_StateMachine(void)
{
    if (USBD_GetState() < USBD_STATE_CONFIGURED){}
    else MSDD_StateMachine(&msdFunction);

}

/**@}*/

