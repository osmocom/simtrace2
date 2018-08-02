/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2009, Atmel Corporation
 * Copyright (c) 2018, sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>
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
 * This file contains the default exception handlers.
 *
 * \note
 * The exception handler has weak aliases.
 * As they are weak aliases, any function with the same name will override
 * this definition.
 */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#define printf printf_sync
#include "chip.h"

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Default interrupt handler for not used irq.
 */
void IrqHandlerNotUsed( void )
{
    printf("NotUsed\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default NMI interrupt handler.
 */
WEAK void NMI_Handler( void )
{
    printf("NMI\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default HardFault interrupt handler.
 */
struct hardfault_args {
	unsigned long r0;
	unsigned long r1;
	unsigned long r2;
	unsigned long r3;
	unsigned long r12;
	unsigned long lr;
	unsigned long pc;
	unsigned long psr;
};

void hard_fault_handler_c(struct hardfault_args *args)
{
    printf("\r\nHardFault\r\n");
    printf("R0=%08x, R1=%08x, R2=%08x, R3=%08x, R12=%08x\r\n",
	   args->r0, args->r1, args->r2, args->r3, args->r12);
    printf("LR[R14]=%08x, PC[R15]=%08x, PSR=%08x\r\n",
	   args->lr, args->pc, args->psr);
    printf("BFAR=%08x, CFSR=%08x, HFSR=%08x\r\n",
	   SCB->BFAR, SCB->CFSR, SCB->HFSR);
    printf("DFSR=%08x, AFSR=%08x, SHCSR=%08x\r\n",
	   SCB->DFSR, SCB->CFSR, SCB->SHCSR);

    if (SCB->HFSR & 0x40000000)
	    printf("FORCED ");
    if (SCB->HFSR & 0x00000002)
	    printf("VECTTBL ");

    uint32_t ufsr = SCB->CFSR >> 16;
    if (ufsr & 0x0200)
	    printf("DIVBYZERO ");
    if (ufsr & 0x0100)
	    printf("UNALIGNED ");
    if (ufsr & 0x0008)
	    printf("NOCP ");
    if (ufsr & 0x0004)
	    printf("INVPC ");
    if (ufsr & 0x0002)
	    printf("INVSTATE ");
    if (ufsr & 0x0001)
	    printf("UNDEFINSTR ");

    uint32_t bfsr = (SCB->CFSR >> 8) & 0xff;
    if (bfsr & 0x80)
	    printf("BFARVALID ");
    if (bfsr & 0x10)
	    printf("STKERR ");
    if (bfsr & 0x08)
	    printf("UNSTKERR ");
    if (bfsr & 0x04)
	    printf("IMPRECISERR ");
    if (bfsr & 0x02)
	    printf("PRECISERR ");
    if (bfsr & 0x01)
	    printf("IBUSERR ");

    uint32_t mmfsr = (SCB->CFSR & 0xff);
    if (mmfsr & 0x80)
	    printf("MMARVALID ");
    if (mmfsr & 0x10)
	    printf("MSTKERR ");
    if (mmfsr & 0x08)
	    printf("MUNSTKERR ");
    if (mmfsr & 0x02)
	    printf("DACCVIOL ");
    if (mmfsr & 0x01)
	    printf("IACCVIOL ");

    while ( 1 ) ;
}

__attribute__((naked))
WEAK void HardFault_Handler( void )
{
	__asm volatile(
		".syntax unified	\n"
		" tst lr, #4		\n"
		" ite eq		\n"
		" mrseq r0, msp		\n"
		" mrsne r0, psp		\n"
		//" ldr r1, [r0, #24] 	\n"
		" b hard_fault_handler_c\n"
		".syntax divided	\n");
}

/**
 * \brief Default MemManage interrupt handler.
 */
WEAK void MemManage_Handler( void )
{
    printf("MemManage\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default BusFault interrupt handler.
 */
WEAK void BusFault_Handler( void )
{
    printf("BusFault\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default UsageFault interrupt handler.
 */
WEAK void UsageFault_Handler( void )
{
    printf("UsageFault\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default SVC interrupt handler.
 */
WEAK void SVC_Handler( void )
{
    printf("SVC\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default DebugMon interrupt handler.
 */
WEAK void DebugMon_Handler( void )
{
    printf("DebugMon\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default PendSV interrupt handler.
 */
WEAK void PendSV_Handler( void )
{
    printf("PendSV\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default SysTick interrupt handler.
 */
WEAK void SysTick_Handler( void )
{
    printf("SysTick\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for Supply Controller.
 */
WEAK void SUPC_IrqHandler( void )
{
    printf("SUPC\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for Reset Controller.
 */
WEAK void RSTC_IrqHandler( void )
{
    printf("RSTC\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for Real Time Clock.
 */
WEAK void RTC_IrqHandler( void )
{
    printf("RTC\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for Real Time Timer.
 */
WEAK void RTT_IrqHandler( void )
{
    printf("RTT\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for Watchdog Timer.
 */
WEAK void WDT_IrqHandler( void )
{
    printf("WDT\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for PMC.
 */
WEAK void PMC_IrqHandler( void )
{
    printf("PMC\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for EEFC.
 */
WEAK void EEFC_IrqHandler( void )
{
    printf("EEFC\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for UART0.
 */
WEAK void UART0_IrqHandler( void )
{
    printf("UART0\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for UART1.
 */
WEAK void UART1_IrqHandler( void )
{
    printf("UART1\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for SMC.
 */
WEAK void SMC_IrqHandler( void )
{
    printf("SMC\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for PIOA Controller.
 */
WEAK void PIOA_IrqHandler( void )
{
    printf("PIOA\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for PIOB Controller.
 */
WEAK void PIOB_IrqHandler( void )
{
    printf("PIOB\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for PIOC Controller.
 */
WEAK void PIOC_IrqHandler( void )
{
    printf("PIOC\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for USART0.
 */
WEAK void USART0_IrqHandler( void )
{
    printf("USART0\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for USART1.
 */
WEAK void USART1_IrqHandler( void )
{
    printf("USART1\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for MCI.
 */
WEAK void MCI_IrqHandler( void )
{
    printf("MCI\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for TWI0.
 */
WEAK void TWI0_IrqHandler( void )
{
    printf("TWI0\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for TWI1.
 */
WEAK void TWI1_IrqHandler( void )
{
    printf("TWI1\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for SPI.
 */
WEAK void SPI_IrqHandler( void )
{
    printf("SPI\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for SSC.
 */
WEAK void SSC_IrqHandler( void )
{
    printf("SSC\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for TC0.
 */
WEAK void TC0_IrqHandler( void )
{
    printf("TC0\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for TC1.
 */
WEAK void TC1_IrqHandler( void )
{
    printf("TC1\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default interrupt handler for TC2.
 */
WEAK void TC2_IrqHandler( void )
{
    printf("TC2\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default SUPC interrupt handler for TC3.
 */
WEAK void TC3_IrqHandler( void )
{
    printf("TC3\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default SUPC interrupt handler for TC4.
 */
WEAK void TC4_IrqHandler( void )
{
    printf("TC4\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default SUPC interrupt handler for TC5.
 */
WEAK void TC5_IrqHandler( void )
{
    printf("TC5\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default SUPC interrupt handler for ADC.
 */
WEAK void ADC_IrqHandler( void )
{
    printf("ADC\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default SUPC interrupt handler for DAC.
 */
WEAK void DAC_IrqHandler( void )
{
    printf("DAC\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default SUPC interrupt handler for PWM.
 */
WEAK void PWM_IrqHandler( void )
{
    printf("PWM\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default SUPC interrupt handler for CRCCU.
 */
WEAK void CRCCU_IrqHandler( void )
{
    printf("CRCCU\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default SUPC interrupt handler for ACC.
 */
WEAK void ACC_IrqHandler( void )
{
    printf("ACC\r\n");
    while ( 1 ) ;
}

/**
 * \brief Default SUPC interrupt handler for USBD.
 */
WEAK void USBD_IrqHandler( void )
{
    printf("USBD\r\n");
    while ( 1 ) ;
}
