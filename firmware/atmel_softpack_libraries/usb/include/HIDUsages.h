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
 * Definitions used for declaring the usages of a HID device.
 *
 */

#ifndef _HIDUSAGES_H_
#define _HIDUSAGES_H_
/** \addtogroup usb_hid
 *@{
 */

/*----------------------------------------------------------------------------
 *         Includes
 *----------------------------------------------------------------------------*/

#include <stdint.h>


/*----------------------------------------------------------------------------
 *         Defines
 *----------------------------------------------------------------------------*/

/** \addtogroup usb_hid_usage_pages HID Usage Pages' IDs
 *      @{
 *  (HUT section 3)
 */

/** ID for the HID Generic Desktop Controls. */
#define HIDUsage_GENERICDESKTOP                 1
/** ID for the HID Game Controls. */
#define HIDUsage_GAME                           5
/** ID for the HID Generic Device Controls. */
#define HIDUsage_GENERICDEVICE                  6
/** ID for the HID Keyboard/Keypad */
#define HIDUsage_KEYBOARD                       7
/** ID for the HID LEDs. */
#define HIDUsage_LEDS                           8
/** ID for the HID buttons. */
#define HIDUsage_BUTTON                         9
/** ID for Bar Code Scanner page. */
#define HIDUsage_BARCODE                        0x8C
/** ID for Camera Control Page. */
#define HIDUsage_CAMERA                         0x90
/** ID for vendor-defined controls. */
#define HIDUsage_VENDOR                         0xFF
/**      @}*/

/** \addtogroup usb_hid_genericdesktop_page_id HID GenericDesktop Page ID
 *      @{
 *
 * \section ID
 * - HIDGenericDesktop_PAGEID
 */

/** ID for the HID generic desktop usage page. */
#define HIDGenericDesktop_PAGEID            0x01
/**      @}*/

/** \addtogroup usb_hid_genericdesktop_usages HID GenericDesktop Usages
 *      @{
 *
 * \section Usages
 * - HIDGenericDesktop_POINTER
 * - HIDGenericDesktop_MOUSE
 * - HIDGenericDesktop_JOYSTICK
 * - HIDGenericDesktop_GAMEPAD
 * - HIDGenericDesktop_KEYBOARD
 * - HIDGenericDesktop_KEYPAD
 * - HIDGenericDesktop_MULTIAXIS
 * - HIDGenericDesktop_X
 * - HIDGenericDesktop_Y
 */

/** Pointer usage ID. */
#define HIDGenericDesktop_POINTER           0x01
/** Mouse usage ID. */
#define HIDGenericDesktop_MOUSE             0x02
/** Joystick usage ID. */
#define HIDGenericDesktop_JOYSTICK          0x04
/** Gamepad usage ID. */
#define HIDGenericDesktop_GAMEPAD           0x05
/** Keyboard usage ID. */
#define HIDGenericDesktop_KEYBOARD          0x06
/** Keypad usage ID. */
#define HIDGenericDesktop_KEYPAD            0x07
/** Multi-axis controller usage ID. */
#define HIDGenericDesktop_MULTIAXIS         0x08

/** Axis Usage X direction ID. */
#define HIDGenericDesktop_X                 0x30
/** Axis Usage Y direction ID. */
#define HIDGenericDesktop_Y                 0x31
/**      @}*/


/** \addtogroup usb_hid_keypad_page_id HID Keypad Page ID
 *      @{
 * This page lists HID Keypad page ID.
 *
 * \section ID
 * - HIDKeypad_PAGEID
 */

/** Identifier for the HID keypad usage page */
#define HIDKeypad_PAGEID                    0x07
/**      @}*/

/** \addtogroup usb_hid_alphabetic_keys HID Alphabetic Keys
 *      @{
 *
 * \section Keys
 * - HIDKeypad_A
 * - HIDKeypad_B
 * - HIDKeypad_C
 * - HIDKeypad_D
 * - HIDKeypad_E
 * - HIDKeypad_F
 * - HIDKeypad_G
 * - HIDKeypad_H
 * - HIDKeypad_I
 * - HIDKeypad_J
 * - HIDKeypad_K
 * - HIDKeypad_L
 * - HIDKeypad_M
 * - HIDKeypad_N
 * - HIDKeypad_O
 * - HIDKeypad_P
 * - HIDKeypad_Q
 * - HIDKeypad_R
 * - HIDKeypad_S
 * - HIDKeypad_T
 * - HIDKeypad_U
 * - HIDKeypad_V
 * - HIDKeypad_W
 * - HIDKeypad_X
 * - HIDKeypad_Y
 * - HIDKeypad_Z
 */

/** Key code for 'a' and 'A'. */
#define HIDKeypad_A                     4
/** Key code for 'b' and 'B'. */
#define HIDKeypad_B                     5
/** Key code for 'c' and 'C'. */
#define HIDKeypad_C                     6
/** Key code for 'd' and 'D'. */
#define HIDKeypad_D                     7
/** Key code for 'e' and 'E'. */
#define HIDKeypad_E                     8
/** Key code for 'f' and 'F'. */
#define HIDKeypad_F                     9
/** Key code for 'g' and 'G'. */
#define HIDKeypad_G                     10
/** Key code for 'h' and 'H'. */
#define HIDKeypad_H                     11
/** Key code for 'i' and 'I'. */
#define HIDKeypad_I                     12
/** Key code for 'j' and 'J'. */
#define HIDKeypad_J                     13
/** Key code for 'k' and 'K'. */
#define HIDKeypad_K                     14
/** Key code for 'l' and 'L'. */
#define HIDKeypad_L                     15
/** Key code for 'm' and 'M'. */
#define HIDKeypad_M                     16
/** Key code for 'n' and 'N'. */
#define HIDKeypad_N                     17
/** Key code for 'o' and 'O'. */
#define HIDKeypad_O                     18
/** Key code for 'p' and 'P'. */
#define HIDKeypad_P                     19
/** Key code for 'q' and 'Q'. */
#define HIDKeypad_Q                     20
/** Key code for 'r' and 'R'. */
#define HIDKeypad_R                     21
/** Key code for 's' and 'S'. */
#define HIDKeypad_S                     22
/** Key code for 't' and 'T'. */
#define HIDKeypad_T                     23
/** Key code for 'u' and 'U'. */
#define HIDKeypad_U                     24
/** Key code for 'v' and 'V'. */
#define HIDKeypad_V                     25
/** Key code for 'w' and 'W'. */
#define HIDKeypad_W                     26
/** Key code for 'x' and 'X'. */
#define HIDKeypad_X                     27
/** Key code for 'y' and 'Y'. */
#define HIDKeypad_Y                     28
/** Key code for 'z' and 'Z'. */
#define HIDKeypad_Z                     29
/**      @}*/

/** \addtogroup usb_hid_numeric_keys HID Numeric Keys
 *      @{
 *
 * \section Keys
 * - HIDKeypad_1
 * - HIDKeypad_2
 * - HIDKeypad_3
 * - HIDKeypad_4
 * - HIDKeypad_5
 * - HIDKeypad_6
 * - HIDKeypad_7
 * - HIDKeypad_8
 * - HIDKeypad_9
 * - HIDKeypad_0
 */

/** Key code for '1' and '!'. */
#define HIDKeypad_1                     30
/** Key code for '2' and '@'. */
#define HIDKeypad_2                     31
/** Key code for '3' and '#'. */
#define HIDKeypad_3                     32
/** Key code for '4' and '$'. */
#define HIDKeypad_4                     33
/** Key code for '5' and '%'. */
#define HIDKeypad_5                     34
/** Key code for '6' and '^'. */
#define HIDKeypad_6                     35
/** Key code for '7' and '&'. */
#define HIDKeypad_7                     36
/** Key code for '8' and '*'. */
#define HIDKeypad_8                     37
/** Key code for '9' and '('. */
#define HIDKeypad_9                     38
/** Key code for '0' and ')'. */
#define HIDKeypad_0                     39
/**      @}*/

/** \addtogroup usb_hid_special_keys HID Special Keys
 *      @{
 *
 * \section Keys
 * - HIDKeypad_ENTER
 * - HIDKeypad_ESCAPE
 * - HIDKeypad_BACKSPACE
 * - HIDKeypad_TAB
 * - HIDKeypad_SPACEBAR
 * - HIDKeypad_PRINTSCREEN
 * - HIDKeypad_SCROLLLOCK
 * - HIDKeypad_NUMLOCK
 */

/** Enter key code. */
#define HIDKeypad_ENTER                 40
/** Escape key code. */
#define HIDKeypad_ESCAPE                41
/** Backspace key code. */
#define HIDKeypad_BACKSPACE             42
/** Tab key code. */
#define HIDKeypad_TAB                   43
/** Spacebar key code. */
#define HIDKeypad_SPACEBAR              44
/** Printscreen key code. */
#define HIDKeypad_PRINTSCREEN           70
/** Scroll lock key code. */
#define HIDKeypad_SCROLLLOCK            71
/** Num lock key code. */
#define HIDKeypad_NUMLOCK               83
/**      @}*/

/** \addtogroup usb_hid_modified_keys HID Modified Keys
 *      @{
 *
 * \section Keys
 * - HIDKeypad_LEFTCONTROL
 * - HIDKeypad_LEFTSHIFT
 * - HIDKeypad_LEFTALT
 * - HIDKeypad_LEFTGUI
 * - HIDKeypad_RIGHTCONTROL
 * - HIDKeypad_RIGHTSHIFT
 * - HIDKeypad_RIGHTALT
 * - HIDKeypad_RIGHTGUI
 */

/** Key code for the left 'Control' key. */
#define HIDKeypad_LEFTCONTROL           224
/** Key code for the left 'Shift' key. */
#define HIDKeypad_LEFTSHIFT             225
/** Key code for the left 'Alt' key. */
#define HIDKeypad_LEFTALT               226
/** Key code for the left 'GUI' (e.g. Windows) key. */
#define HIDKeypad_LEFTGUI               227
/** Key code for the right 'Control' key. */
#define HIDKeypad_RIGHTCONTROL          228
/** Key code for the right 'Shift' key. */
#define HIDKeypad_RIGHTSHIFT            229
/** Key code for the right 'Alt' key. */
#define HIDKeypad_RIGHTALT              230
/** Key code for the right 'GUI' key. */
#define HIDKeypad_RIGHTGUI              231
/**      @}*/

/** \addtogroup usb_hid_error_codes HID Error Codes
 *      @{
 *
 * \section Codes
 * - HIDKeypad_ERRORROLLOVER
 * - HIDKeypad_POSTFAIL
 * - HIDKeypad_ERRORUNDEFINED
 */

/** Indicates that too many keys have been pressed at the same time. */
#define HIDKeypad_ERRORROLLOVER         1
/** postfail */
#define HIDKeypad_POSTFAIL              2
/** Indicates an undefined error. */
#define HIDKeypad_ERRORUNDEFINED        3
/**      @}*/


/** \addtogroup usb_hid_leds_page_id HID LEDs Page ID
 *      @{
 * This page lists the page ID of the HID LEDs usage page.
 *
 * \section ID
 * - HIDLeds_PAGEID
 */

/** ID of the HID LEDs usage page. */
#define HIDLeds_PAGEID                  0x08
/**      @}*/

/** \addtogroup usb_hid_leds_usage HID LEDs Usages
 *      @{
 * This page lists the Usages of the HID LEDs.
 *
 * \section Usages
 * - HIDLeds_NUMLOCK
 * - HIDLeds_CAPSLOCK
 * - HIDLeds_SCROLLLOCK
 */

/** Num lock LED usage. */
#define HIDLeds_NUMLOCK                 0x01
/** Caps lock LED usage. */
#define HIDLeds_CAPSLOCK                0x02
/** Scroll lock LED usage. */
#define HIDLeds_SCROLLLOCK              0x03
/**      @}*/


/** \addtogroup usb_hid_buttons_page_id HID BUTTONs Page ID
 *      @{
 */
/** Identifier for the HID button usage page*/
#define HIDButton_PAGEID                0x09
/**     @}*/


/*------------------------------------------------------------------------------
 *         Exported functions
 *------------------------------------------------------------------------------*/

extern uint8_t HIDKeypad_IsModifierKey(uint8_t key);

/**@}*/
#endif  /* #define _HIDUSAGES_H_ */

