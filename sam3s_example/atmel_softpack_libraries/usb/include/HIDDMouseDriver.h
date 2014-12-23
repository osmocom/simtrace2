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

/**\file
 *
 * \section Purpose
 *
 * Definition of methods for using a HID mouse device driver.
 *
 * \section Usage
 *
 * -# Re-implement the USBDCallbacks_RequestReceived callback to forward
 *    requests to HIDDMouseDriver_RequestHandler. This is done
 *    automatically unless the NOAUTOCALLBACK symbol is defined during
 *    compilation.
 * -# Initialize the driver using HIDDMouseDriver_Initialize. The
 *    USB driver is automatically initialized by this method.
 * -# Call the HIDDMouseDriver_ChangePoints method when one or more
 *    keys are pressed/released.
 */

#ifndef HIDDKEYBOARDDRIVER_H
#define HIDDKEYBOARDDRIVER_H

/** \addtogroup usbd_hid_mouse
 *@{
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <USBRequests.h>

#include <HIDDescriptors.h>
#include <HIDRequests.h>

#include <USBDDriver.h>

/*------------------------------------------------------------------------------
 *         Definitions
 *------------------------------------------------------------------------------*/

/** \addtogroup usbd_hid_mouse_button_bitmaps HID Mouse Button bitmaps
 *      @{
 * \section Bits
 * - HIDDMouse_LEFT_BUTTON
 * - HIDDMouse_RIGHT_BUTTON
 * - HIDDMouse_MIDDLE_BUTTON
 */

/** Left mouse button */
#define HIDDMouse_LEFT_BUTTON   (1 << 0)
/** Right mouse button */
#define HIDDMouse_RIGHT_BUTTON  (1 << 1)
/** Middle mouse button */
#define HIDDMouse_MIDDLE_BUTTON (1 << 2)
/**      @}*/

/** Size of the report descriptor in bytes. */
#define HIDDMouseDriver_REPORTDESCRIPTORSIZE              50

/*------------------------------------------------------------------------------
 *         Types
 *------------------------------------------------------------------------------*/

#ifdef __ICCARM__          /* IAR */
#pragma pack(1)            /* IAR */
#define __attribute__(...) /* IAR */
#endif                     /* IAR */

/**
 * \typedef HIDDMouseDriverConfigurationDescriptors
 * \brief List of descriptors that make up the configuration descriptors of a
 *        device using the HID Mouse driver.
 */
typedef struct _HIDDMouseDriverConfigurationDescriptors {

    /** Configuration descriptor. */
    USBConfigurationDescriptor configuration;
    /** Interface descriptor. */
    USBInterfaceDescriptor interface;
    /** HID descriptor. */
    HIDDescriptor1 hid;
    /** Interrupt IN endpoint descriptor. */
    USBEndpointDescriptor interruptIn;

} __attribute__ ((packed)) HIDDMouseDriverConfigurationDescriptors;

/**
 * \typedef HIDDMouseInputReport
 * \brief HID input report data struct used by the Mouse driver to notify the
 *        host of pressed keys.
 */
typedef struct _HIDDMouseInputReport {

    uint8_t bmButtons;          /**< Bitmap state of three mouse buttons. */
    int8_t  bX;                 /**< Pointer displacement along the X axis. */
    int8_t  bY;                 /**< Pointer displacement along the Y axis. */
} __attribute__ ((packed)) HIDDMouseInputReport; /* GCC */

#ifdef __ICCARM__          /* IAR */
#pragma pack()             /* IAR */
#endif                     /* IAR */

/*------------------------------------------------------------------------------
 *         Exported functions
 *------------------------------------------------------------------------------*/

extern void HIDDMouseDriver_Initialize(const USBDDriverDescriptors *pDescriptors);

extern void HIDDMouseDriver_ConfigurationChangedHandler(uint8_t cfgnum);

extern void HIDDMouseDriver_RequestHandler(const USBGenericRequest *request);

extern uint8_t HIDDMouseDriver_ChangePoints(uint8_t bmButtons,
                                            int8_t deltaX,
                                            int8_t deltaY);

extern void HIDDMouseDriver_RemoteWakeUp(void);

/**@}*/

#endif /*#ifndef HIDDKEYBOARDDRIVER_H */

