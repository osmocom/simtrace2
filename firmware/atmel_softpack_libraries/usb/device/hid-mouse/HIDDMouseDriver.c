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
 * \addtogroup usbd_hid_mouse
 *@{
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <HIDDFunction.h>
#include <HIDDMouseDriver.h>

#include <USBD.h>
#include <USBD_HAL.h>
#include <USBDDriver.h>

#include <USBLib_Trace.h>

/*------------------------------------------------------------------------------
 *         Internal Defines
 *------------------------------------------------------------------------------*/

/** Tag bit (Always 1) */
#define HIDDMouse_TAG       (1 << 3)
/** Xsign bit */
#define HIDDMouse_Xsign     (1 << 4)
/** Ysign bit */
#define HIDDMouse_Ysign     (1 << 5)

/*------------------------------------------------------------------------------
 *         Internal types
 *------------------------------------------------------------------------------*/

/**
 * Struct for an HID Mouse report.
 */
typedef struct _HIDDMouseReport {
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
    /** Report data block */
    HIDDMouseInputReport report;
} HIDDMouseReport;

/**
 * Driver structure for an HID device implementing keyboard functionalities.
 */
typedef struct _HIDDMouseDriver {

    /** Mouse function instance */
    HIDDFunction hidDrv;
    /** Mouse input report */
    HIDDReport   *inputReports[1];
} HIDDMouseDriver;

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/

/** Static instance of the HID mouse device driver. */
static HIDDMouseDriver hiddMouseDriver;
/** Input report */
static HIDDMouseReport hiddInputReport;

/** Report descriptor used by the driver. */
static const uint8_t hiddReportDescriptor[] = {

    /* Global Usage Page */
    HIDReport_GLOBAL_USAGEPAGE + 1, HIDGenericDesktop_PAGEID,
    /* Collection: Application */
    HIDReport_LOCAL_USAGE + 1, HIDGenericDesktop_MOUSE,
    HIDReport_COLLECTION + 1, HIDReport_COLLECTION_APPLICATION,
        /* Physical collection: Pointer */
        HIDReport_LOCAL_USAGE + 1, HIDGenericDesktop_POINTER,
        HIDReport_COLLECTION + 1, HIDReport_COLLECTION_PHYSICAL,

            /* Input report: buttons */
            HIDReport_GLOBAL_USAGEPAGE + 1, HIDButton_PAGEID,

            HIDReport_GLOBAL_REPORTCOUNT + 1, 3,
            HIDReport_GLOBAL_REPORTSIZE + 1, 1,
            HIDReport_LOCAL_USAGEMINIMUM + 1, 1,
            HIDReport_LOCAL_USAGEMAXIMUM + 1, 3,
            HIDReport_GLOBAL_LOGICALMINIMUM + 1, 0,
            HIDReport_GLOBAL_LOGICALMAXIMUM + 1, 1,
            HIDReport_INPUT + 1, HIDReport_VARIABLE,    /* 3 button bits */

            /* Input report: padding */
            HIDReport_GLOBAL_REPORTCOUNT + 1, 1,
            HIDReport_GLOBAL_REPORTSIZE + 1, 5,
            HIDReport_INPUT + 1, HIDReport_CONSTANT,    /* 5 bit padding */

            /* Input report: pointer */
            HIDReport_GLOBAL_USAGEPAGE + 1, HIDGenericDesktop_PAGEID,
            HIDReport_GLOBAL_REPORTSIZE + 1, 8,
            HIDReport_GLOBAL_REPORTCOUNT + 1, 2,
            HIDReport_LOCAL_USAGE + 1, HIDGenericDesktop_X,
            HIDReport_LOCAL_USAGE + 1, HIDGenericDesktop_Y,
            HIDReport_GLOBAL_LOGICALMINIMUM + 1, (uint8_t) -127,
            HIDReport_GLOBAL_LOGICALMAXIMUM + 1, 127,
            HIDReport_INPUT + 1, HIDReport_VARIABLE | HIDReport_RELATIVE,

        HIDReport_ENDCOLLECTION,
    HIDReport_ENDCOLLECTION
};

/*------------------------------------------------------------------------------
 *         Internal functions
 *------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *      Exported functions
 *------------------------------------------------------------------------------*/

/**
 * Initializes the HID Mouse %device driver.
 * \param pDescriptors Pointer to descriptor list for the HID Mouse.
 */
void HIDDMouseDriver_Initialize(const USBDDriverDescriptors *pDescriptors)
{
    HIDDMouseDriver* pMouse = &hiddMouseDriver;
    HIDDFunction* pHidd = &pMouse->hidDrv;
    USBDDriver* pUsbd = USBD_GetDriver();

    /* One input report */
    pMouse->inputReports[0] = (HIDDReport*)&hiddInputReport;
    HIDDFunction_InitializeReport(pMouse->inputReports[0],
                                  HIDDMouseDriver_REPORTDESCRIPTORSIZE,
                                  0,
                                  0, 0);

    /* Initialize USBD Driver instance */
    USBDDriver_Initialize(pUsbd,
                          pDescriptors,
                          0); /* Multiple interface settings not supported */
    /* Function initialize */
    HIDDFunction_Initialize(pHidd,
                            pUsbd, 0,
                            hiddReportDescriptor,
                            (HIDDReport**)(&pMouse->inputReports), 1,
                            0, 0);
    USBD_Init();
}

/**
 * Handles configureation changed event.
 * \param cfgnum New configuration number
 */
void HIDDMouseDriver_ConfigurationChangedHandler(uint8_t cfgnum)
{
    HIDDMouseDriver * pMouse = &hiddMouseDriver;
    HIDDFunction * pHidd = &pMouse->hidDrv;
    USBDDriver * pUsbd = pHidd->pUsbd;
    USBConfigurationDescriptor *pDesc;

    if (cfgnum > 0) {

        /* Parse endpoints for reports */
        pDesc = USBDDriver_GetCfgDescriptors(pUsbd, cfgnum);
        HIDDFunction_ParseInterface(pHidd,
                                    (USBGenericDescriptor*)pDesc,
                                    pDesc->wTotalLength);
    }
}

/**
 * Handles HID-specific SETUP request sent by the host.
 * \param request Pointer to a USBGenericRequest instance
 */
void HIDDMouseDriver_RequestHandler(const USBGenericRequest *request)
{
    HIDDMouseDriver * pMouse = &hiddMouseDriver;
    HIDDFunction * pHidd = &pMouse->hidDrv;
    USBDDriver * pUsbd = pHidd->pUsbd;
    
    TRACE_INFO("NewReq ");

    /* Process HID requests */
    if (USBRC_SUCCESS == HIDDFunction_RequestHandler(pHidd,
                                                     request)) {
        return;
    }
    /* Process STD requests */
    else {

        USBDDriver_RequestHandler(pUsbd, request);
    }

}

/**
 * Update the Mouse button status and location changes via input report
 * to host
 * \param bmButtons Bit map of the button status
 * \param deltaX Movment on X direction
 * \param deltaY Movment on Y direction
 */
uint8_t HIDDMouseDriver_ChangePoints(uint8_t bmButtons,
                                     int8_t deltaX,
                                     int8_t deltaY)
{
    HIDDMouseDriver * pMouse = &hiddMouseDriver;
    HIDDFunction * pHidd = &pMouse->hidDrv;
    HIDDMouseInputReport * pReport = &hiddInputReport.report;

    pReport->bmButtons = (bmButtons & 0x07) | HIDDMouse_TAG;
    pReport->bX        = deltaX;
    pReport->bY        = deltaY;

    /* Send input report through the interrupt IN endpoint */
    return USBD_Write(pHidd->bPipeIN,
                      (void*)pReport,
                      sizeof(HIDDMouseInputReport),
                      0,
                      0);
}

/**
 * Starts a remote wake-up sequence if the host has explicitely enabled it
 * by sending the appropriate SET_FEATURE request.
 */
void HIDDMouseDriver_RemoteWakeUp(void)
{
    HIDDMouseDriver * pMouse = &hiddMouseDriver;
    HIDDFunction * pHidd = &pMouse->hidDrv;
    USBDDriver * pUsbd = pHidd->pUsbd;

    /* Remote wake-up has been enabled */
    if (USBDDriver_IsRemoteWakeUpEnabled(pUsbd)) {

        USBD_RemoteWakeUp();
    }
}

/**@}*/

