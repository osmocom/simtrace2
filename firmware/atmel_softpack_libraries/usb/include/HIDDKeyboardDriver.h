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
 * \file
 * 
 * \section Purpose
 * 
 * Definition of methods for using a HID keyboard device driver.
 * 
 * \section Usage
 * 
 * -# Re-implement the USBDCallbacks_RequestReceived callback to forward
 *    requests to HIDDKeyboardDriver_RequestHandler. This is done
 *    automatically unless the NOAUTOCALLBACK symbol is defined during
 *    compilation.
 * -# Initialize the driver using HIDDKeyboardDriver_Initialize. The
 *    USB driver is automatically initialized by this method.
 * -# Call the HIDDKeyboardDriver_ChangeKeys method when one or more
 *    keys are pressed/released.
 */

#ifndef HIDDKEYBOARDDRIVER_H
#define HIDDKEYBOARDDRIVER_H

/** \addtogroup usbd_hid_key
 *@{
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <USBDescriptors.h>
#include <USBRequests.h>

#include <HIDDescriptors.h>

#include <HIDDKeyboard.h>

#include <USBDDriver.h>

/*------------------------------------------------------------------------------
 *         Definitions
 *------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *         Types
 *------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *         Exported functions
 *------------------------------------------------------------------------------*/

extern void HIDDKeyboardDriver_Initialize(
    const USBDDriverDescriptors *pDescriptors);

extern void HIDDKeyboardDriver_ConfigurationChangedHandler(
    uint8_t cfgnum);

extern void HIDDKeyboardDriver_RequestHandler(
    const USBGenericRequest *request);

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
static inline uint32_t HIDDKeyboardDriver_ChangeKeys(
    uint8_t *pressedKeys,
    uint8_t pressedKeysSize,
    uint8_t *releasedKeys,
    uint8_t releasedKeysSize){
    return HIDDKeyboard_ChangeKeys(pressedKeys, pressedKeysSize,
                                   releasedKeys, releasedKeysSize);
}

/**
 * Starts a remote wake-up sequence if the host has explicitely enabled it
 * by sending the appropriate SET_FEATURE request.
 */
static inline void HIDDKeyboardDriver_RemoteWakeUp(void) {
    HIDDKeyboard_RemoteWakeUp();
}

/**@}*/

#endif /*#ifndef HIDDKEYBOARDDRIVER_H*/

