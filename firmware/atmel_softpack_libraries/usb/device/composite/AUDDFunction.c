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
/** \file */

/** \addtogroup usbd_aud_fun
 *@{
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/
     
#include <USBLib_Trace.h>

#include <AUDDFunction.h>
#include <AUDDSpeakerPhone.h>

#include <AUDRequests.h>
     
#include <USBD.h>
#include <USBD_HAL.h>
#include <USBDDriver.h>

/*----------------------------------------------------------------------------
 *         Internal types
 *----------------------------------------------------------------------------*/

/**
 * \brief Audio speaker driver struct.
 */
typedef struct _AUDDFunction {
    /** Speaker & Phone function */
    AUDDSpeakerPhone drv;
    /** Stream instance for speaker */
    AUDDStream  speaker;
    /** Stream instance for microphone */
    AUDDStream  mic;
} AUDDFunction;

/*----------------------------------------------------------------------------
 *         Internal variables
 *----------------------------------------------------------------------------*/

/** Global USB audio function driver instance. */
static AUDDFunction auddFunction;

/*----------------------------------------------------------------------------
 *         Internal functions
 *----------------------------------------------------------------------------*/

/**
 * Callback triggerred after the mute or volume status of the channel has been
 * changed.
 * \param ec        Event code.
 * \param channel   Channel number.
 * \param pArg      Pointer to AUDDStream instance.
 */
static void AUDDFunction_EventCallback(uint32_t ec,
                                      uint8_t channel,
                                      AUDDStream *pArg)
{
    AUDDFunction *pAudf = &auddFunction;
    uint8_t mic = ((uint32_t)pArg == (uint32_t)(&pAudf->mic));
    if (ec == AUDD_EC_MuteChanged) {
        if (AUDDFunction_MuteChanged)
            AUDDFunction_MuteChanged(mic, channel, pArg->bmMute);
    }
    else if (ec == AUDD_EC_VolumeChanged) {
        /* Not supported now */
    }
}

/*---------------------------------------------------------------------------
 *         Exported functions
 *---------------------------------------------------------------------------*/

/**
 * Initializes an USB audio speaker device driver, as well as the underlying
 * USB controller.
 */
void AUDDFunction_Initialize(USBDDriver *pUsbd, uint8_t bInterface)
{
    AUDDFunction *pAudf = &auddFunction;
    AUDDSpeakerPhone *pDrv = &pAudf->drv;
    AUDDStream *pSpk = &pAudf->speaker;
    AUDDStream *pMic = &pAudf->mic;

    /* 0: Speaker */
    AUDDSpeakerPhone_InitializeStream(
        pSpk, AUDDFunction_MaxNumSpeakerChannels, 0,
        (AUDDStreamEventCallback)AUDDFunction_EventCallback,
        (void*)pSpk);
    /* 1: Mic */
    AUDDSpeakerPhone_InitializeStream(
        pMic, AUDDFunction_MaxNumMicrophoneChannels, 0,
        (AUDDStreamEventCallback)AUDDFunction_EventCallback,
        (void*)pMic);
    /* Audio Driver initialize */
    AUDDSpeakerPhone_Initialize(pDrv, pUsbd, pSpk, pMic);

}

/**
 * Configure function with expected descriptors and start functionality.
 * Usually invoked when device is configured.
 * \pDescriptors Pointer to the descriptors for function configure.
 * \wLength      Length of descriptors in number of bytes.
 */
void AUDDFunction_Configure(USBGenericDescriptor *pDescriptors,
                            uint16_t wLength)
{
    AUDDFunction *pAudf = &auddFunction;
    AUDDSpeakerPhone *pDrv = &pAudf->drv;
    AUDDSpeakerPhone_ParseInterfaces(pDrv, pDescriptors, wLength);
}

/**
 * Invoked whenever the active setting of an interface is changed by the
 * host. Changes the status of the third LED accordingly.
 * \param interface Interface number.
 * \param setting Newly active setting.
 */
void AUDDFunction_InterfaceSettingChangedHandler(uint8_t interface,
                                                 uint8_t setting)
{
    AUDDFunction *pAudf = &auddFunction;
    AUDDSpeakerPhone *pDrv = &pAudf->drv;
    if (setting == 0) AUDDSpeakerPhone_CloseStream(pDrv, interface);
    if (AUDDFunction_StreamSettingChanged) {
        uint8_t mic = (interface == pDrv->pMicrophone->bAsInterface);
        AUDDFunction_StreamSettingChanged(mic, setting);
    }
}

/**
 * Handles AUDIO-specific USB requests sent by the host
 * \param request Pointer to a USBGenericRequest instance.
 * \return USBRC_SUCCESS if request is handled.
 */
uint32_t AUDDFunction_RequestHandler(
    const USBGenericRequest *request)
{
    AUDDFunction *pAudf = &auddFunction;
    AUDDSpeakerPhone *pDrv = &pAudf->drv;
    return AUDDSpeakerPhone_RequestHandler(pDrv, request);
}

/**
 * Reads incoming audio data sent by the USB host into the provided buffer.
 * When the transfer is complete, an optional callback function is invoked.
 * \param buffer Pointer to the data storage buffer.
 * \param length Size of the buffer in bytes.
 * \param callback Optional callback function.
 * \param argument Optional argument to the callback function.
 * \return <USBD_STATUS_SUCCESS> if the transfer is started successfully;
 *         otherwise an error code.
 */
uint8_t AUDDFunction_Read(void *buffer,
                          uint32_t length,
                          TransferCallback callback,
                          void *argument)
{
    AUDDFunction *pAudf = &auddFunction;
    AUDDSpeakerPhone *pDrv = &pAudf->drv;
    return AUDDSpeakerPhone_Read(pDrv, buffer, length, callback, argument);
}

/**
 * Initialize Frame List for sending audio data.
 *
 * \param pListInit Pointer to the allocated list for audio write.
 * \param pDmaInit  Pointer to the allocated DMA descriptors for autio write
 *                  (if DMA supported).
 * \param listSize  Circular list size.
 * \param delaySize Start transfer after delaySize frames filled in.
 * \param callback  Optional callback function for transfer.
 * \param argument  Optional callback argument.
 * \return USBD_STATUS_SUCCESS if setup successfully; otherwise an error code.
 */
uint8_t AUDDFunction_SetupWrite(void * pListInit,
                                void * pDmaInit,
                                uint16_t listSize,
                                uint16_t delaySize,
                                TransferCallback callback,
                                void * argument)
{
    AUDDFunction *pAudf = &auddFunction;
    AUDDSpeakerPhone *pDrv = &pAudf->drv;
    return AUDDSpeakerPhone_SetupWrite(pDrv,
                                       pListInit, pDmaInit, listSize, delaySize,
                                       callback, argument);
}

/**
 *  Add frame buffer to audio sending list.
 *  \buffer Pointer to data frame to send.
 *  \length Frame size in bytes.
 *  \return USBD_STATUS_SUCCESS if the transfer is started successfully;
 *          otherwise an error code.
 */
uint8_t AUDDFunction_Write(void* buffer, uint16_t length)
{
    AUDDFunction *pAudf = &auddFunction;
    AUDDSpeakerPhone *pDrv = &pAudf->drv;
    return AUDDSpeakerPhone_Write(pDrv, buffer, length);
}

/**@}*/

