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

#ifndef _SUPC_
#define _SUPC_

#include "chip.h"

#include <stdint.h>

//------------------------------------------------------------------------------
//         Global functions
//------------------------------------------------------------------------------

extern
#ifdef __ICCARM__
__ramfunc /* IAR */
#endif
void SUPC_EnableFlash( Supc* pSupc, uint32_t dwTime ) ;

extern
#ifdef __ICCARM__
__ramfunc /* IAR */
#endif
void SUPC_DisableFlash( Supc* pSupc ) ;

extern void SUPC_SetVoltageOutput( Supc* pSupc, uint32_t dwVoltage ) ;

extern void SUPC_EnableDeepMode( Supc* pSupc ) ;

extern void SUPC_EnableSram( Supc* pSupc ) ;

extern void SUPC_DisableSram( Supc* pSupc ) ;

extern void SUPC_EnableRtc( Supc* pSupc ) ;

extern void SUPC_DisableRtc( Supc* pSupc ) ;

extern void SUPC_SetBodSampling( Supc* pSupc, uint32_t dwMode ) ;

extern void SUPC_DisableDeepMode( Supc* pSupc ) ;

extern void SUPC_DisableVoltageRegulator( Supc* pSupc ) ;

extern void SUPC_Shutdown( Supc* pSupc ) ;

extern void SUPC_SetWakeUpSources( Supc* pSupc, uint32_t dwSources ) ;

extern void SUPC_SetWakeUpInputs( Supc* pSupc, uint32_t dwInputs ) ;

#endif /* #ifndef _SUPC_ */

