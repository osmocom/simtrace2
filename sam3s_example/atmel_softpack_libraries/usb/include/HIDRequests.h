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
 * \section Purpose
 *
 * Definitions used for declaring the requests of a HID device.
 *
 * -# Receive a GET_REPORT or SET_REPORT request from the host.
 * -# Retrieve the report type using HIDReportRequest_GetReportType.
 * -# Retrieve the report ID using HIDReportRequest_GetReportId.
 * -# Retrieve the idle rate indicated by a GET_IDLE or SET_IDLE request
 *    with HIDIdleRequest_GetIdleRate.
 */

#ifndef _HIDREQUESTS_H_
#define _HIDREQUESTS_H_
/** \addtogroup usb_hid
 *@{
 */

/*----------------------------------------------------------------------------
 *         Includes
 *----------------------------------------------------------------------------*/

#include <stdint.h>
#include <USBRequests.h>

/*----------------------------------------------------------------------------
 *         Definitions
 *----------------------------------------------------------------------------*/

/** \addtogroup usb_hid_request_codes HID Request Codes
 *      @{
 *
 * \section Codes
 * - HIDGenericRequest_GETREPORT
 * - HIDGenericRequest_GETIDLE
 * - HIDGenericRequest_GETPROTOCOL
 * - HIDGenericRequest_SETREPORT
 * - HIDGenericRequest_SETIDLE
 * - HIDGenericRequest_SETPROTOCOL
 */

/** GetReport request code. */
#define HIDGenericRequest_GETREPORT             0x01
/** GetIdle request code. */
#define HIDGenericRequest_GETIDLE               0x02
/** GetProtocol request code. */
#define HIDGenericRequest_GETPROTOCOL           0x03
/** SetReport request code. */
#define HIDGenericRequest_SETREPORT             0x09
/** SetIdle request code. */
#define HIDGenericRequest_SETIDLE               0x0A
/** SetProtocol request code. */
#define HIDGenericRequest_SETPROTOCOL           0x0B
/**      @}*/

/** \addtogroup usb_hid_report_types HID Report Types
 *      @{
 * This page lists the types for USB HID Reports.
 *
 * \section Types
 * - HIDReportRequest_INPUT
 * - HIDReportRequest_OUTPUT
 * - HIDReportRequest_FEATURE
 */

/** Input report. */
#define HIDReportRequest_INPUT                  1
/** Output report. */
#define HIDReportRequest_OUTPUT                 2
/** Feature report. */
#define HIDReportRequest_FEATURE                3
/**      @}*/

/** \addtogroup usb_hid_protocol_types HID Protocol Types
 *      @{
 */
/** Boot Protocol */
#define HIDProtocol_BOOT                        0
/** Report Protocol */
#define HIDProtocol_REPORT                      1

/** Infinite idle rate.*/
#define HIDIdleRequest_INFINITE                 0

/*----------------------------------------------------------------------------
 *         Types
 *----------------------------------------------------------------------------*/

#ifdef __ICCARM__          /* IAR */
#pragma pack(1)            /* IAR */
#define __attribute__(...) /* IAR */
#endif                     /* IAR */



#ifdef __ICCARM__          /* IAR */
#pragma pack()             /* IAR */
#endif                     /* IAR */

/*----------------------------------------------------------------------------
 *         Functions
 *----------------------------------------------------------------------------*/

extern uint8_t HIDReportRequest_GetReportType(
    const USBGenericRequest *request);

extern uint8_t HIDReportRequest_GetReportId(
    const USBGenericRequest *request);


static inline uint16_t HIDProtocolRequest_GetProtocol(
    const USBGenericRequest *request)
{
    return USBGenericRequest_GetValue(request);
}

extern uint8_t HIDIdleRequest_GetReportId(
    const USBGenericRequest * request);

extern uint8_t HIDIdleRequest_GetIdleRate(
    const USBGenericRequest *request);

/**@}*/
#endif /* #define _HIDREQUESTS_H_ */
