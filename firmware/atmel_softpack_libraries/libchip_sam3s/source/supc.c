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

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include "chip.h"

#include <assert.h>

//------------------------------------------------------------------------------
//         Local definitions
//------------------------------------------------------------------------------

/// Key value for the SUPC_MR register.
#define SUPC_KEY            ((uint32_t) (0xA5 << 24))

//------------------------------------------------------------------------------
//         Global functions
//------------------------------------------------------------------------------

#if !defined(__ICCARM__)
__attribute__ ((section (".ramfunc"))) // GCC
#endif
//------------------------------------------------------------------------------
/// Enables the flash power supply with the given wake-up setting.
/// \param time  Wake-up time.
//------------------------------------------------------------------------------
void SUPC_EnableFlash( Supc* pSupc, uint32_t dwTime )
{
    pSupc->SUPC_FWUTR = dwTime ;
    pSupc->SUPC_MR = SUPC_KEY | pSupc->SUPC_MR | AT91C_SUPC_FLASHON ;

    while ((pSupc->SUPC_SR & AT91C_SUPC_FLASHS) != AT91C_SUPC_FLASHS) ;
}

#if !defined(__ICCARM__)
__attribute__ ((section (".ramfunc"))) // GCC
#endif
//------------------------------------------------------------------------------
/// Disables the flash power supply.
//------------------------------------------------------------------------------
void SUPC_DisableFlash( Supc* pSupc )
{
    pSupc->SUPC_MR = SUPC_KEY | (pSupc->SUPC_MR & ~AT91C_SUPC_FLASHON) ;

    while ((pSupc->SUPC_SR & AT91C_SUPC_FLASHS) == AT91C_SUPC_FLASHS) ;
}

//------------------------------------------------------------------------------
/// Sets the voltage regulator output voltage.
/// \param voltage  Voltage to set.
//------------------------------------------------------------------------------
void SUPC_SetVoltageOutput( Supc* pSupc, uint32_t dwVoltage )
{
    assert( (voltage & ~AT91C_SUPC_VRVDD) == 0 ) ;

    pSupc->SUPC_MR = SUPC_KEY | (pSupc->SUPC_MR & ~AT91C_SUPC_VRVDD) | dwVoltage ;
}

//------------------------------------------------------------------------------
/// Puts the voltage regulator in deep mode.
//------------------------------------------------------------------------------
void SUPC_EnableDeepMode( Supc* pSupc )
{
    pSupc->SUPC_MR = SUPC_KEY | pSupc->SUPC_MR | AT91C_SUPC_VRDEEP ;
}

//------------------------------------------------------------------------------
/// Puts the voltage regulator in normal mode.
//------------------------------------------------------------------------------
void SUPC_DisableDeepMode( Supc* pSupc )
{
    pSupc->SUPC_MR = SUPC_KEY | (pSupc->SUPC_MR & ~AT91C_SUPC_VRDEEP) ;
}

//-----------------------------------------------------------------------------
/// Enables the backup SRAM power supply, so its data is saved while the device
/// is in backup mode.
//-----------------------------------------------------------------------------
void SUPC_EnableSram( Supc* pSupc )
{
    pSupc->SUPC_MR = SUPC_KEY | pSupc->SUPC_MR | AT91C_SUPC_SRAMON ;
}

//-----------------------------------------------------------------------------
/// Disables the backup SRAM power supply.
//-----------------------------------------------------------------------------
void SUPC_DisableSram( Supc* pSupc )
{
    pSupc->SUPC_MR = SUPC_KEY | (pSupc->SUPC_MR & ~AT91C_SUPC_SRAMON) ;
}

//-----------------------------------------------------------------------------
/// Enables the RTC power supply.
//-----------------------------------------------------------------------------
void SUPC_EnableRtc( Supc* pSupc )
{
    pSupc->SUPC_MR = SUPC_KEY | pSupc->SUPC_MR | AT91C_SUPC_RTCON ;

    while ((pSupc->SUPC_SR & AT91C_SUPC_RTS) != AT91C_SUPC_RTS) ;
}

//-----------------------------------------------------------------------------
/// Disables the RTC power supply.
//-----------------------------------------------------------------------------
void SUPC_DisableRtc( Supc* pSupc )
{
    pSupc->SUPC_MR = SUPC_KEY | (pSupc->SUPC_MR & ~AT91C_SUPC_RTCON) ;

    while ((pSupc->SUPC_SR & AT91C_SUPC_RTS) == AT91C_SUPC_RTS);
}

//-----------------------------------------------------------------------------
/// Sets the BOD sampling mode (or disables it).
/// \param mode  BOD sampling mode.
//-----------------------------------------------------------------------------
void SUPC_SetBodSampling( Supc* pSupc, uint32_t dwMode )
{
    assert( (dwMode & ~AT91C_SUPC_BODSMPL) == 0 ) ;
    
    pSupc->SUPC_BOMR &= ~AT91C_SUPC_BODSMPL;
    pSupc->SUPC_BOMR |= dwMode ;
}

//------------------------------------------------------------------------------
/// Disables the voltage regulator, which makes the device enter backup mode.
//------------------------------------------------------------------------------
void SUPC_DisableVoltageRegulator( Supc* pSupc )
{
    pSupc->SUPC_CR = SUPC_KEY | AT91C_SUPC_VROFF ;

    while ( 1 ) ;
}

//------------------------------------------------------------------------------
/// Shuts the device down so it enters Off mode.
//------------------------------------------------------------------------------
void SUPC_Shutdown( Supc* pSupc )
{
    pSupc->SUPC_CR = SUPC_KEY | AT91C_SUPC_SHDW ;

    while (1);
}

//------------------------------------------------------------------------------
/// Sets the wake-up sources when in backup mode.
/// \param sources  Wake-up sources to enable.
//------------------------------------------------------------------------------
void SUPC_SetWakeUpSources( Supc* pSupc, uint32_t dwSources )
{
    assert( (dwSources & ~0x0000000B) == 0 ) ;
    
    pSupc->SUPC_WUMR &= ~0x0000000B;
    pSupc->SUPC_WUMR |= dwSources ;
}

//------------------------------------------------------------------------------
/// Sets the wake-up inputs when in backup mode.
/// \param inputs  Wake up inputs to enable.
//------------------------------------------------------------------------------
void SUPC_SetWakeUpInputs( Supc* pSupc, uint32_t dwInputs )
{
    assert( (dwInputs & ~0xFFFF) == 0 ) ;
    
    pSupc->SUPC_WUIR &= ~0xFFFF ;
    pSupc->SUPC_WUIR |= dwInputs ;
}

