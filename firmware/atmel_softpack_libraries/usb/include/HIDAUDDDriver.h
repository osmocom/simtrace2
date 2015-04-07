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

/**
 * \file
 *
 * \section Purpose
 *
 *   Definitions and methods for USB composite device implement.
 * 
 */

#ifndef HIDAUDDDRIVER_H
#define HIDAUDDDRIVER_H
/** \addtogroup usbd_hid_aud
 *@{
 */

/*---------------------------------------------------------------------------
 *         Headers
 *---------------------------------------------------------------------------*/

#include <USBRequests.h>
#include <HIDDescriptors.h>
#include <AUDDescriptors.h>
#include <USBD.h>
#include <USBDDriver.h>

/*---------------------------------------------------------------------------
 *         Definitions
 *---------------------------------------------------------------------------*/

/** \addtogroup usbd_hid_aud_desc USB HID(Keyboard) + AUD(Speaker) Definitions
 *      @{
 */
/** Number of interfaces of the device 1+2 */
#define HIDAUDDDriverDescriptors_NUMINTERFACE       3
/** Number of the CDC interface. */
#define HIDAUDDDriverDescriptors_HID_INTERFACE      0
/** Number of the Audio interface. */
#define HIDAUDDDriverDescriptors_AUD_INTERFACE      1
/**     @}*/

/*---------------------------------------------------------------------------
 *         Types
 *---------------------------------------------------------------------------*/

#ifdef __ICCARM__          /* IAR */
#pragma pack(1)            /* IAR */
#define __attribute__(...) /* IAR */
#endif                     /* IAR */

/** Structure of audio header descriptor*/
typedef struct _AUDHeaderDescriptor1{

    /** Header descriptor.*/
    AUDHeaderDescriptor header;
    /** Id of the first grouped interface.*/
    unsigned char bInterface0;

} __attribute__ ((packed)) AUDHeaderDescriptor1;

/**
 * Feature unit descriptor with 3 channel controls (master, right, left).
 */
typedef struct _AUDFeatureUnitDescriptor3{

    /** Feature unit descriptor.*/
    AUDFeatureUnitDescriptor feature;
    /** Available controls for each channel.*/
    unsigned char bmaControls[3];
    /** Index of a string descriptor for the feature unit.*/
    unsigned char iFeature;

} __attribute__ ((packed)) AUDFeatureUnitDescriptor3;

/**
 * List of descriptors for detailling the audio control interface of a
 * device using a USB audio speaker driver.
 */
typedef struct _AUDDSpeakerDriverAudioControlDescriptors{

    /** Header descriptor (with one slave interface).*/
    AUDHeaderDescriptor1 header;
    /** Input terminal descriptor.*/
    AUDInputTerminalDescriptor input;
    /** Output terminal descriptor.*/
    AUDOutputTerminalDescriptor output;
    /** Feature unit descriptor.*/
    AUDFeatureUnitDescriptor3 feature;

} __attribute__ ((packed)) AUDDSpeakerDriverAudioControlDescriptors; // GCC

/**
 * Format type I descriptor with one discrete sampling frequency.
 */
typedef struct _AUDFormatTypeOneDescriptor1{

    /** Format type I descriptor.*/
    AUDFormatTypeOneDescriptor formatType;
    /** Sampling frequency in Hz.*/
    unsigned char tSamFreq[3];

} __attribute__ ((packed)) AUDFormatTypeOneDescriptor1; // GCC

/**
 * \typedef CdcAudDriverConfigurationDescriptors
 * \brief Configuration descriptor list for a device implementing a
 *        composite HID (Keyboard) + Audio (Speaker) driver.
 */
typedef struct _HidAuddDriverConfigurationDescriptors {

    /** Standard configuration descriptor. */
    USBConfigurationDescriptor configuration;

    /* --- HID */
    /** HID interface. */
    USBInterfaceDescriptor hidInterface;
    /** HID descriptor */
    HIDDescriptor1 hid;
    /** HID interrupt IN */
    USBEndpointDescriptor hidInterruptIn;
    /** HID interrupt OUT */
    USBEndpointDescriptor hidInterruptOut;

    /* --- AUDIO (AC) */
    /** Audio IAD */
    USBInterfaceAssociationDescriptor audIAD;
    /** Audio control interface.*/
    USBInterfaceDescriptor audInterface;
    /** Descriptors for the audio control interface.*/
    AUDDSpeakerDriverAudioControlDescriptors audControl;
    /* -- AUDIO out (AS) */
    /** Streaming out interface descriptor (with no endpoint, required).*/
    USBInterfaceDescriptor audStreamingOutNoIsochronous;
    /** Streaming out interface descriptor.*/
    USBInterfaceDescriptor audStreamingOut;
    /** Audio class descriptor for the streaming out interface.*/
    AUDStreamingInterfaceDescriptor audStreamingOutClass;
    /** Stream format descriptor.*/
    AUDFormatTypeOneDescriptor1 audStreamingOutFormatType;
    /** Streaming out endpoint descriptor.*/
    AUDEndpointDescriptor audStreamingOutEndpoint;
    /** Audio class descriptor for the streaming out endpoint.*/
    AUDDataEndpointDescriptor audStreamingOutDataEndpoint;

} __attribute__ ((packed)) HidAuddDriverConfigurationDescriptors;

#ifdef __ICCARM__          /* IAR */
#pragma pack()             /* IAR */
#endif                     /* IAR */

/*---------------------------------------------------------------------------
 *         Exported functions
 *---------------------------------------------------------------------------*/

extern void HIDAUDDDriver_Initialize(const USBDDriverDescriptors * pDescriptors);
extern void HIDAUDDDriver_ConfigurationChangedHandler(uint8_t cfgnum);
extern void HIDAUDDDriver_InterfaceSettingChangedHandler(
    uint8_t interface, uint8_t setting);
extern void HIDAUDDDriver_RequestHandler(const USBGenericRequest *request);

/**@}*/
#endif //#ifndef CDCHIDDDRIVER_H

