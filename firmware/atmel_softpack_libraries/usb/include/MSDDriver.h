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
 * \section Purpose
 * 
 * Mass storage %device driver implementation.
 * 
 * \section Usage
 * 
 * -# Enable and setup USB related pins (see pio & board.h).
 * -# Configure the memory interfaces used for Mass Storage LUNs
 *    (see memories, MSDLun.h).
 * -# Instance the USB device configure descriptor as
 *    MSDConfigurationDescriptors or MSDConfigurationDescriptorsOTG defined.
 *    Interface number should be 0.
 * -# Configure the USB MSD %driver using MSDDriver_Initialize.
 * -# Invoke MSDDriver_StateMachine in main loop to handle all Mass Storage
 *    operations.
 */

#ifndef MSDDRIVER_H
#define MSDDRIVER_H

/** \addtogroup usbd_msd 
 *@{
 */
 
/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <stdint.h>

#include <MSDLun.h>
#include <MSD.h>
#include <MSDDStateMachine.h>
#include <MSDFunction.h>

/*------------------------------------------------------------------------------
 *         Types
 *------------------------------------------------------------------------------*/

#ifdef __ICCARM__          /* IAR */
#pragma pack(1)            /* IAR */
#define __attribute__(...) /* IAR */
#endif                     /* IAR */

/**
 * \typedef MSDConfigurationDescriptors
 * \brief List of configuration descriptors used by a Mass Storage device driver.
 */
typedef struct _MSDConfigurationDescriptors {

    /** Standard configuration descriptor. */
    USBConfigurationDescriptor configuration;
    /** Mass storage interface descriptor. */
    USBInterfaceDescriptor interface;
    /** Bulk-out endpoint descriptor. */
    USBEndpointDescriptor bulkOut;
    /** Bulk-in endpoint descriptor. */
    USBEndpointDescriptor bulkIn;

} __attribute__ ((packed)) MSDConfigurationDescriptors;

/**
 * \typedef MSDConfigurationDescriptorsOTG
 * \brief List of configuration descriptors used by a
 *        Mass Storage device driver, with OTG support.
 */
typedef struct _MSDConfigurationDescriptorsOTG {

    /** Standard configuration descriptor. */
    USBConfigurationDescriptor configuration;
    /* OTG descriptor */
    USBOtgDescriptor otgDescriptor;
    /** Mass storage interface descriptor. */
    USBInterfaceDescriptor interface;
    /** Bulk-out endpoint descriptor. */
    USBEndpointDescriptor bulkOut;
    /** Bulk-in endpoint descriptor. */
    USBEndpointDescriptor bulkIn;

} __attribute__ ((packed)) MSDConfigurationDescriptorsOTG;

#ifdef __ICCARM__          /* IAR */
#pragma pack()             /* IAR */
#endif                     /* IAR */

/*------------------------------------------------------------------------------
 *      Global functions
 *------------------------------------------------------------------------------*/

extern void MSDDriver_Initialize(
    const USBDDriverDescriptors *pDescriptors,
    MSDLun *luns, uint8_t numLuns);

extern void MSDDriver_RequestHandler(
    const USBGenericRequest *request);

extern void MSDDriver_ConfigurationChangeHandler(
    uint8_t cfgnum);

/**
 * State machine for the MSD driver
 * \param  pMsdDriver  Pointer to MSDDriver instance.
 */
static inline void MSDDriver_StateMachine(void) {
    MSDFunction_StateMachine();
}

/**@}*/

#endif /* #ifndef MSDDRIVER_H */

