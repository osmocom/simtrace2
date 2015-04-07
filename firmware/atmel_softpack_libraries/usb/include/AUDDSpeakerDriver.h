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
 *   Definition of a USB Audio Speaker Driver with two playback channels.
 *
 * \section Usage
 *
 *   -# Enable and setup USB related pins (see pio & board.h).
 *   -# Configure the USB Audio Speaker driver using AUDDSpeakerDriver_Initialize
 *   -# To get %audio stream frames from host, use AUDDSpeakerDriver_Read
 */

#ifndef AUDDSPEAKERDRIVER_H
#define AUDDSPEAKERDRIVER_H

/** \addtogroup usbd_audio_speaker
 *@{
 */

/*----------------------------------------------------------------------------
 *         Headers
 *----------------------------------------------------------------------------*/

#include <USBLib_Types.h>

#include <USBRequests.h>
#include <USBDescriptors.h>
#include <AUDDescriptors.h>

#include <USBD.h>
#include <USBDDriver.h>

#include <AUDDSpeakerPhone.h>

/*----------------------------------------------------------------------------
 *         Definitions
 *----------------------------------------------------------------------------*/

/** \addtogroup usbd_audio_ch USB Device Audio Speaker Channel Numbers
 *      @{
 * This page lists codes for USB Audio Speaker channel numbers.
 *
 * \par Playback channel numbers
 * - \ref AUDDSpeakerDriver_MASTERCHANNEL
 * - \ref AUDDSpeakerDriver_LEFTCHANNEL
 * - \ref AUDDSpeakerDriver_RIGHTCHANNEL
 *
 * \par Record channel number
 * - \ref AUDDSpeakerDriver_RECCHANNEL
 */
/** Master channel of playback. */
#define AUDDSpeakerDriver_MASTERCHANNEL     0
/** Front left channel of playback. */
#define AUDDSpeakerDriver_LEFTCHANNEL       1
/** Front right channel of playback. */
#define AUDDSpeakerDriver_RIGHTCHANNEL      2

/** Channel of record. */
#define AUDDSpeakerDriver_RECCHANNEL        0

/** Max Number of channels in audio stream (excluding master). */
#define AUDDSpeakerDriver_NUMCHANNELS       2
/**     @}*/

/** \addtogroup usbd_audio_speaker_if USB Device Audio Speaker Interface IDs
 *      @{
 * This page lists the interface numbers for USB Audio Speaker device.
 * - \ref AUDDSpeakerDriverDescriptors_CONTROL
 * - \ref AUDDSpeakerDriverDescriptors_STREAMING
 * Total number of integrated interfaces
 * - \ref AUDDSpeakerDriver_NUMINTERFACES
 */
/** Audio control interface ID. */
#define AUDDSpeakerDriverDescriptors_CONTROL            0
/** Audio streaming interface ID (OUT, for playback). */
#define AUDDSpeakerDriverDescriptors_STREAMING          1

/** Audio speaker total number of interfaces */
#define AUDDSpeakerDriver_NUMINTERFACES                 2
/**     @}*/

/** \addtogroup usbd_audio_entity USB Device Audio Speaker Entity IDs
 *      @{
 * This page lists the entity IDs for USB Audio Speaker device.
 * - \ref AUDDSpeakerDriverDescriptors_INPUTTERMINAL
 * - \ref AUDDSpeakerDriverDescriptors_OUTPUTTERMINAL
 * - \ref AUDDSpeakerDriverDescriptors_FEATUREUNIT
 */
/** Playback input terminal ID. */
#define AUDDSpeakerDriverDescriptors_INPUTTERMINAL      AUDD_ID_SpeakerIT
/** Playback output terminal ID. */
#define AUDDSpeakerDriverDescriptors_OUTPUTTERMINAL     AUDD_ID_SpeakerOT
/** Playback feature unit ID. */
#define AUDDSpeakerDriverDescriptors_FEATUREUNIT        AUDD_ID_SpeakerFU
/**     @}*/

/*----------------------------------------------------------------------------
 *         Types
 *----------------------------------------------------------------------------*/

#ifdef __ICCARM__          /* IAR*/
#pragma pack(1)            /* IAR*/
#define __attribute__(...) /* IAR*/
#endif                     /* IAR*/

/**
 * \typedef AUDHeaderDescriptor1
 * \brief Header descriptor with 1 interface.
 */
typedef struct _AUDHeaderDescriptor1 {

    /** Header descriptor. */
    AUDHeaderDescriptor header;
    /** Id of the first grouped interface - Speaker. */
    uint8_t bInterface0;

} __attribute__ ((packed)) AUDHeaderDescriptor1; /* GCC*/

/**
 * \typedef AUDFeatureUnitDescriptor3
 * \brief Feature unit descriptor with 3 channel controls
 *        (master, right, left).
 */
typedef struct _AUDFeatureUnitDescriptor3 {

    /** Feature unit descriptor. */
    AUDFeatureUnitDescriptor feature;
    /** Available controls for each channel. */
    uint8_t bmaControls[3];
    /** Index of a string descriptor for the feature unit. */
    uint8_t iFeature;

} __attribute__ ((packed)) AUDFeatureUnitDescriptor3; /* GCC */

/**
 * \typedef AUDDSpeakerDriverAudioControlDescriptors
 * \brief List of descriptors for detailling the audio control interface of a
 *        device using a USB audio speaker driver.
 */
typedef struct _AUDDSpeakerDriverAudioControlDescriptors {

    /** Header descriptor (with one slave interface). */
    AUDHeaderDescriptor1 header;
    /** Input terminal descriptor. */
    AUDInputTerminalDescriptor input;
    /** Output terminal descriptor. */
    AUDOutputTerminalDescriptor output;
    /** Feature unit descriptor. */
    AUDFeatureUnitDescriptor3 feature;

} __attribute__ ((packed)) AUDDSpeakerDriverAudioControlDescriptors; /* GCC*/

/**
 * \typedef AUDFormatTypeOneDescriptor1
 * \brief Format type I descriptor with one discrete sampling frequency.
 */
typedef struct _AUDFormatTypeOneDescriptor1 {

    /** Format type I descriptor. */
    AUDFormatTypeOneDescriptor formatType;
    /** Sampling frequency in Hz. */
    uint8_t tSamFreq[3];

} __attribute__ ((packed)) AUDFormatTypeOneDescriptor1; /* GCC*/

/**
 * \typedef AUDDSpeakerDriverConfigurationDescriptors
 * \brief Holds a list of descriptors returned as part of the configuration of
 *        a USB audio speaker device.
 */
typedef struct _AUDDSpeakerDriverConfigurationDescriptors {

    /** Standard configuration. */
    USBConfigurationDescriptor configuration;
    /** Audio control interface. */
    USBInterfaceDescriptor control;
    /** Descriptors for the audio control interface. */
    AUDDSpeakerDriverAudioControlDescriptors controlDescriptors;
    /* - AUDIO OUT */
    /** Streaming out interface descriptor (with no endpoint, required). */
    USBInterfaceDescriptor streamingOutNoIsochronous;
    /** Streaming out interface descriptor. */
    USBInterfaceDescriptor streamingOut;
    /** Audio class descriptor for the streaming out interface. */
    AUDStreamingInterfaceDescriptor streamingOutClass;
    /** Stream format descriptor. */
    AUDFormatTypeOneDescriptor1 streamingOutFormatType;
    /** Streaming out endpoint descriptor. */
    AUDEndpointDescriptor streamingOutEndpoint;
    /** Audio class descriptor for the streaming out endpoint. */
    AUDDataEndpointDescriptor streamingOutDataEndpoint;

} __attribute__ ((packed)) AUDDSpeakerDriverConfigurationDescriptors; /* GCC */

#ifdef __ICCARM__          /* IAR */
#pragma pack()             /* IAR */
#endif                     /* IAR */

/*----------------------------------------------------------------------------
 *         Exported functions
 *----------------------------------------------------------------------------*/

extern void AUDDSpeakerDriver_Initialize(
    const USBDDriverDescriptors *pDescriptors);

extern void AUDDSpeakerDriver_ConfigurationChangeHandler(uint8_t cfgnum);

extern void AUDDSpeakerDriver_InterfaceSettingChangedHandler(
    uint8_t interface, uint8_t setting);

extern void AUDDSpeakerDriver_RequestHandler(const USBGenericRequest *request);

extern uint8_t AUDDSpeakerDriver_Read(void *buffer,
                                      uint32_t length,
                                      TransferCallback callback,
                                      void *argument);

extern void AUDDSpeakerDriver_MuteChanged(uint8_t channel,uint8_t muted);

extern void AUDDSpeakerDriver_StreamSettingChanged(uint8_t newSetting);

/**@}*/
#endif /*#ifndef AUDDSPEAKERDRIVER_H */


