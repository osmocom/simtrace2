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
 * Implement HID Keyboard Function For USB Device.
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <HIDDKeyboard.h>
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

/**
 * Extended struct for an HID Keyboard Input report, for transfer driver to
 * send reports.
 */
typedef struct _KBDInputReport {
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
    /** Input Report Data Block */
    HIDDKeyboardInputReport sReport;
} KBDInputReport;

/**
 * Extended struct for an HID Keyboard Output report, for transfer driver to
 * polling reports.
 */
typedef struct _KBDOutputReport {
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
    /** Output Report Data Block */
    HIDDKeyboardOutputReport sReport;
} KBDOutputReport;

/**
 *  Driver structure for an HID device implementing keyboard functionalities.
 */
typedef struct _HIDDKeyboard {

    /** USB HID Functionn */
    HIDDFunction hidDrv;
    /** Input report list */
    HIDDReport *inputReports[1];
    /** Output report list */
    HIDDReport *outputReports[1];
} HIDDKeyboard;

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/

/** HIDD Keyboard Input Report Instance */
static KBDInputReport inputReport;

/** HIDD Keyboard Output Report Instance */
static KBDOutputReport outputReport;

/** Static instance of the HIDD keyboard device driver. */
static HIDDKeyboard hiddKeyboard;

/** Report descriptor used by the driver. */
const uint8_t hiddKeyboardReportDescriptor[] = {

    HIDReport_GLOBAL_USAGEPAGE + 1, HIDGenericDesktop_PAGEID,
    HIDReport_LOCAL_USAGE + 1, HIDGenericDesktop_KEYBOARD,
    HIDReport_COLLECTION + 1, HIDReport_COLLECTION_APPLICATION,

        /* Input report: modifier keys */
        HIDReport_GLOBAL_REPORTSIZE + 1, 1,
        HIDReport_GLOBAL_REPORTCOUNT + 1, 8,
        HIDReport_GLOBAL_USAGEPAGE + 1, HIDKeypad_PAGEID,
        HIDReport_LOCAL_USAGEMINIMUM + 1,
            HIDDKeyboardDescriptors_FIRSTMODIFIERKEY,
        HIDReport_LOCAL_USAGEMAXIMUM + 1,
            HIDDKeyboardDescriptors_LASTMODIFIERKEY,
        HIDReport_GLOBAL_LOGICALMINIMUM + 1, 0,
        HIDReport_GLOBAL_LOGICALMAXIMUM + 1, 1,
        HIDReport_INPUT + 1, HIDReport_VARIABLE,

        /* Input report: standard keys */
        HIDReport_GLOBAL_REPORTCOUNT + 1, 3,
        HIDReport_GLOBAL_REPORTSIZE + 1, 8,
        HIDReport_GLOBAL_LOGICALMINIMUM + 1,
            HIDDKeyboardDescriptors_FIRSTSTANDARDKEY,
        HIDReport_GLOBAL_LOGICALMAXIMUM + 1,
            HIDDKeyboardDescriptors_LASTSTANDARDKEY,
        HIDReport_GLOBAL_USAGEPAGE + 1, HIDKeypad_PAGEID,
        HIDReport_LOCAL_USAGEMINIMUM + 1,
            HIDDKeyboardDescriptors_FIRSTSTANDARDKEY,
        HIDReport_LOCAL_USAGEMAXIMUM + 1,
            HIDDKeyboardDescriptors_LASTSTANDARDKEY,
        HIDReport_INPUT + 1, 0 /* Data array */,

        /* Output report: LEDs */
        HIDReport_GLOBAL_REPORTCOUNT + 1, 3,
        HIDReport_GLOBAL_REPORTSIZE + 1, 1,
        HIDReport_GLOBAL_USAGEPAGE + 1, HIDLeds_PAGEID,
        HIDReport_GLOBAL_LOGICALMINIMUM + 1, 0,
        HIDReport_GLOBAL_LOGICALMAXIMUM + 1, 1,
        HIDReport_LOCAL_USAGEMINIMUM + 1, HIDLeds_NUMLOCK,
        HIDReport_LOCAL_USAGEMAXIMUM + 1, HIDLeds_SCROLLLOCK,
        HIDReport_OUTPUT + 1, HIDReport_VARIABLE,

        /* Output report: padding */
        HIDReport_GLOBAL_REPORTCOUNT + 1, 1,
        HIDReport_GLOBAL_REPORTSIZE + 1, 5,
        HIDReport_OUTPUT + 1, HIDReport_CONSTANT,

    HIDReport_ENDCOLLECTION
};

/*------------------------------------------------------------------------------
 *         Internal functions
 *------------------------------------------------------------------------------*/

/**
 * Callback invoked when an output report has been received from the host.
 * Forward the new status of the LEDs to the user program via the
 * HIDDKeyboardCallbacks_LedsChanged callback.
 */
static void HIDDKeyboard_ReportReceived(void)
{
    HIDDKeyboardOutputReport *pOut = &outputReport.sReport;

    /* Trigger callback */
    if (HIDDKeyboardCallbacks_LedsChanged) {
        HIDDKeyboardCallbacks_LedsChanged(
            pOut->numLockStatus,
            pOut->capsLockStatus,
            pOut->scrollLockStatus);
    }
}

/*------------------------------------------------------------------------------
 *      Exported functions
 *------------------------------------------------------------------------------*/

/**
 * Initializes the HID keyboard device driver SW.
 * (Init USBDDriver .., Init function driver .., Init USBD ...)
 * \param pUsbd        Pointer to USBDDriver instance.
 * \param bInterfaceNb Interface number for the function.
 */
void HIDDKeyboard_Initialize(USBDDriver* pUsbd, uint8_t bInterfaceNb)
{
    HIDDKeyboard *pKbd = &hiddKeyboard;
    HIDDFunction *pHidd = &pKbd->hidDrv;

    /* One input report */
    pKbd->inputReports[0] = (HIDDReport*)&inputReport;
    HIDDFunction_InitializeReport(pKbd->inputReports[0],
                                  sizeof(HIDDKeyboardInputReport),
                                  0,
                                  0, 0);
    /* One output report */
    pKbd->outputReports[0] = (HIDDReport*)&outputReport;
    HIDDFunction_InitializeReport(
        pKbd->outputReports[0],
        sizeof(HIDDKeyboardOutputReport),
        0,
        (HIDDReportEventCallback)HIDDKeyboard_ReportReceived, 0);

    /* Function initialize */
    HIDDFunction_Initialize(pHidd,
                            pUsbd, bInterfaceNb,
                            hiddKeyboardReportDescriptor,
                            pKbd->inputReports, 1,
                            pKbd->outputReports, 1);
}

/**
 * Configure function with expected descriptors and start functionality.
 * Usually invoked when device is configured.
 * \pDescriptors Pointer to the descriptors for function configure.
 * \wLength      Length of descriptors in number of bytes.
 */
void HIDDKeyboard_ConfigureFunction(USBGenericDescriptor *pDescriptors,
                                    uint16_t wLength)
{
    HIDDKeyboard *pKbd = &hiddKeyboard;
    HIDDFunction *pHidd = &pKbd->hidDrv;
    USBGenericDescriptor * pDesc = pDescriptors;

    pDesc = HIDDFunction_ParseInterface(pHidd,
                                        pDescriptors,
                                        wLength);

    /* Start receiving output reports */
    HIDDFunction_StartPollingOutputs(pHidd);
}

/**
 * Handles HID-specific SETUP request sent by the host.
 * \param request Pointer to a USBGenericRequest instance.
 * \return USBRC_SUCCESS if request is handled.
 */
uint32_t HIDDKeyboard_RequestHandler(const USBGenericRequest *request)
{
    HIDDKeyboard *pKbd = &hiddKeyboard;
    HIDDFunction *pHidd = &pKbd->hidDrv;

    TRACE_INFO_WP("Kbd ");

    /* Process HID requests */
    return HIDDFunction_RequestHandler(pHidd, request);
}

/**
 * Reports a change in which keys are currently pressed or release to the
 * host.
 *
 * \param pressedKeys Pointer to an array of key codes indicating keys that have
 *                    been pressed since the last call to
 *                    HIDDKeyboardDriver_ChangeKeys().
 * \param pressedKeysSize Number of key codes in the pressedKeys array.
 * \param releasedKeys Pointer to an array of key codes indicates keys that have
 *                     been released since the last call to
 *                     HIDDKeyboardDriver_ChangeKeys().
 * \param releasedKeysSize Number of key codes in the releasedKeys array.
 * \return USBD_STATUS_SUCCESS if the report has been sent to the host;
 *        otherwise an error code.
 */
uint32_t HIDDKeyboard_ChangeKeys(uint8_t *pressedKeys,
                                 uint8_t pressedKeysSize,
                                 uint8_t *releasedKeys,
                                 uint8_t releasedKeysSize)
{
    HIDDKeyboard *pKbd = &hiddKeyboard;
    HIDDFunction *pHidd = &pKbd->hidDrv;
    HIDDKeyboardInputReport *pReport =
        (HIDDKeyboardInputReport *)pKbd->inputReports[0]->bData;

    /* Press keys */
    while (pressedKeysSize > 0) {

        /* Check if this is a standard or modifier key */
        if (HIDKeypad_IsModifierKey(*pressedKeys)) {

            /* Set the corresponding bit in the input report */
            HIDDKeyboardInputReport_PressModifierKey(
                pReport,
                *pressedKeys);
        }
        else {

            HIDDKeyboardInputReport_PressStandardKey(
                pReport,
                *pressedKeys);
        }

        pressedKeysSize--;
        pressedKeys++;
    }

    /* Release keys */
    while (releasedKeysSize > 0) {

        /* Check if this is a standard or modifier key */
        if (HIDKeypad_IsModifierKey(*releasedKeys)) {

            /* Set the corresponding bit in the input report */
            HIDDKeyboardInputReport_ReleaseModifierKey(
                pReport,
                *releasedKeys);
        }
        else {

            HIDDKeyboardInputReport_ReleaseStandardKey(
                pReport,
                *releasedKeys);
        }

        releasedKeysSize--;
        releasedKeys++;
    }

    /* Send input report through the interrupt IN endpoint */
    return USBD_Write(pHidd->bPipeIN,
                      pReport,
                      sizeof(HIDDKeyboardInputReport),
                      0,
                      0);
}

/**
 * Starts a remote wake-up sequence if the host has explicitely enabled it
 * by sending the appropriate SET_FEATURE request.
 */
void HIDDKeyboard_RemoteWakeUp(void)
{
    HIDDKeyboard *pKbd = &hiddKeyboard;
    HIDDFunction *pHidd = &pKbd->hidDrv;
    USBDDriver *pUsbd = pHidd->pUsbd;

    /* Remote wake-up has been enabled */
    if (USBDDriver_IsRemoteWakeUpEnabled(pUsbd)) {

        USBD_RemoteWakeUp();
    }
}

/**@}*/
