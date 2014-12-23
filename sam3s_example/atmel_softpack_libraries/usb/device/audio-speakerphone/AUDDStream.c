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
 *  USB Audio Device Streaming interface with controls.
 *  (3 channels supported).
 */

/** \addtogroup usbd_audio_speakerphone
 *@{
 */
 
/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <AUDDSpeakerPhone.h>

#include <USBDescriptors.h>
#include <USBRequests.h>
#include <AUDDescriptors.h>
#include <AUDRequests.h>

#include <USBD_HAL.h>

#include <USBLib_Trace.h>

/*------------------------------------------------------------------------------
 *         Types
 *------------------------------------------------------------------------------*/

/** Parse data extention for descriptor parsing  */
typedef struct _AUDDParseData {
    /** Pointer to AUDDSpeakerPhone instance */
    AUDDSpeakerPhone * pAudf;
    /** Pointer to found interface descriptor */
    USBInterfaceDescriptor * pIfDesc;
    
} AUDDParseData;

/** Transfer callback extention */
typedef struct _AUDDXfrExt {
    /** Pointer to AUDDStream instance */
    AUDDStream *pStream;
    /** Buffer for USB device to get data from host */
    uint16_t    usbBuffer;
    /** Additional information: Entity */
    uint8_t     bEntity;
    /** Additional information: Channel */
    uint8_t     bCh;
} AUDDXfrExt;

/*------------------------------------------------------------------------------
 *         Internal Variable
 *------------------------------------------------------------------------------*/

/** Transfer data extension */
static AUDDXfrExt auddXfrData;

/*------------------------------------------------------------------------------
 *         Internal Functions
 *------------------------------------------------------------------------------*/

/**
 * Parse descriptors: Interface, ISO IN/OUT, Feature Unit IDs.
 * \param desc Pointer to descriptor list.
 * \param arg  Argument, pointer to AUDDParseData instance.
 */
static uint32_t AUDDSpeakerPhone_Parse(USBGenericDescriptor *pDesc,
                                   AUDDParseData * pArg)
{
    AUDDStream *pSpeaker = pArg->pAudf->pSpeaker;
    AUDDStream *pMic     = pArg->pAudf->pMicrophone;
    USBEndpointDescriptor* pEp = (USBEndpointDescriptor*)pDesc;
    uint8_t bSpeakerDone = 0, bMicDone = 0;

    /* Validate descriptor */
    if (pDesc->bLength == 0)
        return USBRC_PARAM_ERR;

    /* Log current interface */
    if (pDesc->bDescriptorType == USBGenericDescriptor_INTERFACE) {
        USBInterfaceDescriptor* pIf = (USBInterfaceDescriptor*)pDesc;
        /* AudioControl interface */
        if (pIf->bInterfaceClass ==
                        AUDControlInterfaceDescriptor_CLASS
            && pIf->bInterfaceSubClass ==
                        AUDControlInterfaceDescriptor_SUBCLASS) {
            pArg->pIfDesc = pIf;

            if (pSpeaker) pSpeaker->bAcInterface = pIf->bInterfaceNumber;
            if (pMic)     pMic->bAcInterface = pIf->bInterfaceNumber;
        }
        /* AudioStreaming interface with endpoint */
        else if (pIf->bInterfaceClass ==
                    AUDStreamingInterfaceDescriptor_CLASS
            && pIf->bInterfaceSubClass ==
                    AUDStreamingInterfaceDescriptor_SUBCLASS) {
            pArg->pIfDesc = pIf;
        }
        /* Not Audio interface, force end */
        else if (pArg->pIfDesc){
            return USBRC_PARTIAL_DONE;
        }
    }

    if (pArg->pIfDesc) {
        /* Find Control Interface */
        /* Find Entities */
        /* Find Streaming Interface & Endpoints */
        if (pDesc->bDescriptorType == USBGenericDescriptor_ENDPOINT
            && (pEp->bmAttributes & 0x3) == USBEndpointDescriptor_ISOCHRONOUS) {
            if (pEp->bEndpointAddress & 0x80
                && pMic) {
                pMic->bEndpointIn = pEp->bEndpointAddress & 0x7F;
                pMic->bAsInterface = pArg->pIfDesc->bInterfaceNumber;
                /* Fixed FU */
                pMic->bFeatureUnitIn = AUDD_ID_MicrophoneFU;
            }
            else if (pSpeaker) {
                pSpeaker->bEndpointOut = pEp->bEndpointAddress;
                pSpeaker->bAsInterface = pArg->pIfDesc->bInterfaceNumber;
                /* Fixed FU */
                pSpeaker->bFeatureUnitOut = AUDD_ID_SpeakerFU;
            }
        }
    }

    if (pSpeaker) {
        if (pSpeaker->bAcInterface != 0xFF
            && pSpeaker->bAsInterface != 0xFF
            && pSpeaker->bFeatureUnitOut != 0xFF
            && pSpeaker->bEndpointOut != 0) {
            bSpeakerDone = 1;
        }
    }
    else    bSpeakerDone = 1;

    if (pMic) {
        if (pMic->bAcInterface != 0xFF
            && pMic->bAsInterface != 0xFF
            && pMic->bFeatureUnitIn != 0xFF
            && pMic->bEndpointIn != 0) {
            bMicDone = 1;
        }
    }
    else    bMicDone = 1;

    if (bSpeakerDone && bMicDone)
        return USBRC_FINISHED;
    
    return USBRC_SUCCESS;
}

/**
 * Callback triggered after the new mute status of a channel has been read.
 * Changes the mute status of the given channel accordingly.
 * \param pData Pointer to AUDDXfrExt (transfer extension data).
 */
static void AUDD_MuteReceived(AUDDXfrExt *pData)
{
    AUDDStream_ChangeMute(pData->pStream,
                          pData->bCh,
                          (uint8_t)pData->usbBuffer);
    USBD_Write(0, 0, 0, 0, 0);
}

/**
 * Callback triggered after the new volume status of a channel has been read.
 * Changes the volume status of the given channel accordingly.
 * \param pData Pointer to AUDDXfrExt (transfer extension data).
 */
static void AUDD_VolumeReceived(AUDDXfrExt *pData)
{
    AUDDStream_SetVolume(pData->pStream,
                         pData->bCh,
                         pData->usbBuffer);
    USBD_Write(0, 0, 0, 0, 0);
}

/**
 * Get Target AUDDStream for control
 * \param pAudf         Pointer to AUDDSpeakerPhone instance.
 * \param bAcInterface  Interface number
 * \param bEntity       Entity ID
 * \param bChannel      Channel number
 * \return Pointer to AUDDStream instance
 */
static AUDDStream *AUDD_GetCtlStream(
    AUDDSpeakerPhone *pAudf,
    uint8_t bAcInterface,
    uint8_t bEntity,
    uint8_t bChannel)
{
    AUDDStream *pAuds = 0;

    if (bEntity == pAudf->pSpeaker->bFeatureUnitOut
        || bEntity == pAudf->pSpeaker->bFeatureUnitIn)
        pAuds = pAudf->pSpeaker;
    else if (bEntity == pAudf->pMicrophone->bFeatureUnitIn
        || bEntity == pAudf->pMicrophone->bFeatureUnitOut)
        pAuds = pAudf->pMicrophone;

    if (pAuds != 0
        && bAcInterface == pAuds->bAcInterface
        && bChannel <= pAuds->bNumChannels) {
        return pAuds;
    }

    return 0;
}

/**
 * Handle the SET_CUR request.
 * \param pAudf Pointer to AUDDSpeakerPhone instance.
 * \param pReq  Pointer to USBGenericRequest instance.
 */
static void AUDD_SetCUR(
    AUDDSpeakerPhone *pAudf,
    const USBGenericRequest* pReq)
{
    uint8_t bIf     = AUDGenericRequest_GetInterface(pReq);
    uint8_t bEntity = AUDGenericRequest_GetEntity(pReq);
    uint8_t bLength = USBGenericRequest_GetLength(pReq);
    uint8_t bCh     = AUDFeatureUnitRequest_GetChannel(pReq);
    uint8_t bCtrl   = AUDFeatureUnitRequest_GetControl(pReq);
    uint8_t bSet = 1;
    AUDDStream *pAuds = AUDD_GetCtlStream(pAudf, bIf, bEntity, bCh);
    TransferCallback fCallback;

    TRACE_INFO_WP("sCUR ");
    TRACE_DEBUG("\b(E%d, CtlS%d, Ch%d, L%d) ", bEntity, bCtrl, bCh, bLength);

    /* Set Mute to AC, 1 byte */
    if (bCtrl == AUDFeatureUnitRequest_MUTE
        && bLength == 1
        && pAuds) {
        fCallback = (TransferCallback) AUDD_MuteReceived;
    }
    else if (bCtrl == AUDFeatureUnitRequest_VOLUME
        && bLength == 2
        && pAuds && pAuds->pwVolumes) {
        fCallback = (TransferCallback) AUDD_VolumeReceived;
    }
    else
        bSet = 0;

    if (bSet) {

        auddXfrData.pStream = pAuds;
        auddXfrData.bEntity = bEntity;
        auddXfrData.bCh     = bCh;
        USBD_Read(0,
                  &auddXfrData.usbBuffer,
                  bLength,
                  fCallback,
                  (void *) &auddXfrData);
    }
    else {

        USBD_Stall(0);
    }

}

/**
 * Handle the GET_CUR request.
 * \param pAudf Pointer to AUDDSpeakerPhone instance.
 * \param pReq  Pointer to USBGenericRequest instance.
 */
static void AUDD_GetCUR(
    AUDDSpeakerPhone *pAudf,
    const USBGenericRequest *pReq)
{
    uint8_t bIf     = AUDGenericRequest_GetInterface(pReq);
    uint8_t bEntity = AUDGenericRequest_GetEntity(pReq);
    uint8_t bLength = USBGenericRequest_GetLength(pReq);
    uint8_t bCh     = AUDFeatureUnitRequest_GetChannel(pReq);
    uint8_t bCtrl   = AUDFeatureUnitRequest_GetControl(pReq);
    uint8_t bGet = 1;
    AUDDStream *pAuds = AUDD_GetCtlStream(pAudf, bIf, bEntity, bCh);

    TRACE_INFO_WP("gCUR ");
    TRACE_DEBUG("\b(E%d, CtlS%d, Ch%d, L%d) ", bEntity, bCtrl, bCh, bLength);

    /* Get Mute 1 byte */
    if (bCtrl == AUDFeatureUnitRequest_MUTE
        && bLength == 1
        && pAuds) {
        auddXfrData.usbBuffer = ((pAuds->bmMute & (1<<bCh)) > 0);
    }
    else if (bCtrl == AUDFeatureUnitRequest_VOLUME
        && bLength == 2
        && pAuds && pAuds->pwVolumes) {
        auddXfrData.usbBuffer = pAuds->pwVolumes[bCh];
    }
    else
        bGet = 0;

    if (bGet) {

        USBD_Write(0, &auddXfrData.usbBuffer, bLength, 0, 0);
    }
    else {

        USBD_Stall(0);
    }
}

/*------------------------------------------------------------------------------
 *         Exported Functions
 *------------------------------------------------------------------------------*/

/**
 * Initialize AUDDStream instance.
 * Note the number of channels excludes the master control, so
 * actual volume array size should be (1 + numChannels).
 * \param pAuds Pointer to AUDDStream instance.
 * \param numChannels     Number of channels in the stream (<31).
 * \param wChannelVolumes Data array for channel volume values.
 * \param fCallback Callback function for stream events.
 * \param pArg      Pointer to event handler arguments.
 */
void AUDDStream_Initialize(AUDDStream *pAuds,
                           uint8_t     numChannels,
                           uint16_t    wChannelVolumes[],
                           AUDDStreamEventCallback fCallback,
                           void* pArg)
{
    pAuds->bAcInterface    = 0xFF;
    pAuds->bFeatureUnitOut = 0xFF;
    pAuds->bFeatureUnitIn  = 0xFF;
    pAuds->bAsInterface    = 0xFF;
    pAuds->bEndpointOut    = 0;
    pAuds->bEndpointIn     = 0;

    pAuds->bNumChannels   = numChannels;
    pAuds->bmMute         = 0;
    pAuds->pwVolumes      = wChannelVolumes;

    pAuds->fCallback = fCallback;
    pAuds->pArg      = pArg;
}

/**
 * Check if the request is accepted.
 * \param pAuds Pointer to AUDDStream instance.
 * \param pReq  Pointer to a USBGenericRequest instance.
 * \return 1 if accepted.
 */
uint32_t AUDDStream_IsRequestAccepted(
    AUDDStream *pAuds,
    const USBGenericRequest *pReq)
{
    uint8_t bIf     = AUDGenericRequest_GetInterface(pReq);
    uint8_t bEntity = AUDGenericRequest_GetEntity(pReq);
    uint8_t bCh     = AUDFeatureUnitRequest_GetChannel(pReq);
    /* AudioControl Interface */
    if (bIf == pAuds->bAcInterface) {
        if (bCh > pAuds->bNumChannels)
            return 0;
        if (bEntity != pAuds->bFeatureUnitIn
            && bEntity != pAuds->bFeatureUnitOut)
            return 0;
    }
    /* AudioStream Interface not handled */
    else {
        return 0;
    }
    return 1;
}

/**
 * Change Stream Mute status.
 * \param pAuds     Pointer to AUDDStream instance.
 * \param bChannel  Channel number.
 * \param bmMute     1 to mute, 0 to unmute.
 */
uint32_t AUDDStream_ChangeMute(AUDDStream *pAuds,
                               uint8_t bChannel,
                               uint8_t bMute)
{
    uint8_t bmMute = (bMute << bChannel);

    if (pAuds->bNumChannels < bChannel)
        return USBRC_PARAM_ERR;

    if (bMute)
        pAuds->bmMute |=  bmMute;
    else
        pAuds->bmMute &= ~bmMute;

    if (pAuds->fCallback)
        pAuds->fCallback(AUDD_EC_MuteChanged,
                         bChannel,
                         pAuds->pArg);

    return USBRC_SUCCESS;
}

/**
 * Set Stream Volume status.
 * \param pAuds     Pointer to AUDDStream instance.
 * \param bChannel  Channel number.
 * \param wVolume   New volume value.
 */
uint32_t AUDDStream_SetVolume(AUDDStream *pAuds,
                              uint8_t  bChannel,
                              uint16_t wVolume)
{
    if (pAuds->pwVolumes == 0)
        return USBRC_PARAM_ERR;
    if (bChannel > pAuds->bNumChannels)
        return USBRC_PARAM_ERR;

    pAuds->pwVolumes[bChannel] = wVolume;
    if (pAuds->fCallback) {
        pAuds->fCallback(AUDD_EC_VolumeChanged,
                         bChannel,
                         pAuds->pArg);
    }

    return USBRC_SUCCESS;
}

/**
 * Receives data from the host through the audio function (as speaker).
 * This function behaves like USBD_Read.
 * \param pAuds        Pointer to AUDDStream instance.
 * \param pData  Pointer to the data buffer to put received data.
 * \param dwSize Size of the data buffer in bytes.
 * \param fCallback Optional callback function to invoke when the transfer
 *                  finishes.
 * \param pArg      Optional argument to the callback function.
 * \return USBD_STATUS_SUCCESS if the read operation has been started normally;
 *         otherwise, the corresponding error code.
 */
uint32_t AUDDStream_Read(
    AUDDStream *pAuds,
    void * pData,uint32_t dwSize,
    TransferCallback fCallback,void * pArg)
{
    if (pAuds->bEndpointOut == 0)
        return USBRC_PARAM_ERR;
    return USBD_Read(pAuds->bEndpointOut,
                     pData, dwSize,
                     fCallback, pArg);
}

/**
 * Initialize Frame List for sending audio data.
 * \param pAuds     Pointer to AUDDStream instance.
 * \param pListInit Pointer to the allocated list for audio write.
 * \param pDmaInit  Pointer to the allocated DMA descriptors for autio write
 *                  (if DMA supported).
 * \param listSize  Circular list size.
 * \param delaySize Start transfer after delaySize frames filled in.
 * \param callback  Optional callback function for transfer.
 * \param argument  Optional callback argument.
 * \return USBD_STATUS_SUCCESS if setup successfully; otherwise an error code.
 */
uint32_t AUDDStream_SetupWrite(
    AUDDStream *pAuds,
    void * pListInit,
    void * pDmaInit,
    uint16_t listSize,
    uint16_t delaySize,
    TransferCallback callback,
    void * argument)
{
    uint32_t error;

    if (pAuds->bEndpointIn == 0)
        return USBRC_STATE_ERR;

    error = USBD_HAL_SetupMblTransfer(pAuds->bEndpointIn,
                                      pListInit,
                                      listSize,
                                      delaySize);
    if (error)  return error;

    error = USBD_HAL_SetTransferCallback(pAuds->bEndpointIn,
                                         callback, argument);
    return error;
}


/**
 *  Add frame buffer to audio sending list.
 *  \param pAuds   Pointer to AUDDStream instance.
 *  \param pBuffer Pointer to data frame to send.
 *  \param wLength Frame size in bytes.
 *  \return USBD_STATUS_SUCCESS if the transfer is started successfully;
 *          otherwise an error code.
 */
uint32_t AUDDStream_Write(AUDDStream *pAuds, void* pBuffer, uint16_t wLength)
{
    if (pAuds->bEndpointIn == 0)
        return USBRC_STATE_ERR;

    return USBD_HAL_Write(pAuds->bEndpointIn,
                          pBuffer, wLength);
}

/**
 * Close the stream. All pending transfers are canceled.
 * \param pStream Pointer to AUDDStream instance.
 */
uint32_t AUDDStream_Close(AUDDStream *pStream)
{
    uint32_t bmEPs = 0;

    /* Close output stream */
    if (pStream->bEndpointIn) {
        bmEPs |= 1 << pStream->bEndpointIn;
    }
    /* Close input stream */
    if (pStream->bEndpointOut) {
        bmEPs |= 1 << pStream->bEndpointOut;
    }
    USBD_HAL_ResetEPs(bmEPs, USBRC_CANCELED, 1);

    return USBRC_SUCCESS;
}

/*
 *          Audio Speakerphone functions
 */

/**
 * Initialize AUDDStream instance.
 * Note the number of channels excludes the master control, so
 * actual volume array size should be (1 + numChannels).
 * \param pAuds Pointer to AUDDStream instance.
 * \param numChannels Number of channels in the stream (excluding master,<31).
 * \param wChannelVolumes Data array for channel volume values,
 *                        must include master (1 + numChannels).
 * \param fCallback Callback function for stream control events.
 * \param pArg      Pointer to event handler arguments.
 */
void AUDDSpeakerPhone_InitializeStream(
    AUDDStream *pAuds,
    uint8_t     numChannels,
    uint16_t    wChannelVolumes[],
    AUDDStreamEventCallback fCallback,
    void* pArg)
{
    pAuds->bAcInterface    = 0xFF;
    pAuds->bFeatureUnitOut = 0xFF;
    pAuds->bFeatureUnitIn  = 0xFF;
    pAuds->bAsInterface    = 0xFF;
    pAuds->bEndpointOut    = 0;
    pAuds->bEndpointIn     = 0;

    pAuds->bNumChannels   = numChannels;
    pAuds->bmMute         = 0;
    pAuds->pwVolumes      = wChannelVolumes;

    pAuds->fCallback = fCallback;
    pAuds->pArg      = pArg;
}

/**
 * Initialize AUDDSpeakerPhone instance.
 * \param pAudf       Pointer to AUDDSpeakerPhone instance.
 * \param pUsbd       Pointer to USBDDriver instance.
 * \param pSpeaker    Pointer to speaker streaming interface.
 * \param pMicrophone Pointer to microphone streaming interface.
 */
void AUDDSpeakerPhone_Initialize(
    AUDDSpeakerPhone *pAudf,
    USBDDriver *pUsbd,
    AUDDStream *pSpeaker,
    AUDDStream *pMicrophone)
{
    pAudf->pUsbd       = pUsbd;
    pAudf->pSpeaker    = pSpeaker;
    pAudf->pMicrophone = pMicrophone;
}

/**
 * Parse USB Audio streaming information for AUDDStream instance.
 * \param pAudf        Pointer to AUDDSpeakerPhone instance.
 * \param pDescriptors Pointer to descriptor list.
 * \param dwLength     Descriptor list size in bytes.
 */
USBGenericDescriptor *AUDDSpeakerPhone_ParseInterfaces(
    AUDDSpeakerPhone *pAudf,
    USBGenericDescriptor *pDescriptors,
    uint32_t dwLength)
{
    AUDDParseData data;

    data.pAudf = pAudf;
    data.pIfDesc = 0;

    return USBGenericDescriptor_Parse(pDescriptors,
                                      dwLength,
                                      (USBDescriptorParseFunction)AUDDSpeakerPhone_Parse,
                                      (void*)&data);
}

/**
 * Close the stream. All pending transfers are canceled.
 * \param pAudf        Pointer to AUDDSpeakerPhone instance.
 * \param bInterface   Stream interface number
 */
uint32_t AUDDSpeakerPhone_CloseStream(
    AUDDSpeakerPhone *pAudf,
    uint32_t bInterface)
{
    if (pAudf->pSpeaker->bAsInterface == bInterface) {
        USBD_HAL_ResetEPs(1 << pAudf->pSpeaker->bEndpointOut,
                          USBRC_CANCELED,
                          1);
    }
    else if (pAudf->pMicrophone->bAsInterface == bInterface) {
        USBD_HAL_ResetEPs(1 << pAudf->pMicrophone->bEndpointIn,
                          USBRC_CANCELED,
                          1);
    }

    return USBRC_SUCCESS;
}

/**
 *  Handles audio-specific USB requests sent by the host
 *  \param pAudf    Pointer to AUDDSpeakerPhone instance.
 *  \param pRequest Pointer to a USBGenericRequest instance.
 *  \return USBRC_PARAM_ERR if not handled.
 */
uint32_t AUDDSpeakerPhone_RequestHandler(
    AUDDSpeakerPhone *pAudf,
    const USBGenericRequest* pRequest)
{
    //USBDDriver *pUsbd = pAudf->pUsbd;

    if (USBGenericRequest_GetType(pRequest) != USBGenericRequest_CLASS)
        return USBRC_PARAM_ERR;

    TRACE_INFO_WP("Aud ");
    switch (USBGenericRequest_GetRequest(pRequest)) {
    case AUDGenericRequest_SETCUR:
        AUDD_SetCUR(pAudf, pRequest);
        break;
    case AUDGenericRequest_GETCUR:
        AUDD_GetCUR(pAudf, pRequest);
        break;

    default:
        return USBRC_PARAM_ERR;
    }

    return USBRC_SUCCESS;
}

/**
 * Receives data from the host through the audio function (as speaker).
 * This function behaves like USBD_Read.
 * \param pAudf        Pointer to AUDDSpeakerPhone instance.
 * \param pData  Pointer to the data buffer to put received data.
 * \param dwSize Size of the data buffer in bytes.
 * \param fCallback Optional callback function to invoke when the transfer
 *                  finishes.
 * \param pArg      Optional argument to the callback function.
 * \return USBD_STATUS_SUCCESS if the read operation has been started normally;
 *         otherwise, the corresponding error code.
 */
uint32_t AUDDSpeakerPhone_Read(
    AUDDSpeakerPhone *pAudf,
    void * pData,uint32_t dwSize,
    TransferCallback fCallback,void * pArg)
{
    if (pAudf->pSpeaker == 0)
        return USBRC_PARAM_ERR;
    if (pAudf->pSpeaker->bEndpointOut == 0)
        return USBRC_PARAM_ERR;
    return USBD_Read(pAudf->pSpeaker->bEndpointOut,
                     pData, dwSize,
                     fCallback, pArg);
}

/**
 * Initialize Frame List for sending audio data.
 * \param pAudf     Pointer to AUDDSpeakerPhone instance.
 * \param pListInit Pointer to the allocated list for audio write.
 * \param pDmaInit  Pointer to the allocated DMA descriptors for autio write
 *                  (if DMA supported).
 * \param listSize  Circular list size.
 * \param delaySize Start transfer after delaySize frames filled in.
 * \param callback  Optional callback function for transfer.
 * \param argument  Optional callback argument.
 * \return USBD_STATUS_SUCCESS if setup successfully; otherwise an error code.
 */
uint32_t AUDDSpeakerPhone_SetupWrite(
    AUDDSpeakerPhone *pAudf,
    void * pListInit,
    void * pDmaInit,
    uint16_t listSize,
    uint16_t delaySize,
    TransferCallback callback,
    void * argument)
{
    uint32_t error;

    if (pAudf->pMicrophone == 0)
        return USBRC_PARAM_ERR;
    if (pAudf->pMicrophone->bEndpointIn == 0)
        return USBRC_STATE_ERR;

    error = USBD_HAL_SetupMblTransfer(pAudf->pMicrophone->bEndpointIn,
                                      pListInit,
                                      listSize,
                                      delaySize);
    if (error)  return error;

    error = USBD_HAL_SetTransferCallback(
                                    pAudf->pMicrophone->bEndpointIn,
                                    callback, argument);
    return error;
}


/**
 *  Add frame buffer to audio sending list.
 *  \param pAudf   Pointer to AUDDSpeakerPhone instance.
 *  \param pBuffer Pointer to data frame to send.
 *  \param wLength Frame size in bytes.
 *  \return USBD_STATUS_SUCCESS if the transfer is started successfully;
 *          otherwise an error code.
 */
uint32_t AUDDSpeakerPhone_Write(AUDDSpeakerPhone *pAudf, void* pBuffer, uint16_t wLength)
{
    if (pAudf->pSpeaker == 0)
        return USBRC_PARAM_ERR;
    if (pAudf->pSpeaker->bEndpointIn == 0)
        return USBRC_STATE_ERR;

    return USBD_HAL_Write(pAudf->pSpeaker->bEndpointIn,
                          pBuffer, wLength);
}

/**@}*/

