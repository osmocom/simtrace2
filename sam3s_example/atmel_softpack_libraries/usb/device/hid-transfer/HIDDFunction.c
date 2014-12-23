/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2010, Atmel Corporation
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
 *  Implementation of the HIDDFunction class methods.
 */
/** \addtogroup usbd_hid
 * @{
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <HIDDFunction.h>
#include <USBDescriptors.h>
#include <HIDDescriptors.h>

#include <USBLib_Trace.h>

/*------------------------------------------------------------------------------
 *         Definitions
 *------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *         Macros
 *------------------------------------------------------------------------------*/

/**
 * Get byte pointer
 */
#define _PU8(v)  ((uint8_t*)&(v))

/**
 * Get word from un-aligned value
 */
#define _Word(a) (_PU8(a)[0] + (_PU8(a)[1] << 8))

/*------------------------------------------------------------------------------
 *         Types
 *------------------------------------------------------------------------------*/

/** Parse data extention for descriptor parsing  */
typedef struct _HIDDParseData {
    HIDDFunction * pHidd;
    USBInterfaceDescriptor * pIfDesc;
} HIDDParseData;

/** Parse data extension for HID descriptor */

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *         Internal functions
 *------------------------------------------------------------------------------*/

/**
 *  Returns the descriptor requested by the host.
 * \param pHidd   Pointer to HIDDFunction instance
 * \param bType   Descriptor type.
 * \param wLength Maximum number of bytes to send.
 * \return USBRC_SUCCESS if the request has been handled by this function,
 *         otherwise USBRC_PARAM_ERR.
 */
static uint32_t HIDDFunction_GetDescriptor(HIDDFunction *pHidd,
                                           uint8_t  bType,
                                           uint32_t wLength)
{
    HIDDescriptor1 *pHidDescriptor = (HIDDescriptor1 *)pHidd->pHidDescriptor;
    uint16_t wDescriptorLength;

    TRACE_INFO_WP("gDesc{%x) ", bType);

    switch (bType) {

        case HIDGenericDescriptor_REPORT:

            /* Adjust length and send report descriptor */
            /*
            wDescriptorLength = pHidDescriptor->bDescriptorLength0[0]
                              + pHidDescriptor->bDescriptorLength0[1];
            */
            wDescriptorLength = _Word(pHidDescriptor->wDescriptorLength0);
            if (wLength > wDescriptorLength)
                wLength = wDescriptorLength;

            TRACE_INFO_WP("Report(%d) ", wLength);

            USBD_Write(0, pHidd->pReportDescriptor, wLength, 0, 0);
            break;

        case HIDGenericDescriptor_HID:

            /* Adjust length and send HID descriptor */
            if (wLength > sizeof(HIDDescriptor1)) 
                wLength = sizeof(HIDDescriptor1);

            TRACE_INFO_WP("HID(%d) ", wLength);

            USBD_Write(0, pHidDescriptor, wLength, 0, 0);
            break;

        default:
            return USBRC_PARAM_ERR;
    }

    return USBRC_SUCCESS;
}

/**
 * Return expected report header pointer.
 * \param pHidd Pointer to HIDDFunction instance
 * \param bType Report type.
 * \param bID   Report ID.
 */
static HIDDReport* HIDDFunction_FindReport(const HIDDFunction *pHidd,
                                           uint8_t bType,
                                           uint8_t bID)
{
    HIDDReport** pReportList;
    int32_t listSize, i;
    switch(bType) {
    case HIDReportRequest_INPUT:
        pReportList = pHidd->pInputList;
        listSize = pHidd->bInputListSize;
        break;
    case HIDReportRequest_OUTPUT:
        pReportList = pHidd->pOutputList;
        listSize = pHidd->bOutputListSize;
        break;
    /* No other reports supported */
    default:
        TRACE_INFO("Report %x.%x not support\n\r", bType, bID);
        return 0;
    }
    /* No list */
    if (pReportList == 0)
        return 0;
    /* Find report in the list */
    for (i = 0; i < listSize; i ++) {
        if (bID == pReportList[i]->bID)
            return pReportList[i];
    }
    /* Not found */
    return 0;
}

/**
 * Sends the current Idle rate of the input report to the host.
 * \param pHidd Pointer to HIDDFunction instance
 * \param bID   Report ID
 */
static void HIDDFunction_GetIdle(HIDDFunction *pHidd,
                                 uint8_t bID)
{
    HIDDReport *pReport = HIDDFunction_FindReport(pHidd,
                                                  HIDReportRequest_INPUT,
                                                  bID);
    TRACE_INFO_WP("gIdle(%x) ", bID);
    if (pReport == 0) {
        USBD_Stall(0);
        return;
    }
    USBD_Write(0, &pReport->bIdleRate, 1, 0, 0);
}

/**
 * Retrieves the new idle rate of the input report from the USB host.
 * \param pHidd     Pointer to HIDDFunction instance
 * \param bType     Report type
 * \param bID       Report ID
 * \param bIdleRate Report idle rate.
 */
static void HIDDFunction_SetIdle(HIDDFunction *pHidd,
                                 uint8_t bID,
                                 uint8_t bIdleRate)
{
    HIDDReport *pReport = HIDDFunction_FindReport(pHidd,
                                                  HIDReportRequest_INPUT,
                                                  bID);
    TRACE_INFO_WP("sIdle(%x<%x) ", bID, bIdleRate);
    if (pReport == 0) {
        USBD_Stall(0);
        return;
    }
    USBD_Write(0, 0, 0, 0, 0);
}

/**
 * Callback function when GetReport request data sent to host
 * \param pReport Pointer to report information.
 * \param status  Result status
 * \param transferred Number of bytes transferred
 * \param remaining Number of bytes that are not transferred yet
 */
static void _GetReportCallback(HIDDReport *pReport,
                               uint8_t status,
                               uint32_t transferred,
                               uint32_t remaining)
{
    pReport->wTransferred = transferred;
    if (pReport->fCallback)
        pReport->fCallback(HIDD_EC_GETREPORT, pReport->pArg);

    USBD_Read(0, 0, 0, 0, 0);
}

/**
 * Sends the requested report to the host.
 * \param pHidd   Pointer to HIDDFunction instance
 * \param bType   Report type.
 * \param bID     Report ID.
 * \param wLength Maximum number of bytes to send.
 */
static void HIDDFunction_GetReport(HIDDFunction *pHidd,
                                   uint8_t bType,
                                   uint8_t bID,
                                   uint8_t wLength)
{
    HIDDReport *pReport = HIDDFunction_FindReport(pHidd,
                                                  bType,
                                                  bID);
    TRACE_INFO_WP("gReport(%x.%x) ", bType, bID);
    if (pReport == 0) {
        USBD_Stall(0);
        return;
    }
    if (wLength >= pReport->wMaxSize) {
        wLength = pReport->wMaxSize;
    }
    USBD_Write(0, pReport->bData, wLength,
               (TransferCallback)_GetReportCallback, pReport);
}

/**
 * Callback function when GetReport request data sent to host
 * \param pReport Pointer to report information.
 * \param status  Result status
 * \param transferred Number of bytes transferred
 * \param remaining Number of bytes that are not transferred yet
 */
static void _SetReportCallback(HIDDReport *pReport,
                               uint8_t status,
                               uint32_t transferred,
                               uint32_t remaining)
{
    pReport->wTransferred = transferred;
    if (pReport->fCallback) {
        pReport->fCallback(HIDD_EC_SETREPORT, pReport->pArg);
    }
}

/**
 * Reads the requested report from the host.
 * \param pHidd   Pointer to HIDDFunction instance
 * \param bType   Report type.
 * \param bID     Report ID.
 * \param wLength Maximum number of bytes to read.
 */
static void HIDDFunction_SetReport(HIDDFunction *pHidd,
                                   uint8_t bType,
                                   uint8_t bID,
                                   uint8_t wLength)
{
    HIDDReport *pReport = HIDDFunction_FindReport(pHidd,
                                                  bType,
                                                  bID);
    TRACE_INFO_WP("sReport(%x.%x) ", bType, bID);

    if (pReport == 0) {
        USBD_Stall(0);
        return;
    }

    if (wLength >= pReport->wMaxSize) {
        wLength = pReport->wMaxSize;
    }
    USBD_Read(0, pReport->bData, wLength,
              (TransferCallback)_SetReportCallback, pReport);
}

/**
 * Parse descriptors: Interface, Interrupt IN/OUT.
 * \param desc Pointer to descriptor list.
 * \param arg  Argument, pointer to HIDDParseData instance.
 */
static uint32_t HIDDFunction_Parse(USBGenericDescriptor * pDesc,
                                   HIDDParseData * pArg)
{
    /* Find HID Interface */
    if (pArg->pIfDesc == 0) {
        if (pDesc->bDescriptorType == USBGenericDescriptor_INTERFACE) {
            USBInterfaceDescriptor *pIf = (USBInterfaceDescriptor*)pDesc;
            /* Right interface for HID:
                   HID Class + at least 1 endpoint */
            if (pIf->bInterfaceClass == HIDInterfaceDescriptor_CLASS
                && pIf->bNumEndpoints >= 1) {
                /* Obtain new interface setting */
                if (pArg->pHidd->bInterface == 0xFF) {
                    pArg->pHidd->bInterface = pIf->bInterfaceNumber;
                    pArg->pIfDesc = pIf;
                }
                /* Find specific interface setting */
                else if (pArg->pHidd->bInterface == pIf->bInterfaceNumber) {
                    pArg->pIfDesc = pIf;
                }
            }
        }
    }
    /* Interface end */
    else {
        /* Start another interface ? */
        if (pDesc->bDescriptorType == USBGenericDescriptor_INTERFACE) {
            /* Terminate the parse */
            return USBRC_PARTIAL_DONE;
        }
        /* Parse HID descriptor */
        else if (pDesc->bDescriptorType == HIDGenericDescriptor_HID) {
            pArg->pHidd->pHidDescriptor = (HIDDescriptor*)pDesc;
        }
        /* Parse endpoints */
        else if (pDesc->bDescriptorType == USBGenericDescriptor_ENDPOINT) {
            USBEndpointDescriptor *pEp = (USBEndpointDescriptor*)pDesc;
            if (pEp->bEndpointAddress & 0x80)
                pArg->pHidd->bPipeIN = pEp->bEndpointAddress & 0x7F;
            else
                pArg->pHidd->bPipeOUT = pEp->bEndpointAddress;
        }

        /* Check if all data is OK */
        if (pArg->pHidd->bInterface  != 0xFF
            && pArg->pHidd->bPipeIN  != 0xFF
            && pArg->pHidd->bPipeOUT != 0xFF)
            return USBRC_FINISHED;
    }
    return 0;
}

/**
 * Callback function when interrupt OUT data received from host
 * \param pHidd  Pointer to HIDDFunction instance
 * \param status Result status
 * \param transferred Number of bytes transferred
 * \param remaining Number of bytes that are not transferred yet
 */
static void HIDDFunction_ReportReceived(HIDDFunction *pHidd,
                                        uint8_t status,
                                        uint32_t transferred,
                                        uint32_t remaining)
{
    HIDDReport *pOut = pHidd->pOutputList[pHidd->bCurrOutput];
    if (status != USBRC_SUCCESS) {

        TRACE_ERROR("HIDDFun::ReadReport: %x\n\r", status);
        return;
    }

    /* Transfered information */
    pOut->wTransferred = transferred;

    /* Data Change callback */
    if (pOut->fCallback)
        pOut->fCallback(HIDD_EC_REPORTCHANGED, pOut->pArg);

    /* Proceed to next output report */
    pHidd->bCurrOutput ++;
    if (pHidd->bCurrOutput >= pHidd->bOutputListSize)
        pHidd->bCurrOutput = 0;

    /* Start reading a report */
    USBD_Read(pHidd->bPipeOUT,
              pHidd->pOutputList[pHidd->bCurrOutput]->bData,
              pHidd->pOutputList[pHidd->bCurrOutput]->wMaxSize,
              (TransferCallback)HIDDFunction_ReportReceived,
              (void*)pHidd);
}

/**
 * Callback function when interrupt IN data sent to host
 * \param pHidd  Pointer to HIDDFunction instance
 * \param status Result status
 * \param transferred Number of bytes transferred
 * \param remaining Number of bytes that are not transferred yet
 */
static void HIDDFunction_ReportSent(HIDDFunction *pHidd,
                                    uint8_t status,
                                    uint32_t transferred,
                                    uint32_t remaining)
{
    HIDDReport *pIn = pHidd->pInputList[pHidd->bCurrInput];
    if (status != USBRC_SUCCESS) {

        TRACE_ERROR("HIDDFun::WriteReport: %x\n\r", status);
        return;
    }

    /* Transfered information */
    pIn->wTransferred = transferred;

    /* Report Sent Callback */
    if (pIn->fCallback)
        pIn->fCallback(HIDD_EC_REPORTSENT, pIn->pArg);

    /* Proceed to next output report */
    pHidd->bCurrInput ++;
    if (pHidd->bCurrInput >= pHidd->bInputListSize)
        pHidd->bCurrInput = 0;

    /* Start writing a report */
    USBD_Write(pHidd->bPipeIN,
               pHidd->pInputList[pHidd->bCurrInput]->bData,
               pHidd->pInputList[pHidd->bCurrInput]->wMaxSize,
              (TransferCallback)HIDDFunction_ReportReceived,
              (void*)pHidd);
}


/*------------------------------------------------------------------------------
 *         Exported functions
 *------------------------------------------------------------------------------*/

/**
 * Initialize the USB Device HID function, for general HID device support.
 * \param pHidd             Pointer to HIDDFunction instance.
 * \param pUsbd             Pointer to USBDDriver instance.
 * \param bInterfaceNb      Interface number,
 *                          can be 0xFF to obtain from descriptors.
 * \param pReportDescriptor Pointer to report descriptor.
 * \param pInputList        Pointer to an HID input report list
 * \param bInputListSize    HID input report list size
 * \param pOutputList       Pointer to an HID output report list
 * \param bOutputListSize   HID output report list size
 */
void HIDDFunction_Initialize(HIDDFunction * pHidd,
                             USBDDriver * pUsbd, uint8_t bInterfaceNb,
                             const uint8_t * pReportDescriptor,
                             HIDDReport* pInputList[], uint8_t bInputListSize,
                             HIDDReport* pOutputList[], uint8_t bOutputListSize)
{
    TRACE_INFO("HIDDFunction_Initialize\n\r");

    pHidd->pUsbd = pUsbd;
    pHidd->pReportDescriptor = (uint8_t *)pReportDescriptor;
    pHidd->pHidDescriptor = 0;

    pHidd->bInterface = bInterfaceNb;
    pHidd->bPipeIN    = 0xFF;
    pHidd->bPipeOUT   = 0xFF;
    pHidd->bProtocol  = HIDProtocol_REPORT;    /* Non-boot protocol */

    pHidd->pInputList = pInputList;
    pHidd->pOutputList = pOutputList;
    pHidd->bInputListSize = bInputListSize;
    pHidd->bOutputListSize = bOutputListSize;
    pHidd->bCurrInput = 0;
    pHidd->bCurrOutput = 0;

}

/**
 * Parse the USB HID Function Interface.
 * Only first interface and its endpoints parsed.
 * \param pHidd Pointer to HIDDFunction instance.
 * \param pDescriptors Pointer to descriptor list.
 * \param dwLength     Descriptor list block length in bytes.
 * \return Pointer to next descriptor. 0 means no other descriptor.
 */
USBGenericDescriptor *HIDDFunction_ParseInterface(HIDDFunction * pHidd,
                                                  USBGenericDescriptor * pDescriptors,
                                                  uint32_t dwLength)
{
    HIDDParseData data;
    pHidd->bPipeIN    = 0xFF;
    pHidd->bPipeOUT   = 0xFF;
    data.pHidd = pHidd;
    data.pIfDesc     = 0;
    return USBGenericDescriptor_Parse(pDescriptors,
                                      dwLength,
                                      (USBDescriptorParseFunction)HIDDFunction_Parse,
                                      (void*)&data);
}

/**
 * Start polling interrupt OUT pipe
 * (output report, host to device) if there is.
 * \param pHidd Pointer to HIDDFunction instance.
 */
uint32_t HIDDFunction_StartPollingOutputs(HIDDFunction * pHidd)
{
    /* No report, do nothing */
    if (pHidd->bOutputListSize == 0
        || pHidd->pOutputList == 0)
        return USBRC_PARAM_ERR;

    /* Start reading a report */
    return USBD_Read(pHidd->bPipeOUT,
                     pHidd->pOutputList[pHidd->bCurrOutput]->bData,
                     pHidd->pOutputList[pHidd->bCurrOutput]->wMaxSize,
                     (TransferCallback)HIDDFunction_ReportReceived,
                     (void*)pHidd);
}

/**
 * Start sending reports via interrupt IN pipe
 * (input report, device to host) if there is.
 * \param pHidd Pointer to HIDDFunction instance.
 */
uint32_t HIDDFunction_StartSendingInputs(HIDDFunction * pHidd)
{
    /* No report, do nothing */
    if (pHidd->bInputListSize == 0
        || pHidd->pInputList == 0)
        return USBRC_PARAM_ERR;
    /* Start sending a report */
    return USBD_Write(pHidd->bPipeIN,
                      pHidd->pInputList[pHidd->bCurrInput]->bData,
                      pHidd->pInputList[pHidd->bCurrInput]->wMaxSize,
                      (TransferCallback)HIDDFunction_ReportSent,
                      (void*)pHidd);
}

/**
 * Handles HID-specific SETUP request sent by the host.
 * \param pHidd Pointer to HIDDFunction instance.
 * \param request Pointer to a USBGenericRequest instance
 */
uint32_t HIDDFunction_RequestHandler(HIDDFunction *pHidd,
                                     const USBGenericRequest *request)
{
    uint32_t reqCode = (request->bmRequestType << 8)
                     | (request->bRequest);

    switch (reqCode) {
    /* Get_Descriptor */
    case USBGenericRequest_GETDESCRIPTOR|(0x81<<8):
        return HIDDFunction_GetDescriptor(
                    pHidd,
                    USBGetDescriptorRequest_GetDescriptorType(request),
                    USBGenericRequest_GetLength(request));
    /* Clear_Feature (EP) */
    case USBGenericRequest_CLEARFEATURE|(0x02<<8):
        if (USBFeatureRequest_GetFeatureSelector(request)
            == USBFeatureRequest_ENDPOINTHALT) {
            uint8_t ep = USBGenericRequest_GetEndpointNumber(request);
            if (USBD_IsHalted(ep)) {
                /* Unhalt EP */
                USBD_Unhalt(ep);
                /* Restart Polling OUT */
                if (ep == pHidd->bPipeOUT) {
                    HIDDFunction_StartPollingOutputs(pHidd);
                }
                /* and send a zero-length packet */
                USBD_Write(0, 0, 0, 0, 0);
            }
            break; /* Handled success */
        }
        return USBRC_PARAM_ERR;
    /* Set_Descriptor */
    case USBGenericRequest_SETDESCRIPTOR|(0x01<<8):
        /* Optional, not implemented */
        USBD_Stall(0);
        break;
    /* Get_Idle */
    case (0xa1<<8)|HIDGenericRequest_GETIDLE:
        HIDDFunction_GetIdle(pHidd,
                             HIDReportRequest_GetReportId(request));
        break;
    /* Set_Idle */
    case (0x21<<8)|HIDGenericRequest_SETIDLE:
        HIDDFunction_SetIdle(pHidd,
                             HIDReportRequest_GetReportId(request),
                             HIDIdleRequest_GetIdleRate(request));
        break;
    /* Get_Report */
    case (0xa1<<8)|HIDGenericRequest_GETREPORT:
        HIDDFunction_GetReport(pHidd,
                               HIDReportRequest_GetReportType(request),
                               HIDReportRequest_GetReportId(request),
                               USBGenericRequest_GetLength(request));
        break;
    /* Set_Report */
    case (0x21<<8)|HIDGenericRequest_SETREPORT:
        HIDDFunction_SetReport(pHidd,
                               HIDReportRequest_GetReportType(request),
                               HIDReportRequest_GetReportId(request),
                               USBGenericRequest_GetLength(request));
        break;
    /* Get_Protocol */
    case (0xa1<<8)|HIDGenericRequest_SETPROTOCOL:
        pHidd->bProtocol = request->wValue;
        USBD_Write(0, 0, 0, 0, 0);
        break;
    /* Set_Protocol */
    case (0x21<<8)|HIDGenericRequest_GETPROTOCOL:
        USBD_Write(0, &pHidd->bProtocol, 1, 0, 0);
        break;

    default:
        return USBRC_PARAM_ERR;
    }
    return USBRC_SUCCESS;
}

/**
 * Read raw data through USB interrupt OUT EP.
 * \param pHidd     Pointer to HIDDFunction instance.
 * \param pData     Pointer to the data buffer.
 * \param dwLength  The data length.
 * \param fCallback Callback function invoked when transferring done.
 * \param pArg Pointer to additional arguments.
 */
uint32_t HIDDFunction_Read(const HIDDFunction *pHidd,
                           void* pData,
                           uint32_t dwLength,
                           TransferCallback fCallback,
                           void* pArg)
{
    return USBD_Read(pHidd->bPipeIN,
                     pData, dwLength,
                     fCallback, pArg);
}

/**
 * Write raw data through USB interrupt IN EP.
 * \param pHidd     Pointer to HIDDFunction instance.
 * \param pData     Pointer to the data sent.
 * \param dwLength  The data length.
 * \param fCallback Callback function invoked when transferring done.
 * \param pArg Pointer to additional arguments.
 */
uint32_t HIDDFunction_Write(const HIDDFunction *pHidd,
                            void* pData,
                            uint32_t dwLength,
                            TransferCallback fCallback,
                            void* pArg)
{
    return USBD_Write(pHidd->bPipeIN,
                      pData, dwLength,
                      fCallback, pArg);
}

/**
 * Initialize a report.
 * \param pReport   Pointer to HIDDReport instance.
 * \param wSize     Size of the report data.
 * \param bID       Report ID.
 * \param fCallback Callback function for report events.
 * \param pArg      Pointer to event handler arguments.
 */
void HIDDFunction_InitializeReport(HIDDReport* pReport,
                                   uint16_t wSize,
                                   uint8_t bID,
                                   HIDDReportEventCallback fCallback,
                                   void* pArg)
{
    pReport->wMaxSize = wSize;
    pReport->wTransferred = 0;
    pReport->bIdleRate = 0;
    pReport->bDelay = 0;
    pReport->bID = bID;

    pReport->fCallback = fCallback;
    pReport->pArg = pArg;
}

/**@}*/

