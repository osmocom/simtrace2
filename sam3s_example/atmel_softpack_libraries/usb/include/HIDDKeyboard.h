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
 * Definition of methods for using a HID keyboard function.
 */

#ifndef HIDDKEYBOARD_H
#define HIDDKEYBOARD_H

/** \addtogroup usbd_hid_key
 *@{
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include "board.h"

#include "USBDescriptors.h"
#include "USBRequests.h"

#include "HIDDescriptors.h"
#include "USBDDriver.h"

/*------------------------------------------------------------------------------
 *         Definitions
 *------------------------------------------------------------------------------*/

/** \addtogroup usbd_hid_kbd_desc HIDD Keyboard Driver Definitions
 *  @{
 */

/** Maximum number of simultaneous key presses. */
#define HIDDKeyboardInputReport_MAXKEYPRESSES           3

/** \addtogroup usbd_hid_kbd_keys HID Keypad keys
 *      @{
 * This page lists definition for HID keypad keys.
 *
 * \section Keys
 * - HIDDKeyboardDescriptors_FIRSTMODIFIERKEY
 * - HIDDKeyboardDescriptors_LASTMODIFIERKEY
 * - HIDDKeyboardDescriptors_FIRSTSTANDARDKEY
 * - HIDDKeyboardDescriptors_LASTSTANDARDKEY
 */
/** Key code of the first accepted modifier key */
#define HIDDKeyboardDescriptors_FIRSTMODIFIERKEY  HIDKeypad_LEFTCONTROL
/** Key code of the last accepted modifier key */
#define HIDDKeyboardDescriptors_LASTMODIFIERKEY   HIDKeypad_RIGHTGUI
/** Key code of the first accepted standard key */
#define HIDDKeyboardDescriptors_FIRSTSTANDARDKEY  0
/** Key code of the last accepted standard key */
#define HIDDKeyboardDescriptors_LASTSTANDARDKEY   HIDKeypad_NUMLOCK
/**      @}*/

/** \addtogroup usbd_hid_kbd_polling HID Keyboard Default Polling Rates
 *      @{
 */
/** Default HID interrupt IN endpoint polling rate FS (16ms). */
#define HIDDKeyboardDescriptors_INTERRUPTIN_POLLING_FS     16
/** Default HID interrupt IN endpoint polling rate HS (16ms). */
#define HIDDKeyboardDescriptors_INTERRUPTIN_POLLING_HS     8
/** Default interrupt OUT endpoint polling rate FS (16ms). */
#define HIDDKeyboardDescriptors_INTERRUPTOUT_POLLING_FS    16
/** Default interrupt OUT endpoint polling rate HS (16ms). */
#define HIDDKeyboardDescriptors_INTERRUPTOUT_POLLING_HS    8
/**     @}*/

/** Size of the report descriptor in bytes */
#define HIDDKeyboard_REPORTDESCRIPTORSIZE                   61

/** @}*/

/*------------------------------------------------------------------------------
 *         Types
 *------------------------------------------------------------------------------*/

#ifdef __ICCARM__          /* IAR */
#pragma pack(1)            /* IAR */
#define __attribute__(...) /* IAR */
#endif                     /* IAR */

/**
 * \typedef HIDDKeyboardOutputReport
 * \brief HID output report structure used by the host to control the state of
 *        the keyboard LEDs.
 *
 * Only the first three bits are relevant, the other 5 are used as
 * padding bits.
 */
typedef struct _HIDDKeyboardOutputReport {

    uint8_t numLockStatus:1,      /** State of the num. lock LED. */
                  capsLockStatus:1,     /** State of the caps lock LED. */
                  scrollLockStatus:1,   /** State of the scroll lock LED. */
                  padding:5;            /** Padding bits. */

} __attribute__ ((packed)) HIDDKeyboardOutputReport; /* GCC */

/**
 * \typedef HIDDKeyboardInputReport
 * \brief HID input report structure used by the keyboard driver to notify the
 *        host of pressed keys.
 *
 * The first byte is used to report the state of modifier keys. The
 * other three contains the keycodes of the currently pressed keys.
 */
typedef struct _HIDDKeyboardInputReport {

    /** State of modifier keys. */
    uint8_t bmModifierKeys:8;
    /** Key codes of pressed keys. */
    uint8_t pressedKeys[HIDDKeyboardInputReport_MAXKEYPRESSES];
} __attribute__ ((packed)) HIDDKeyboardInputReport; /* GCC */


/**
 * \typedef HIDDKeyboardDriverConfigurationDescriptors
 * \brief List of descriptors that make up the configuration descriptors of a
 *        device using the HID keyboard driver.
 */
typedef struct _HIDDKeyboardDriverConfigurationDescriptors {

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

} __attribute__ ((packed)) HIDDKeyboardDriverConfigurationDescriptors;

#ifdef __ICCARM__          /* IAR */
#pragma pack()             /* IAR */
#endif                     /* IAR */

/*------------------------------------------------------------------------------
 *         Exported functions
 *------------------------------------------------------------------------------*/

extern void HIDDKeyboard_Initialize(USBDDriver * pUsbd,uint8_t bInterfaceNb);

extern void HIDDKeyboard_ConfigureFunction( USBGenericDescriptor * pDescriptors, uint16_t wLength);

extern uint32_t HIDDKeyboard_RequestHandler( const USBGenericRequest *request);

extern uint32_t HIDDKeyboard_ChangeKeys(
    uint8_t *pressedKeys,
    uint8_t pressedKeysSize,
    uint8_t *releasedKeys,
    uint8_t releasedKeysSize);

extern void HIDDKeyboard_RemoteWakeUp(void);

extern void HIDDKeyboardCallbacks_LedsChanged(
    uint8_t numLockStatus,
    uint8_t capsLockStatus,
    uint8_t scrollLockStatus);


extern void HIDDKeyboardInputReport_Initialize(HIDDKeyboardInputReport *report);

extern void HIDDKeyboardInputReport_PressStandardKey( HIDDKeyboardInputReport *report, uint8_t key ) ;

extern void HIDDKeyboardInputReport_ReleaseStandardKey( HIDDKeyboardInputReport *report, uint8_t key ) ;

extern void HIDDKeyboardInputReport_PressModifierKey( HIDDKeyboardInputReport *report, uint8_t key);

extern void HIDDKeyboardInputReport_ReleaseModifierKey( HIDDKeyboardInputReport *report, uint8_t key);


extern void HIDDKeyboardOutputReport_Initialize( HIDDKeyboardOutputReport *report);

extern uint8_t HIDDKeyboardOutputReport_GetNumLockStatus( const HIDDKeyboardOutputReport *report);

extern uint8_t HIDDKeyboardOutputReport_GetCapsLockStatus( const HIDDKeyboardOutputReport *report);

extern uint8_t HIDDKeyboardOutputReport_GetScrollLockStatus( const HIDDKeyboardOutputReport *report);

/**@}*/

#endif /*#ifndef HIDDKEYBOARD_H*/

