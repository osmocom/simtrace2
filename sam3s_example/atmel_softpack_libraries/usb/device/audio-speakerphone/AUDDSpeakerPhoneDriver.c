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
 * \addtogroup usbd_audio_speakerphone
 *@{
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <AUDDSpeakerPhoneDriver.h>

#include <AUDRequests.h>

#include <USBLib_Trace.h>

/*------------------------------------------------------------------------------
 *         Internal types
 *------------------------------------------------------------------------------*/

/**
 * Structs of USB Audio Stream Function Interface.
 */
typedef struct _AUDDStream {

    /* -- USB Interface settings -- */
    /** Audio Control Interface Number */
    uint8_t  bAcInterface;
    /** Audio Streaming Interface Number */
    uint8_t  bAsInterface;
    /** Audio Streaming endpoint address */
    uint8_t  bEpNum;
    /** Audio Control Unit ID */
    uint8_t  bUnitID;

    /* -- Channel settings -- */
    /** Number of channels (including master 0, max 32) */
    uint16_t  bNumChannels;
    /** Mute Controls bitmap */
    uint16_t  bmMuteControls;
    /** Volume Controls (Master,L,R..) array */
    uint16_t *pVolumes;
} AUDDStream;

/**
 * \brief Audio SpeakerPhone driver internal state.
 */
typedef struct _AUDDSpeakerPhoneDriver {

    /** Pointer to USBDDriver instance */
    USBDDriver * pUsbd;
    /** Intermediate storage variable for the mute status of a stream */
    uint8_t    muted;
    /** Array for storing the current setting of each interface. */
    uint8_t    interfaces[3];
    /** Audio Speaker interface */
    AUDDStream speaker;
    /** Audio Microphone interface */
    AUDDStream mic;
} AUDDSpeakerPhoneDriver;

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/

/** Global USB audio SpeakerPhone driver instance. */
static AUDDSpeakerPhoneDriver auddSpeakerPhoneDriver;

/*------------------------------------------------------------------------------
 *         Internal functions
 *------------------------------------------------------------------------------*/

/**
 * Parse descriptors: Interrupt IN, Bulk EP IN/OUT.
 * \param desc Pointer to descriptor.
 * \param arg  Argument, pointer to AUDDSpeakerPhoneDriver instance.
 */
static uint32_t AUDDSpeakerPhone_Parse(USBGenericDescriptor* desc,
                                       AUDDSpeakerPhoneDriver* arg)
{
    /* Not a valid descriptor */
    if (desc->bLength == 0) {
        return USBD_STATUS_INVALID_PARAMETER;
    }
    /* Parse endpoint descriptor */
    if (desc->bDescriptorType == USBGenericDescriptor_ENDPOINT) {
        USBEndpointDescriptor *pEP = (USBEndpointDescriptor*)desc;
        if (pEP->bmAttributes == USBEndpointDescriptor_ISOCHRONOUS) {
            if (pEP->bEndpointAddress & 0x80)
                arg->mic.bEpNum = pEP->bEndpointAddress & 0x7F;
            else
                arg->speaker.bEpNum = pEP->bEndpointAddress;
        }
    }
    return 0;
}

/**
 * Callback triggered after the new mute status of a channel has been read
 * by AUDDSpeakerPhoneDriver_SetFeatureCurrentValue. Changes the mute status
 * of the given channel accordingly.
 * \param channel Number of the channel whose mute status has changed.
 */
static void AUDDSpeakerPhone_MuteReceived(uint32_t channel)
{
    AUDDSpeakerPhoneDriver *pAudd = &auddSpeakerPhoneDriver;
    AUDDStream *pAuds;

    if ((uint8_t)(channel >> 8) ==
        AUDDSpeakerPhoneDriverDescriptors_OUTPUTTERMINAL_REC) {
        pAuds = &pAudd->mic;
    }
    else {
        pAuds = &pAudd->speaker;
    }

    if (pAudd->muted != pAuds->bmMuteControls) {
        pAuds->bmMuteControls = pAudd->muted;
        AUDDSpeakerPhoneDriver_MuteChanged(0, channel, pAudd->muted);
    }
    USBD_Write(0, 0, 0, 0, 0);
}

/**
 * Handle the SET_CUR request.
 * \param pReq Pointer to USBGenericRequest instance.
 */
static void AUDDSpeakerPhone_SetCUR(const USBGenericRequest* pReq)
{
    AUDDSpeakerPhoneDriver *pAudd = &auddSpeakerPhoneDriver;
    uint8_t bIf     = AUDGenericRequest_GetInterface(pReq);
    uint8_t bEntity = AUDGenericRequest_GetEntity(pReq);
    uint8_t bLength = USBGenericRequest_GetLength(pReq);
    uint8_t bCh     = AUDFeatureUnitRequest_GetChannel(pReq);
    uint8_t bCtrl   = AUDFeatureUnitRequest_GetControl(pReq);
    uint8_t bSet = 0;
    AUDDStream *pAuds = 0;

    TRACE_INFO_WP("sCUR ");
    TRACE_DEBUG("\b(E%d, CtlS%d, Ch%d, L%d) ", bEntity, bCtrl, bCh, bLength);
    /* Only AC.FeatureUnit accepted */
    if (bCtrl == AUDFeatureUnitRequest_MUTE
        && bLength == 1) {

        if (bEntity == pAudd->speaker.bUnitID)
            pAuds = &pAudd->speaker;
        else if (bEntity == pAudd->mic.bUnitID)
            pAuds = &pAudd->mic;

        if (pAuds != 0
            && bIf == pAuds->bAcInterface
            && bCh <= pAuds->bNumChannels) {
            bSet = 1;
        }
    }

    if (bSet) {

        uint32_t argument = bCh | (bEntity << 8);
        USBD_Read(0, /* Endpoint #0 */
                  &pAudd->muted,
                  sizeof(uint8_t),
                  (TransferCallback) AUDDSpeakerPhone_MuteReceived,
                  (void *) argument);
    }
    else {

        USBD_Stall(0);
    }

}

/**
 * Handle the GET_CUR request.
 * \param pReq Pointer to USBGenericRequest instance.
 */
static void AUDDSpeakerPhone_GetCUR(const USBGenericRequest *pReq)
{
    AUDDSpeakerPhoneDriver *pAudd = &auddSpeakerPhoneDriver;
    uint8_t bIf     = AUDGenericRequest_GetInterface(pReq);
    uint8_t bEntity = AUDGenericRequest_GetEntity(pReq);
    uint8_t bLength = USBGenericRequest_GetLength(pReq);
    uint8_t bCh     = AUDFeatureUnitRequest_GetChannel(pReq);
    uint8_t bCtrl   = AUDFeatureUnitRequest_GetControl(pReq);
    uint8_t bGet = 0;
    AUDDStream *pAuds = 0;

    TRACE_INFO_WP("gCUR ");
    TRACE_DEBUG("\b(E%d, CtlS%d, Ch%d, L%d) ", bEntity, bCtrl, bCh, bLength);
    /* Only AC.FeatureUnit accepted */
    if (bCtrl == AUDFeatureUnitRequest_MUTE
        && bLength == 1) {

        if (bEntity == pAudd->speaker.bUnitID)
            pAuds = &pAudd->speaker;
        else if (bEntity == pAudd->mic.bUnitID)
            pAuds = &pAudd->mic;

        if (pAuds != 0
            && bIf == pAuds->bAcInterface
            && bCh <= pAuds->bNumChannels) {
            bGet = 1;
        }
    }

    if (bGet) {

        pAudd->muted = pAuds->bmMuteControls;
        USBD_Write(0, &pAudd->muted, sizeof(uint8_t), 0, 0);
    }
    else {

        USBD_Stall(0);
    }
}

/*------------------------------------------------------------------------------
 *         Exported functions
 *------------------------------------------------------------------------------*/

/**
 * Initializes an USB audio SpeakerPhone device driver, as well as the underlying
 * USB controller.
 */
void AUDDSpeakerPhoneDriver_Initialize(const USBDDriverDescriptors *pDescriptors)
{
    AUDDSpeakerPhoneDriver *pAudd = &auddSpeakerPhoneDriver;
    USBDDriver *pUsbd = USBD_GetDriver();

    pAudd->pUsbd = pUsbd;

    /* Initialize SpeakerPhone channels */
    pAudd->speaker.bNumChannels = 3;
    pAudd->speaker.bmMuteControls = 0;
    pAudd->speaker.pVolumes = 0;

    pAudd->mic.bNumChannels = 1;
    pAudd->mic.bmMuteControls = 0;
    pAudd->mic.pVolumes = 0;

    pAudd->mic.bAcInterface = AUDDSpeakerPhoneDriverDescriptors_CONTROL;
    pAudd->mic.bAsInterface = AUDDSpeakerPhoneDriverDescriptors_STREAMINGIN;
    pAudd->mic.bEpNum = 5;//AUDDSpeakerPhoneDriverDescriptors_DATAIN;
    pAudd->mic.bUnitID = AUDDSpeakerPhoneDriverDescriptors_FEATUREUNIT_REC;

    pAudd->speaker.bAcInterface = AUDDSpeakerPhoneDriverDescriptors_CONTROL;
    pAudd->speaker.bAsInterface = AUDDSpeakerPhoneDriverDescriptors_STREAMING;
    pAudd->speaker.bEpNum = 4;//AUDDSpeakerPhoneDriverDescriptors_DATAOUT;
    pAudd->speaker.bUnitID = AUDDSpeakerPhoneDriverDescriptors_FEATUREUNIT;

    /* Initialize the USB driver */
    USBDDriver_Initialize(pUsbd,
                          pDescriptors,
                          pAudd->interfaces);
    USBD_Init();

}

/**
 * Invoked whenever the active configuration of device is changed by the
 * host.
 * \param cfgnum Configuration number.
 */
void AUDDSpeakerPhoneDriver_ConfigurationChangeHandler(uint8_t cfgnum)
{
    AUDDSpeakerPhoneDriver *pAudd = &auddSpeakerPhoneDriver;
    const USBDDriverDescriptors *pDescriptors = pAudd->pUsbd->pDescriptors;
    USBConfigurationDescriptor *pDesc;

    if (cfgnum > 0) {

        /* Parse endpoints for data & notification */
        if (USBD_HAL_IsHighSpeed() && pDescriptors->pHsConfiguration)
            pDesc = (USBConfigurationDescriptor*)pDescriptors->pHsConfiguration;
        else
            pDesc = (USBConfigurationDescriptor*)pDescriptors->pFsConfiguration;

        USBGenericDescriptor_Parse((USBGenericDescriptor*)pDesc, pDesc->wTotalLength,
                    (USBDescriptorParseFunction)AUDDSpeakerPhone_Parse, pAudd);
    }
}

/**
 * Invoked whenever the active setting of an interface is changed by the
 * host. Changes the status of the third LED accordingly.
 * \param interface Interface number.
 * \param setting Newly active setting.
 */
void AUDDSpeakerPhoneDriver_InterfaceSettingChangedHandler(uint8_t interface,
                                                           uint8_t setting)
{
    AUDDSpeakerPhoneDriver *pAudd = &auddSpeakerPhoneDriver;

    if (interface == pAudd->speaker.bAsInterface) {
        /* reset ISO OUT ep */
        if (setting == 0 && pAudd->speaker.bEpNum) {
            USBD_HAL_ResetEPs(1 << pAudd->speaker.bEpNum,
                              USBD_STATUS_CANCELED, 1);
        }
        AUDDSpeakerPhoneDriver_StreamSettingChanged(0, setting);
    }
    if (interface == pAudd->mic.bAsInterface) {
        /* reset ISO IN ep */
        if (setting == 0 && pAudd->mic.bEpNum) {
            USBD_HAL_ResetEPs(1 << pAudd->mic.bEpNum,
                              USBD_STATUS_CANCELED, 1);
        }
        AUDDSpeakerPhoneDriver_StreamSettingChanged(1, setting);
    }
}


/**
 *  Handles audio-specific USB requests sent by the host, and forwards
 *  standard ones to the USB device driver.
 *  \param request Pointer to a USBGenericRequest instance.
 */
void AUDDSpeakerPhoneDriver_RequestHandler(const USBGenericRequest *request)
{
    AUDDSpeakerPhoneDriver *pAudd = &auddSpeakerPhoneDriver;
    USBDDriver *pUsbd = pAudd->pUsbd;

    TRACE_INFO_WP("NewReq ");

    /* Check if this is a class request */
    if (USBGenericRequest_GetType(request) == USBGenericRequest_CLASS) {

        /* Check if the request is supported */
        switch (USBGenericRequest_GetRequest(request)) {

            case AUDGenericRequest_SETCUR:

                AUDDSpeakerPhone_SetCUR(request);
                break;

            case AUDGenericRequest_GETCUR:

                AUDDSpeakerPhone_GetCUR(request);
                break;

            default:

                TRACE_WARNING(
                          "AUDDSpeakerPhoneDriver_RequestHandler: Unsupported request (%d)\n\r",
                          USBGenericRequest_GetRequest(request));
                USBD_Stall(0);
        }
    }
    /* Check if this is a standard request */
    else if (USBGenericRequest_GetType(request) == USBGenericRequest_STANDARD) {

        /* Forward request to the standard handler */
        USBDDriver_RequestHandler(pUsbd, request);
    }
    /* Unsupported request type */
    else {

        TRACE_WARNING(
                  "AUDDSpeakerPhoneDriver_RequestHandler: Unsupported request type (%d)\n\r",
                  USBGenericRequest_GetType(request));
        USBD_Stall(0);
    }
}

/**
 * Reads incoming audio data sent by the USB host into the provided
 * buffer. When the transfer is complete, an optional callback function is
 * invoked.
 * \param buffer Pointer to the data storage buffer.
 * \param length Size of the buffer in bytes.
 * \param callback Optional callback function.
 * \param argument Optional argument to the callback function.
 * \return USBD_STATUS_SUCCESS if the transfer is started successfully;
 *         otherwise an error code.
 */
uint8_t AUDDSpeakerPhoneDriver_Read(void *buffer,
                                    uint32_t length,
                                    TransferCallback callback,
                                    void *argument)
{
    AUDDSpeakerPhoneDriver *pAudd = &auddSpeakerPhoneDriver;
    return USBD_Read(pAudd->speaker.bEpNum,
                     buffer,
                     length,
                     callback,
                     argument);
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
uint8_t AUDDSpeakerPhoneDriver_SetupWrite(void * pListInit,
                                          void * pDmaInit,
                                          uint16_t listSize,
                                          uint16_t delaySize,
                                          TransferCallback callback,
                                          void * argument)
{
    AUDDSpeakerPhoneDriver *pAudd = &auddSpeakerPhoneDriver;
    uint8_t error;

    if (pAudd->mic.bEpNum == 0)
        return USBRC_STATE_ERR;

    error = USBD_HAL_SetupMblTransfer(pAudd->mic.bEpNum,
                                      pListInit,
                                      listSize,
                                      delaySize);
    if (error)  return error;
    error = USBD_HAL_SetTransferCallback(
                                    pAudd->mic.bEpNum,
                                    callback, argument);
    return error;
}

/**
 *  Add frame buffer to audio sending list.
 *  \buffer Pointer to data frame to send.
 *  \length Frame size in bytes.
 *  \return USBD_STATUS_SUCCESS if the transfer is started successfully;
 *          otherwise an error code.
 */
uint8_t AUDDSpeakerPhoneDriver_Write(void* buffer, uint16_t length)
{
    AUDDSpeakerPhoneDriver *pAudd = &auddSpeakerPhoneDriver;

    return USBD_HAL_Write(pAudd->mic.bEpNum,
                          buffer, length);
}

/**@}*/
