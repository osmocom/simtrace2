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

/**\file
 *
 * Definitions used for general HID support.
 *
 * The HID Function supports following:
 * - 1 interface with 1 pipe in and 1 pipe out;
 * - An input report list;
 * - An output report list;
 * - handles requests:
 *   - handles: GET_IDLE/SET_IDLE,
 *              GET_REPORT/SET_REPORT,
 *              SET_PROTOCOL/GET_PROTOCOL;
 *   - stall  : SET_DESCRIPTOR.
 */

#ifndef _HIDDFUNCTION_H_
#define _HIDDFUNCTION_H_
/** \addtogroup usbd_hid
 * @{
 */

/*----------------------------------------------------------------------------
 *         Includes
 *----------------------------------------------------------------------------*/

#include <stdint.h>

#include <HIDRequests.h>
#include <HIDDescriptors.h>

#include <USBD.h>
#include <USBDDriver.h>

/*----------------------------------------------------------------------------
 *         Definitions
 *----------------------------------------------------------------------------*/

/** \addtogroup usbd_hid_events HIDD Event codes
 *      @{
 */
/** Report sent */
#define HIDD_EC_REPORTSENT          1
/** Report changed */
#define HIDD_EC_REPORTCHANGED       2
/** Report sent because of GET_REPORT Request */
#define HIDD_EC_GETREPORT           3
/** Report changed because of SET_REPORT Request */
#define HIDD_EC_SETREPORT           4
/**     @}*/

/*----------------------------------------------------------------------------
 *         Types
 *----------------------------------------------------------------------------*/

/**
 * Callback function for HID report events.
 */
typedef void(*HIDDReportEventCallback)(uint32_t ec, void *pArg);

/**
 * Struct for a header of basic HID report descriptor.
 */
typedef struct _HIDDReportHeader {
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
} HIDDReportHeader;

/**
 * Struct for an basic HID report descriptor.
 */
typedef struct _HIDDReport {
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
    /** Report data block start ... */
    uint8_t bData[1];
} HIDDReport;

/**
 * Struct for an HID general function.
 * Supports Input/Output reports. No feature report support.
 */
typedef struct _HIDDFunction {
    /** USB Driver for the %device */
    USBDDriver *pUsbd;
    /** HID descriptor */
    HIDDescriptor *pHidDescriptor;
    /** HID Specific report descriptor */
    uint8_t *pReportDescriptor;
    /** USB interface for HID function */
    uint8_t bInterface;
    /** USB interrupt IN EP */
    uint8_t bPipeIN;
    /** USB interrupt OUT EP */
    uint8_t bPipeOUT;

    /** HID Protocol */
    uint8_t bProtocol;

    /** HID Input reports list */
    HIDDReport **pInputList;
    /** HID Output reports list */
    HIDDReport **pOutputList;
    /** HID Input report list size */
    uint8_t bInputListSize;
    /** Current input report */
    uint8_t bCurrInput;
    /** HID Output reports list */
    uint8_t bOutputListSize;
    /** Current output report */
    uint8_t bCurrOutput;
} HIDDFunction;

/*----------------------------------------------------------------------------
 *         Functions
 *----------------------------------------------------------------------------*/

extern void HIDDFunction_Initialize(
    HIDDFunction * pHidd,
    USBDDriver * pUsbd, uint8_t bInterfaceNb,
    const uint8_t * pReportDescriptor,
    HIDDReport * pInputList [ ],  uint8_t bInputListSize,
    HIDDReport * pOutputList [ ], uint8_t bOutputListSize);

extern USBGenericDescriptor* HIDDFunction_ParseInterface(
    HIDDFunction * pHidd,
    USBGenericDescriptor * pDescriptors,
    uint32_t dwLength);

extern uint32_t HIDDFunction_RequestHandler(
    HIDDFunction * pHidd,
    const USBGenericRequest * request);

extern uint32_t HIDDFunction_StartSendingInputs(HIDDFunction * pHidd);

extern uint32_t HIDDFunction_StartPollingOutputs(HIDDFunction * pHidd);

extern uint32_t HIDDFunction_Read(
    const HIDDFunction * pHidd,
    void * pData, uint32_t dwLength,
    TransferCallback fCallback, void * pArg);

extern uint32_t HIDDFunction_Write(
    const HIDDFunction * pHidd,
    void * pData, uint32_t dwLength,
    TransferCallback fCallback, void * pArg);

extern void HIDDFunction_InitializeReport(
    HIDDReport * pReport,
    uint16_t wSize,
    uint8_t bID,
    HIDDReportEventCallback fCallback, void* pArg);

/**@}*/
#endif /* #define _HIDDFUNCTION_H_ */

