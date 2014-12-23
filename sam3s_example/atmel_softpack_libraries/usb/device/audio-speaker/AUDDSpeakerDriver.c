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
 * \addtogroup usbd_audio_speaker
 *@{
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include <AUDDSpeakerDriver.h>

#include <AUDDSpeakerPhone.h>

#include <USBLib_Trace.h>

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
typedef struct _AUDDSpeakerDriver {
    /** Speaker & Phone function */
    AUDDSpeakerPhone fun;
    /** Stream instance for speaker */
    AUDDStream  speaker;
    /** Array for storing the current setting of each interface */
    uint8_t     bAltInterfaces[AUDDSpeakerDriver_NUMINTERFACES];
} AUDDSpeakerDriver;

/*----------------------------------------------------------------------------
 *         Internal variables
 *----------------------------------------------------------------------------*/

/** Global USB audio speaker driver instance. */
static AUDDSpeakerDriver auddSpeakerDriver;

/*----------------------------------------------------------------------------
 *         Dummy callbacks
 *----------------------------------------------------------------------------*/

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
static void AUDDSpeaker_EventCallback(uint32_t ec,
                                      uint8_t channel,
                                      AUDDStream *pArg)
{
    if (ec == AUDD_EC_MuteChanged) {
        if (AUDDSpeakerDriver_MuteChanged)
            AUDDSpeakerDriver_MuteChanged(channel, pArg->bmMute);
    }
    else if (ec == AUDD_EC_VolumeChanged) {
        /* Not supported now */
    }
}

/*----------------------------------------------------------------------------
 *         Exported functions
 *----------------------------------------------------------------------------*/

/**
 * Initializes an USB audio speaker device driver, as well as the underlying
 * USB controller.
 */
void AUDDSpeakerDriver_Initialize(const USBDDriverDescriptors *pDescriptors)
{
    AUDDSpeakerDriver *pAudd = &auddSpeakerDriver;
    AUDDSpeakerPhone *pAudf  = &pAudd->fun;
    AUDDStream *pAuds = &pAudd->speaker;
    USBDDriver *pUsbd = USBD_GetDriver();

    AUDDSpeakerPhone_InitializeStream(
        pAuds, AUDDSpeakerDriver_NUMCHANNELS, 0,
        (AUDDStreamEventCallback)AUDDSpeaker_EventCallback, pAuds);

    AUDDSpeakerPhone_Initialize(
        pAudf, pUsbd, pAuds, 0);

    /* Initialize the USB driver */
    USBDDriver_Initialize(pUsbd,
                          pDescriptors,
                          pAudd->bAltInterfaces);
    USBD_Init();
}

/**
 * Invoked whenever the active configuration of device is changed by the
 * host.
 * \param cfgnum Configuration number.
 */
void AUDDSpeakerDriver_ConfigurationChangeHandler(uint8_t cfgnum)
{
    AUDDSpeakerDriver *pAudd = &auddSpeakerDriver;
    AUDDSpeakerPhone *pAudf = &pAudd->fun;
    const USBDDriverDescriptors *pDescriptors = pAudf->pUsbd->pDescriptors;
    USBConfigurationDescriptor *pDesc;

    if (cfgnum > 0) {

        /* Parse endpoints for data & notification */
        if (USBD_HAL_IsHighSpeed() && pDescriptors->pHsConfiguration)
            pDesc = (USBConfigurationDescriptor*)pDescriptors->pHsConfiguration;
        else
            pDesc = (USBConfigurationDescriptor*)pDescriptors->pFsConfiguration;

        AUDDSpeakerPhone_ParseInterfaces(pAudf,
                                         (USBGenericDescriptor*)pDesc,
                                         pDesc->wTotalLength);
    }
}

/**
 * Invoked whenever the active setting of an interface is changed by the
 * host. Changes the status of the third LED accordingly.
 * \param interface Interface number.
 * \param setting Newly active setting.
 */
void AUDDSpeakerDriver_InterfaceSettingChangedHandler(uint8_t interface,
                                                      uint8_t setting)
{
    AUDDSpeakerDriver *pSpeakerd = &auddSpeakerDriver;
    AUDDSpeakerPhone  *pAudf     = &pSpeakerd->fun;

    if (setting == 0) {
        AUDDSpeakerPhone_CloseStream(pAudf, interface);
    }

    if (AUDDSpeakerDriver_StreamSettingChanged)
        AUDDSpeakerDriver_StreamSettingChanged(setting);
}

/**
 * Handles audio-specific USB requests sent by the host, and forwards
 * standard ones to the USB device driver.
 * \param request Pointer to a USBGenericRequest instance.
 */
void AUDDSpeakerDriver_RequestHandler(const USBGenericRequest *request)
{
    AUDDSpeakerDriver *pAudd = &auddSpeakerDriver;
    AUDDSpeakerPhone *pAudf  = &pAudd->fun;
    USBDDriver *pUsbd = pAudf->pUsbd;

    TRACE_INFO_WP("NewReq ");

    /* Handle Audio Class requests */
    if (AUDDSpeakerPhone_RequestHandler(pAudf, request) == USBRC_SUCCESS) {
        return;
    }

    /* Handle STD requests */
    if (USBGenericRequest_GetType(request) == USBGenericRequest_STANDARD) {

        USBDDriver_RequestHandler(pUsbd, request);
    }
    /* Unsupported request */
    else {

        TRACE_WARNING(
          "AUDDSpeakerDriver_RequestHandler: Unsupported request (%d,%x)\n\r",
          USBGenericRequest_GetType(request),
          USBGenericRequest_GetRequest(request));
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
uint8_t AUDDSpeakerDriver_Read(void *buffer,
                               uint32_t length,
                               TransferCallback callback,
                               void *argument)
{
    AUDDSpeakerDriver *pAudd = &auddSpeakerDriver;
    AUDDSpeakerPhone *pAudf  = &pAudd->fun;
    return USBD_Read(pAudf->pSpeaker->bEndpointOut,
                     buffer, length,
                     callback, argument);
}

/**@}*/
