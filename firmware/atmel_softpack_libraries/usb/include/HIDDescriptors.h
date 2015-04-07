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
 * Definitions used for declaring the descriptors of a HID device.
 *
 */

#ifndef _HIDDESCRIPTORS_H_
#define _HIDDESCRIPTORS_H_
/** \addtogroup usb_hid
 *@{
 */

/*----------------------------------------------------------------------------
 *         Includes
 *----------------------------------------------------------------------------*/

#include <stdint.h>

#include "HIDReports.h"
#include "HIDUsages.h"

/*----------------------------------------------------------------------------
 *         Definitions
 *----------------------------------------------------------------------------*/

/** \addtogroup usb_hid_device_descriptor_codes HID Device Descriptor Codes
 *      @{
 * This page lists HID device class, subclass and protocol codes.
 *
 * \section Codes
 * - HIDDeviceDescriptor_CLASS
 * - HIDDeviceDescriptor_SUBCLASS
 * - HIDDeviceDescriptor_PROTOCOL
 */

/** Class code for a HID device. */
#define HIDDeviceDescriptor_CLASS               0
/** Subclass code for a HID device. */
#define HIDDeviceDescriptor_SUBCLASS            0
/** Protocol code for a HID device. */
#define HIDDeviceDescriptor_PROTOCOL            0
/**      @}*/

/** \addtogroup usb_hid_interface_descriptor_codes HID Interface Descriptor Codes
 *      @{
 * This page lists HID Interface class, subclass and protocol codes.
 *
 * \section Codes
 * - HIDInterfaceDescriptor_CLASS
 * - HIDInterfaceDescriptor_SUBCLASS_NONE
 * - HIDInterfaceDescriptor_SUBCLASS_BOOT
 * - HIDInterfaceDescriptor_PROTOCOL_NONE
 * - HIDInterfaceDescriptor_PROTOCOL_KEYBOARD
 * - HIDInterfaceDescriptor_PROTOCOL_MOUSE
 */

/** HID interface class code. */
#define HIDInterfaceDescriptor_CLASS                0x03
/** Indicates the interface does not implement a particular subclass. */
#define HIDInterfaceDescriptor_SUBCLASS_NONE        0x00
/** Indicates the interface is compliant with the boot specification. */
#define HIDInterfaceDescriptor_SUBCLASS_BOOT        0x01
/** Indicates the interface does not implement a particular protocol. */
#define HIDInterfaceDescriptor_PROTOCOL_NONE        0x00
/** Indicates the interface supports the boot specification as a keyboard. */
#define HIDInterfaceDescriptor_PROTOCOL_KEYBOARD    0x01
/** Indicates the interface supports the boot specification as a mouse. */
#define HIDInterfaceDescriptor_PROTOCOL_MOUSE       0x02
/**      @}*/

/** \addtogroup usb_hid_ver HID Release Numbers
 *      @{
 * - \ref HIDDescriptor_HID1_11
 */

/** Identifies version 1.11 of the HID specification. */
#define HIDDescriptor_HID1_11           0x0111
/**     @}*/

/** \addtogroup usb_descriptors_types HID Descriptors Types
 *      @{
 *
 * \section Types
 * - HIDGenericDescriptor_HID
 * - HIDGenericDescriptor_REPORT
 * - HIDGenericDescriptor_PHYSICAL
 */

/** HID descriptor type. */
#define HIDGenericDescriptor_HID            0x21
/** Report descriptor type. */
#define HIDGenericDescriptor_REPORT         0x22
/** Physical descriptor type. */
#define HIDGenericDescriptor_PHYSICAL       0x23
/**      @}*/

/*----------------------------------------------------------------------------
 *         Types
 *----------------------------------------------------------------------------*/

#ifdef __ICCARM__          /* IAR */
#pragma pack(1)            /* IAR */
#define __attribute__(...) /* IAR */
#endif                     /* IAR */

/**
 * \typedef HIDDescriptor
 * \brief Identifies the length of type of subordinate descriptors of a HID
 *        device. This particular type has no subordinate descriptor.
 */
typedef struct _HIDDescriptor {

    /** Size of descriptor in bytes. */
    uint8_t  bLength;
    /** Descriptor type (\ref HIDGenericDescriptor_HID). */
    uint8_t  bDescriptorType;
    /** HID class specification release number in BCD format. */
    uint16_t bcdHID;
    /** Country code of the device if it is localized. */
    uint8_t  bCountryCode;
    /** Number of subordinate descriptors. */
    uint8_t  bNumDescriptors;

} __attribute__ ((packed)) HIDDescriptor; /* GCC */

/**
 * \typedef HIDDescriptor
 * \brief Identifies the length of type of subordinate descriptors of a HID
 *        device. This particular type only supports one subordinate descriptor.
 */
typedef struct _HIDDescriptor1 {

    /** Size of descriptor in bytes. */
    uint8_t  bLength;
    /** Descriptor type (\ref HIDGenericDescriptor_HID). */
    uint8_t  bDescriptorType;
    /** HID class specification release number in BCD format. */
    uint16_t bcdHID;
    /** Country code of the device if it is localized. */
    uint8_t  bCountryCode;
    /** Number of subordinate descriptors. */
    uint8_t  bNumDescriptors;
    /** Type of the first subordinate descriptor. */
    uint8_t  bDescriptorType0;
    /** Size in bytes of the first subordinate descriptor. */
    /* uint8_t  bDescriptorLength0[2]; */
    uint16_t wDescriptorLength0;

} __attribute__ ((packed)) HIDDescriptor1; /* GCC */

/**
 * HID Physical Descriptor set 0: specifies the number of additional
 * descriptor sets.
 */
typedef struct _HIDPhysicalDescriptor0 {
    /** Numeric expression specifying the number of Physical Descriptor sets
        Physical Descriptor 0 itself not included */
    uint8_t   bNumber;
    /** Numeric expression identifying the length of each Physical descriptor */
    uint8_t   bLength[2];
} __attribute__ ((packed)) HIDPhysicalDescriptor0; /* GCC */

/**
 * HID Physical information
 */
typedef union _HIDPhysicalInfo {
    /** Bits specifying physical information: 7..5 Bias, 4..0 Preference */
    uint8_t bData;
    struct {
        uint8_t Preference:5,   /**< 0=Most preferred */
                Bias:3;         /**< indicates which hand the descriptor
                                     set is characterizing */
    } sPhysicalInfo;
} HIDPhysicalInfo;

/**
 * HID Physical Descriptor
 */
typedef struct _HIDPhysicalDescriptor {
    /** Designator: indicates which part of the body affects the item */
    uint8_t bDesignator;
    /** Bits specifying flags:
        7..5 Qualifier;
        4..0 Effort */
    uint8_t bFlags;
} __attribute__ ((packed)) HIDPhysicalDescriptor; /* GCC */

#ifdef __ICCARM__          /* IAR */
#pragma pack()             /* IAR */
#endif                     /* IAR */

/*----------------------------------------------------------------------------
 *         Functions
 *----------------------------------------------------------------------------*/

/**@}*/
#endif /* #ifndef _HIDDESCRIPTORS_H_ */

