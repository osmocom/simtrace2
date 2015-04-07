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

/**
 *\file
 *
 *\section Purpose
 *
 *Definition of methods for using a HID transfer %device driver.
 *
 *\section Usage
 *
 *-# Re-implement the USBDCallbacks_RequestReceived callback to forward
 *      requests to HIDDTransferDriver_RequestHandler. This is done
 *      automatically unless the NOAUTOCALLBACK symbol is defined during
 *      compilation.
 *-# Initialize the driver using HIDDTransferDriver_Initialize. The
 *   USB driver is automatically initialized by this method.
 *-# Call the HIDDTransferDriver_Write method when sendint data to host.
 *-# Call the HIDDTransferRead, HIDDTransferReadReport when checking and getting
 *   received data from host.
 */

#ifndef HIDDKEYBOARDDRIVER_H
#define HIDDKEYBOARDDRIVER_H

/** \addtogroup usbd_hid_tran
 *@{
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <USBDescriptors.h>
#include <HIDDescriptors.h>

#include <USBRequests.h>

#include <USBD.h>
#include <USBDDriver.h>

/*------------------------------------------------------------------------------
 *         Definitions
 *------------------------------------------------------------------------------*/
    
/** Size of the input and output report, in bytes */
#define HIDDTransferDriver_REPORTSIZE               32

/** Size of the report descriptor, in bytes */
#define HIDDTransferDriver_REPORTDESCRIPTORSIZE     32

/*------------------------------------------------------------------------------
 *         Types
 *------------------------------------------------------------------------------*/

#ifdef __ICCARM__          /* IAR */
#pragma pack(1)            /* IAR */
#define __attribute__(...) /* IAR */
#endif                     /* IAR */

/**
 * \typedef HIDDTransferDriverConfigurationDescriptors
 * \brief List of descriptors that make up the configuration descriptors of a
 *        device using the HID Transfer driver.
 */
typedef struct _HIDDTransferDriverConfigurationDescriptors {

    /** Configuration descriptor. */
    USBConfigurationDescriptor configuration;
    /** Interface descriptor. */
    USBInterfaceDescriptor interface;
    /** HID descriptor. */
    HIDDescriptor1 hid;
    /** Interrupt IN endpoint descriptor. */
    USBEndpointDescriptor interruptIn;
    /** Interrupt OUT endpoint descriptor. */
    USBEndpointDescriptor interruptOut;

} __attribute__ ((packed)) HIDDTransferDriverConfigurationDescriptors;

#ifdef __ICCARM__          /* IAR */
#pragma pack()             /* IAR */
#endif                     /* IAR */

/*------------------------------------------------------------------------------
 *         Exported functions
 *------------------------------------------------------------------------------*/

extern void HIDDTransferDriver_Initialize(
    const USBDDriverDescriptors * pDescriptors);

extern void HIDDTransferDriver_ConfigurationChangedHandler(uint8_t cfgnum);

extern void HIDDTransferDriver_RequestHandler(
    const USBGenericRequest *request);

extern uint16_t HIDDTransferDriver_Read(void *pData,
                                        uint32_t dLength);

extern uint16_t HIDDTransferDriver_ReadReport(void *pData,
                                              uint32_t dLength);

extern uint8_t HIDDTransferDriver_Write(const void *pData,
                                        uint32_t size,
                                        TransferCallback callback,
                                        void *pArg);


extern void HIDDTransferDriver_RemoteWakeUp(void);

/**@}*/

#endif /*#ifndef HIDDKEYBOARDDRIVER_H*/

