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
 *    Definition of a USB Audio Speakerphone Driver with two playback channels
 *    and one record channel.
 *
 * \section Usage
 *
 *    -# Enable and setup USB related pins (see pio & board.h).
 *    -# Inplement a descriptor list using definitions
 *       - \ref usbd_audio_speakerphone_channel_num
 *       - \ref usbd_audio_speaker_interface_ids
 *       - \ref usbd_audio_speaker_entity_ids
 *    -# Configure the USB Audio Speakerphone driver using
 *       AUDDSpeakerPhoneDriver_Initialize
 *    -# To get %audio stream frames from host, use
 *       AUDDSpeakerPhoneDriver_Read
 *    -# To send %audio sampling stream to host, use
 *       AUDDSpeakerPhoneDriver_Write
 *
 */

#ifndef AUDDSpeakerPhoneDriver_H
#define AUDDSpeakerPhoneDriver_H

/** \addtogroup usbd_audio_speakerphone
 *@{
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <USBRequests.h>
#include <USBDescriptors.h>
#include <AUDDescriptors.h>

#include <USBD.h>
#include <USBD_HAL.h>
#include <USBDDriver.h>

/*------------------------------------------------------------------------------
 *         Definitions
 *------------------------------------------------------------------------------*/

/** \addtogroup usbd_audio_speakerphone_channel_num Audio SpeakerPhone Channel Numbers
 *      @{
 * This page lists codes for USB Audio Speakerphone channel numbers.
 *
 * \section Playback channel numbers
 * - AUDDSpeakerPhoneDriver_MASTERCHANNEL
 * - AUDDSpeakerPhoneDriver_LEFTCHANNEL
 * - AUDDSpeakerPhoneDriver_RIGHTCHANNEL
 *
 * \section Record channel number
 * - AUDDSpeakerPhoneDriver_RECCHANNEL
 */

/** Master channel of playback. */
#define AUDDSpeakerPhoneDriver_MASTERCHANNEL                0
/** Front left channel of playback. */
#define AUDDSpeakerPhoneDriver_LEFTCHANNEL                  1
/** Front right channel of playback. */
#define AUDDSpeakerPhoneDriver_RIGHTCHANNEL                 2
/** Channel of record. */
#define AUDDSpeakerPhoneDriver_RECCHANNEL                   0
/**      @}*/

/** \addtogroup usbd_audio_speaker_interface_ids Audio SpeakerPhone Interface IDs
 *      @{
 * This page lists the interface numbers for USB Audio Speakerphone device.
 *
 * \section Interfaces
 * - AUDDSpeakerPhoneDriverDescriptors_CONTROL
 * - AUDDSpeakerPhoneDriverDescriptors_STREAMING
 * - AUDDSpeakerPhoneDriverDescriptors_STREAMINGIN
 */

/** Audio control interface ID */
#define AUDDSpeakerPhoneDriverDescriptors_CONTROL            0
/** Audio streaming interface ID (OUT, for playback) */
#define AUDDSpeakerPhoneDriverDescriptors_STREAMING          1
/** Audio streaming interface ID (IN, for record) */
#define AUDDSpeakerPhoneDriverDescriptors_STREAMINGIN        2
/**      @}*/

/** \addtogroup usbd_audio_speaker_entity_ids Audio SpeakerPhone Entity IDs
 *      @{
 * This page lists the entity IDs for USB Audio Speaker device.
 *
 * \section Entities
 * - AUDDSpeakerPhoneDriverDescriptors_INPUTTERMINAL
 * - AUDDSpeakerPhoneDriverDescriptors_OUTPUTTERMINAL
 * - AUDDSpeakerPhoneDriverDescriptors_FEATUREUNIT
 * - AUDDSpeakerPhoneDriverDescriptors_INPUTTERMINAL_REC
 * - AUDDSpeakerPhoneDriverDescriptors_OUTPUTTERMINAL_REC
 * - AUDDSpeakerPhoneDriverDescriptors_FEATUREUNIT_REC
 */

/** Playback input terminal ID */
#define AUDDSpeakerPhoneDriverDescriptors_INPUTTERMINAL      0
/** Playback output terminal ID */
#define AUDDSpeakerPhoneDriverDescriptors_OUTPUTTERMINAL     1
/** Playback feature unit ID */
#define AUDDSpeakerPhoneDriverDescriptors_FEATUREUNIT        2
/** Record input terminal ID */
#define AUDDSpeakerPhoneDriverDescriptors_INPUTTERMINAL_REC  3
/** Record output terminal ID */
#define AUDDSpeakerPhoneDriverDescriptors_OUTPUTTERMINAL_REC 4
/** Record feature unit ID */
#define AUDDSpeakerPhoneDriverDescriptors_FEATUREUNIT_REC    5
/**      @}*/

/*------------------------------------------------------------------------------
 *         Types
 *------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *  Audio control header descriptor with one slave interface.
 *------------------------------------------------------------------------------*/
#ifdef __ICCARM__          /* IAR */
#pragma pack(1)            /* IAR */
#define __attribute__(...) /* IAR */
#endif                     /* IAR */

/**
 * \typedef AUDHeaderDescriptor2
 * \brief Header descriptor with 1 interface.
 */
typedef struct _AUDHeaderDescriptor2 {

    /** Header descriptor. */
    AUDHeaderDescriptor header;
    /** Id of the first grouped interface - Speaker. */
    unsigned char bInterface0;
    /** Id of the second grouped interface - Speakerphone. */
    unsigned char bInterface1;

} __attribute__ ((packed)) AUDHeaderDescriptor2; /* GCC */

/**
 * \typedef AUDFeatureUnitDescriptor3
 * \brief Feature unit descriptor with 3 channel controls (master, right, left).
 */
typedef struct _AUDFeatureUnitDescriptor3 {

    /** Feature unit descriptor. */
    AUDFeatureUnitDescriptor feature;
    /** Available controls for each channel. */
    unsigned char bmaControls[3];
    /** Index of a string descriptor for the feature unit. */
    unsigned char iFeature;

} __attribute__ ((packed)) AUDFeatureUnitDescriptor3; /* GCC */

/**
 * \typedef AUDDSpeakerPhoneDriverAudioControlDescriptors
 * \brief List of descriptors for detailling the audio control interface of a
 *        device using a USB Audio Speakerphoneer driver.
 */
typedef struct _AUDDSpeakerPhoneDriverAudioControlDescriptors {

    /** Header descriptor (with one slave interface). */
    AUDHeaderDescriptor2 header;
    /** Input terminal descriptor. */
    AUDInputTerminalDescriptor inputSpeakerPhone;
    /** Output terminal descriptor. */
    AUDOutputTerminalDescriptor outputSpeakerPhone;
    /** Feature unit descriptor - SpeakerPhone. */
    AUDFeatureUnitDescriptor3 featureSpeakerPhone;
    /** Input terminal descriptor. */
    AUDInputTerminalDescriptor inputRec;
    /** Output terminal descriptor. */
    AUDOutputTerminalDescriptor outputRec;
    /** Feature unit descriptor - SpeakerPhonephone. */
    AUDFeatureUnitDescriptor3 featureRec;

} __attribute__ ((packed)) AUDDSpeakerPhoneDriverAudioControlDescriptors; /* GCC */

/**
 * \typedef AUDFormatTypeOneDescriptor1
 * \brief Format type I descriptor with one discrete sampling frequency.
 */
typedef struct _AUDFormatTypeOneDescriptor1 {

    /** Format type I descriptor. */
    AUDFormatTypeOneDescriptor formatType;
    /** Sampling frequency in Hz. */
    unsigned char tSamFreq[3];

} __attribute__ ((packed)) AUDFormatTypeOneDescriptor1; /* GCC */

/**
 * \typedef AUDDSpeakerPhoneDriverConfigurationDescriptors
 * \brief Holds a list of descriptors returned as part of the configuration of
 *        a USB Audio Speakerphone device.
 */
typedef struct _AUDDSpeakerPhoneDriverConfigurationDescriptors {

    /** Standard configuration. */
    USBConfigurationDescriptor configuration;
    /** Audio control interface. */
    USBInterfaceDescriptor control;
    /** Descriptors for the audio control interface. */
    AUDDSpeakerPhoneDriverAudioControlDescriptors controlDescriptors;
    /*- AUDIO OUT */
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
    /*- AUDIO IN */
    /** Streaming in interface descriptor (with no endpoint, required). */
    USBInterfaceDescriptor streamingInNoIsochronous;
    /** Streaming in interface descriptor. */
    USBInterfaceDescriptor streamingIn;
    /** Audio class descriptor for the streaming in interface. */
    AUDStreamingInterfaceDescriptor streamingInClass;
    /** Stream format descriptor. */
    AUDFormatTypeOneDescriptor1 streamingInFormatType;
    /** Streaming in endpoint descriptor. */
    AUDEndpointDescriptor streamingInEndpoint;
    /** Audio class descriptor for the streaming in endpoint. */
    AUDDataEndpointDescriptor streamingInDataEndpoint;

} __attribute__ ((packed)) AUDDSpeakerPhoneDriverConfigurationDescriptors; /* GCC */

#ifdef __ICCARM__          /* IAR */
#pragma pack()             /* IAR */
#endif                     /* IAR */

/*------------------------------------------------------------------------------
 *         Exported functions
 *------------------------------------------------------------------------------*/

extern void AUDDSpeakerPhoneDriver_Initialize(
    const USBDDriverDescriptors *pDescriptors);

extern void AUDDSpeakerPhoneDriver_ConfigurationChangeHandler(uint8_t cfgnum);

extern void AUDDSpeakerPhoneDriver_InterfaceSettingChangedHandler(
    uint8_t interface,
    uint8_t setting);

extern void AUDDSpeakerPhoneDriver_RequestHandler(
    const USBGenericRequest *request);

extern uint8_t AUDDSpeakerPhoneDriver_Read(void *buffer,
                                           uint32_t length,
                                           TransferCallback callback,
                                           void *argument);

extern uint8_t AUDDSpeakerPhoneDriver_SetupWrite(void * pListInit,
                                                 void * pDmaInit,
                                                 uint16_t listSize,
                                                 uint16_t delaySize,
                                                 TransferCallback callback,
                                                 void * argument);

extern uint8_t AUDDSpeakerPhoneDriver_Write(void * buffer,uint16_t length);


extern void AUDDSpeakerPhoneDriver_MuteChanged(uint8_t mic,
                                                    uint8_t channel,
                                                    uint8_t muted);

extern void AUDDSpeakerPhoneDriver_StreamSettingChanged(uint8_t mic,
                                                            uint8_t newSetting);

/**@}*/

#endif /*#ifndef AUDDSpeakerPhoneDriver_H */

