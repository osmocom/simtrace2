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

/** \file
 *  Default callbacks implement for Audio Speaker Driver.
 */

/** \addtogroup usbd_audio_speaker
 *@{
 */

/*---------------------------------------------------------------------------
 *      Headers
 *---------------------------------------------------------------------------*/

/* These headers were introduced in C99
   by working group ISO/IEC JTC1/SC22/WG14. */
#include <stdint.h>

#include "AUDDSpeakerDriver.h"

/*------------------------------------------------------------------------------
 *         Default WEAK Callbacks
 *------------------------------------------------------------------------------*/

/**
 *  Invoked when an audio channel get muted or unmuted. Mutes/unmutes the
 *  channel at the DAC level.
 *  \param channel  Channel number that changed.
 *  \param muted    Indicates the new mute status of the channel.
 */
WEAK void AUDDSpeakerDriver_MuteChanged(uint8_t channel,uint8_t muted)
{
    /* Do nothing */
}

/**
 *  Invoked when an audio streaming interface setting changed. Actually control
 *  streaming rate.
 *  \param newSetting  New stream (interface) setting.
 */
WEAK void AUDDSpeakerDriver_StreamSettingChanged(uint8_t newSetting)
{
    /* Do nothing */
}

/**@}*/

