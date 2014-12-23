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
 *  USB Audio Speaker & Microphone Function.
 */

/** \addtogroup usbd_audio_speakerphone
 *@{
 *  Implement speakerphone function that combine 1 AC interface, 1 AS interface
 *  for speaker and 1 AS interface for microphone.
 */

#ifndef _AUDD_SPEAKERPHONE_H_
#define _AUDD_SPEAKERPHONE_H_

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <stdint.h>

#include <AUDDStream.h>

#include <USBD.h>
#include <USBDDriver.h>

/*------------------------------------------------------------------------------
 *         Defines
 *------------------------------------------------------------------------------*/

/** \addtogroup usbd_audio_ids Audio Device IDs required by driver
 *      @{
 *  The driver uses following fixed entity IDs, use them to define your
 *  descriptor.
 *  \ref AUDD_ID_SpeakerIT
 *  \ref AUDD_ID_SpeakerOT
 *  \ref AUDD_ID_SpeakerFU
 *  \ref AUDD_ID_MicrophoneIT
 *  \ref AUDD_ID_MicrophoneOT
 *  \ref AUDD_ID_MicrophoneFU
 */
/** Speaker input terminal ID */
#define AUDD_ID_SpeakerIT           0x01
/** Speaker output terminal ID */
#define AUDD_ID_SpeakerOT           0x02
/** Speaker feature unit ID */
#define AUDD_ID_SpeakerFU           0x03
/** Microphone input terminal ID */
#define AUDD_ID_MicrophoneIT        0x04
/** Microphone output terminal ID */
#define AUDD_ID_MicrophoneOT        0x05
/** Microphone feature unit ID */
#define AUDD_ID_MicrophoneFU        0x06
/**     @}*/

/** \addtogroup usbd_audio_chs Audio Device Channels
 *      @{
 */
/** Master channel ID */
#define AUDD_CH_Master              0x0
/** Left channel ID */
#define AUDD_CH_L                   0x1
/** Right channel ID */
#define AUDD_CH_R                   0x2
/**     @}*/

/*------------------------------------------------------------------------------
 *         Types
 *------------------------------------------------------------------------------*/

/**
 * Struct of USB Audio Speakerphone Function, with
 * - 1 control interface for features,
 * - 1 input stream for microphone (Audio IN),
 * - 1 output stream for speaker (Audio OUT).
 */
typedef struct _AUDDSpeakerPhone {
    /** Pointer to USBD Driver Interface */
    USBDDriver *pUsbd;
    /** Pointer to AUDDStream Instance */
    AUDDStream *pSpeaker;
    /** Pointer to AUDDStream Instance */
    AUDDStream *pMicrophone;
} AUDDSpeakerPhone;

/*------------------------------------------------------------------------------
 *         Functions
 *------------------------------------------------------------------------------*/

extern void AUDDSpeakerPhone_InitializeStream(
    AUDDStream * pAuds,
    uint8_t numChannels,uint16_t wChannelVolumes [ ],
    AUDDStreamEventCallback fCallback,void * pArg);

extern void AUDDSpeakerPhone_Initialize(
    AUDDSpeakerPhone * pAudf,
    USBDDriver * pUsbd,
    AUDDStream * pSpeaker,AUDDStream * pMicrophone);

extern USBGenericDescriptor* AUDDSpeakerPhone_ParseInterfaces(
    AUDDSpeakerPhone * pAudf,
    USBGenericDescriptor * pDescriptors,
    uint32_t dwLength);

extern uint32_t AUDDSpeakerPhone_RequestHandler(
    AUDDSpeakerPhone * pAudf,
    const USBGenericRequest * pRequest);

extern uint32_t AUDDSpeakerPhone_CloseStream(
    AUDDSpeakerPhone * pAudf,
    uint32_t bInterface);

extern uint32_t AUDDSpeakerPhone_SetupWrite(
    AUDDSpeakerPhone * pAudf,
    void * pListInit, void * pDmaInit, uint16_t listSize, uint16_t delaySize,
    TransferCallback callback,void * argument);

extern uint32_t AUDDSpeakerPhone_Write(
    AUDDSpeakerPhone * pAudf,
    void * pBuffer,uint16_t wLength);

extern uint32_t AUDDSpeakerPhone_Read(
    AUDDSpeakerPhone * pAudf,
    void * pData, uint32_t dwSize,
    TransferCallback fCallback,void * pArg);

/**@}*/
#endif /* _AUDD_SPEAKERPHONE_H_ */

