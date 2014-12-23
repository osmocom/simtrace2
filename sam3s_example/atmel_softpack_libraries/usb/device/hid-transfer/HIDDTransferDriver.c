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
 * \addtogroup usbd_hid_tran
 *@{
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <stdint.h>

#include "HIDDTransferDriver.h"
#include <USBLib_Trace.h>

#include <USBRequests.h>
#include <HIDDescriptors.h>
#include <HIDDFunction.h>

#include <USBD_HAL.h>

#include <string.h>

/*------------------------------------------------------------------------------
 *         Internal types
 *------------------------------------------------------------------------------*/

/**
 * Report struct for HID transfer.
 */
typedef struct _HIDDTransferReport {
    /** Callback when report done */
    HIDDReportEventCallback fCallback;
    /** Callback arguments */
    void* pArg;

    /** Report size (ID + DATA) */
    uint16_t wMaxSize;
    /** Transfered size */
    uint16_t wTransferred;
    /** Report idle rate */
    uint8_t bIdleRate;
    /** Delay count for Idle */
    uint8_t bDelay;
    /** Report ID */
    uint8_t bID;
    /** Report data address */
    uint8_t bData[HIDDTransferDriver_REPORTSIZE];
} HIDDTransferReport;

/**
 * Driver structure for an HID device implementing simple transfer
 * functionalities.
 */
typedef struct _HIDDTransferDriver {

    /** Standard HID function interface. */
    HIDDFunction hidFunction;

    /** HID Input report list */
    HIDDReport *inputReports[1];
    /** HID Output report list */
    HIDDReport *outputReports[1];

    /* OUT Report - block input for SET_REPORT */
    /**< Output report block size */
    uint16_t iReportLen;
    /**< Output report data buffer */
    uint8_t  iReportBuf[HIDDTransferDriver_REPORTSIZE];

} HIDDTransferDriver;

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/

/** Input report buffers */
static HIDDTransferReport inputReport;

/** Output report buffers */
static HIDDTransferReport outputReport;

/** Static instance of the HID Transfer device driver. */
static HIDDTransferDriver hiddTransferDriver;

/** Report descriptor used by the driver. */
static const uint8_t hiddTransferReportDescriptor[] = {

    /* Global Usage Page */
    HIDReport_GLOBAL_USAGEPAGE + 2, 0xFF, 0xFF, /* Vendor-defined */

    /* Collection: Application */
    HIDReport_LOCAL_USAGE + 1, 0xFF, /* Vendor-defined */
    HIDReport_COLLECTION + 1, HIDReport_COLLECTION_APPLICATION,

        /* Input report: Vendor-defined */
        HIDReport_LOCAL_USAGE + 1, 0xFF, /* Vendor-defined usage */
        HIDReport_GLOBAL_REPORTCOUNT + 1, HIDDTransferDriver_REPORTSIZE,
        HIDReport_GLOBAL_REPORTSIZE + 1, 8,
        HIDReport_GLOBAL_LOGICALMINIMUM + 1, (uint8_t) -128,
        HIDReport_GLOBAL_LOGICALMAXIMUM + 1, (uint8_t)  127,
        HIDReport_INPUT + 1, 0,    /* No Modifiers */

        /* Output report: vendor-defined */
        HIDReport_LOCAL_USAGE + 1, 0xFF, /* Vendor-defined usage */
        HIDReport_GLOBAL_REPORTCOUNT + 1, HIDDTransferDriver_REPORTSIZE,
        HIDReport_GLOBAL_REPORTSIZE + 1, 8,
        HIDReport_GLOBAL_LOGICALMINIMUM + 1, (uint8_t) -128,
        HIDReport_GLOBAL_LOGICALMAXIMUM + 1, (uint8_t)  127,
        HIDReport_OUTPUT + 1, 0,    /* No Modifiers */
    HIDReport_ENDCOLLECTION
};

/*------------------------------------------------------------------------------
 *         Internal functions
 *------------------------------------------------------------------------------*/

/**
 * Returns the descriptor requested by the host.
 * \param type Descriptor type.
 * \param length Maximum number of bytes to send.
 * \return 1 if the request has been handled by this function, otherwise 0.
 */
static uint8_t HIDDTransferDriver_GetDescriptor(uint8_t type,
                                                uint8_t length)
{
    HIDDTransferDriver *pDrv = &hiddTransferDriver;
    HIDDFunction *pHidd = &pDrv->hidFunction;

    const USBConfigurationDescriptor *pConfiguration;
    HIDDescriptor *hidDescriptors[2];

    switch (type) {

        case HIDGenericDescriptor_REPORT:
            TRACE_INFO("Report ");

            /* Adjust length and send report descriptor */
            if (length > HIDDTransferDriver_REPORTDESCRIPTORSIZE) {

                length = HIDDTransferDriver_REPORTDESCRIPTORSIZE;
            }
            USBD_Write(0, &hiddTransferReportDescriptor, length, 0, 0);
            break;

        case HIDGenericDescriptor_HID:
            TRACE_INFO("HID ");

            /* Configuration descriptor is different depending on configuration */
            if (USBD_IsHighSpeed()) {

                pConfiguration =
                   pHidd->pUsbd->pDescriptors->pHsConfiguration;
            }
            else {

                pConfiguration =
                   pHidd->pUsbd->pDescriptors->pFsConfiguration;
            }

            /* Parse the device configuration to get the HID descriptor */
            USBConfigurationDescriptor_Parse(pConfiguration, 0, 0,
                                     (USBGenericDescriptor **) hidDescriptors);

            /* Adjust length and send HID descriptor */
            if (length > sizeof(HIDDescriptor)) {

                length = sizeof(HIDDescriptor);
            }
            USBD_Write(0, hidDescriptors[0], length, 0, 0);
            break;

        default:
            return 0;
    }

    return 1;
}

/**
 * Callback function when SetReport request data received from host
 * \param pArg Pointer to additional argument struct
 * \param status Result status
 * \param transferred Number of bytes transferred
 * \param remaining Number of bytes that are not transferred yet
 */
static void HIDDTransferDriver_ReportReceived(void *pArg,
                                              uint8_t status,
                                              uint32_t transferred,
                                              uint32_t remaining)
{
    HIDDTransferDriver *pDrv = &hiddTransferDriver;
    pDrv->iReportLen = transferred;
    USBD_Write(0, 0, 0, 0, 0);
}

/*------------------------------------------------------------------------------
 *      Exported functions
 *------------------------------------------------------------------------------*/

/**
 * Initializes the HID Transfer %device driver.
 * \param pDescriptors Pointer to USBDDriverDescriptors instance.
 */
void HIDDTransferDriver_Initialize(const USBDDriverDescriptors * pDescriptors)
{
    HIDDTransferDriver * pDrv = &hiddTransferDriver;
    USBDDriver *pUsbd = USBD_GetDriver();

    /* One input report */
    pDrv->inputReports[0] = (HIDDReport*)&inputReport;
    HIDDFunction_InitializeReport((HIDDReport *)pDrv->inputReports[0],
                                  HIDDTransferDriver_REPORTSIZE,
                                  0,
                                  0, 0);
    /* One output report */
    pDrv->outputReports[0] = (HIDDReport*)&outputReport;
    HIDDFunction_InitializeReport((HIDDReport *)pDrv->outputReports[0],
                                  HIDDTransferDriver_REPORTSIZE,
                                  0,
                                  0, 0);

    /* Initialize USBD Driver instance */
    USBDDriver_Initialize(pUsbd,
                          pDescriptors,
                          0); /* Multiple interface settings not supported */
    /* Function instance initialize */
    HIDDFunction_Initialize(&pDrv->hidFunction,
                            pUsbd, 0,
                            hiddTransferReportDescriptor,
                            (HIDDReport **)(&pDrv->inputReports), 1,
                            (HIDDReport **)(&pDrv->outputReports), 1);
    /* Initialize USBD */
    USBD_Init();
}

/**
 * Handles configureation changed event.
 * \param cfgnum New configuration number
 */
void HIDDTransferDriver_ConfigurationChangedHandler(uint8_t cfgnum)
{
    const USBDDriverDescriptors * pDescriptors = USBD_GetDriver()->pDescriptors;
    HIDDTransferDriver * pDrv = &hiddTransferDriver;
    HIDDFunction * pHidd = &pDrv->hidFunction;

    USBConfigurationDescriptor *pDesc;

    if (cfgnum > 0) {

        /* Parse endpoints for reports */
        if (USBD_HAL_IsHighSpeed() && pDescriptors->pHsConfiguration)
            pDesc = (USBConfigurationDescriptor*)pDescriptors->pHsConfiguration;
        else
            pDesc = (USBConfigurationDescriptor*)pDescriptors->pFsConfiguration;
        HIDDFunction_ParseInterface(pHidd,
                                    (USBGenericDescriptor*)pDesc,
                                    pDesc->wTotalLength);

        /* Start polling for Output Reports */
        HIDDFunction_StartPollingOutputs(pHidd);
    }
}

/**
 * Handles HID-specific SETUP request sent by the host.
 * \param request Pointer to a USBGenericRequest instance
 */
void HIDDTransferDriver_RequestHandler(const USBGenericRequest *request)
{
    HIDDTransferDriver *pDrv = &hiddTransferDriver;
    HIDDFunction *pHidd = &pDrv->hidFunction;

    TRACE_INFO("NewReq ");

    /* Check if this is a standard request */
    if (USBGenericRequest_GetType(request) == USBGenericRequest_STANDARD) {

        /* This is a standard request */
        switch (USBGenericRequest_GetRequest(request)) {

        case USBGenericRequest_GETDESCRIPTOR:
            /* Check if this is a HID descriptor, otherwise forward it to
               the standard driver */
            if (!HIDDTransferDriver_GetDescriptor(
                    USBGetDescriptorRequest_GetDescriptorType(request),
                    USBGenericRequest_GetLength(request))) {

                USBDDriver_RequestHandler(pHidd->pUsbd,
                                          request);
            }
            return; /* Handled, no need to do others */

        case USBGenericRequest_CLEARFEATURE:

            /* Check which is the requested feature */
            switch (USBFeatureRequest_GetFeatureSelector(request)) {
                case USBFeatureRequest_ENDPOINTHALT:
                {   uint8_t ep =
                        USBGenericRequest_GetEndpointNumber(request);
                        if (USBD_IsHalted(ep)) {
                            /* Unhalt endpoint restart OUT EP
                             */
                            USBD_Unhalt(ep);
                            if (ep == pHidd->bPipeOUT) {
                                HIDDFunction_StartPollingOutputs(pHidd);
                            }
                        }
                        /* and send a zero-length packet */
                        USBD_Write(0, 0, 0, 0, 0);
                    return; /* Handled, no need to do others */
                }
            }
            break;

        }
    }
    /* We use different buffer for SetReport */
    else if (USBGenericRequest_GetType(request) == USBGenericRequest_CLASS) {

        switch (USBGenericRequest_GetRequest(request)) {

        case HIDGenericRequest_SETREPORT:
            {
                uint16_t length = USBGenericRequest_GetLength(request);
                uint8_t  type = HIDReportRequest_GetReportType(request);
                if (type == HIDReportRequest_OUTPUT) {
                    if (length > HIDDTransferDriver_REPORTSIZE)
                        length = HIDDTransferDriver_REPORTSIZE;
                    USBD_Read(0,
                              pDrv->iReportBuf,
                              length,
                              HIDDTransferDriver_ReportReceived,
                              0); /* No argument to the callback function */
                }
                else {

                    USBD_Stall(0);
                }
            }
            return; /* Handled, no need do others */
        }
    }
    

    /* Process HID requests */
    if (USBRC_SUCCESS == HIDDFunction_RequestHandler(pHidd,
                                                     request)) {
        return;
    }
    else
        USBDDriver_RequestHandler(pHidd->pUsbd, request);
}

/**
 * Try to read request buffer of SetReport.
 * Set pData to 0 to get current data length only.
 * \param pData Pointer to data buffer
 * \param dwLength Data buffer length
 * \return Number of bytes read
 */
uint16_t HIDDTransferDriver_ReadReport(void *pData,
                                       uint32_t dwLength)
{
    HIDDTransferDriver *pDrv = &hiddTransferDriver;

    if (pData == 0) {

        return pDrv->iReportLen;
    }

    if (dwLength > HIDDTransferDriver_REPORTSIZE) {

        dwLength = HIDDTransferDriver_REPORTSIZE;
    }
    if (dwLength > pDrv->iReportLen) {

        dwLength = pDrv->iReportLen;
    }
    pDrv->iReportLen = 0;
    memcpy(pData, pDrv->iReportBuf, dwLength);

    return dwLength;
}

/**
 * Try to read request buffer of interrupt OUT EP.
 * Set pData to 0 to get current data length only.
 * \param pData Pointer to data buffer
 * \param dLength Data buffer length
 * \return Number of bytes read
 */
uint16_t HIDDTransferDriver_Read(void *pData,
                                 uint32_t dLength)
{
    HIDDTransferDriver *pDrv = &hiddTransferDriver;
    if (pData == 0) {

        return pDrv->outputReports[0]->wTransferred;
    }

    if (dLength > HIDDTransferDriver_REPORTSIZE) {

        dLength = HIDDTransferDriver_REPORTSIZE;
    }
    if (dLength > pDrv->outputReports[0]->wTransferred) {

        dLength = pDrv->outputReports[0]->wTransferred;
    }
    pDrv->outputReports[0]->wTransferred = 0;
    memcpy(pData, pDrv->outputReports[0]->bData, dLength);

    return dLength;
}

/**
 * Write data through USB interrupt IN EP.
 * \param pData Pointer to the data sent.
 * \param dLength The data length.
 * \param fCallback Callback function invoked when transferring done.
 * \param pArg Pointer to additional arguments.
 */
uint8_t HIDDTransferDriver_Write(const void *pData,
                                 uint32_t dLength,
                                 TransferCallback fCallback,
                                 void *pArg)
{
    HIDDTransferDriver *pDrv = &hiddTransferDriver;
    if (dLength != HIDDTransferDriver_REPORTSIZE) {

        dLength = HIDDTransferDriver_REPORTSIZE;
    }
    return USBD_Write(pDrv->hidFunction.bPipeIN,
                      pData, dLength,
                      fCallback, pArg);
}

/**
 * Starts a remote wake-up sequence if the host has explicitely enabled it
 * by sending the appropriate SET_FEATURE request.
 */
void HIDDTransferDriver_RemoteWakeUp(void)
{
    HIDDTransferDriver *pDrv = &hiddTransferDriver;

    /* Remote wake-up has been enabled */
    if (USBDDriver_IsRemoteWakeUpEnabled(pDrv->hidFunction.pUsbd)) {

        USBD_RemoteWakeUp();
    }
}

/**@}*/
