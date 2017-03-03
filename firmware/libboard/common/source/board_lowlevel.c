/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2009, Atmel Corporation
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
 * Provides the low-level initialization function that called on chip startup.
 */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "board.h"

/*----------------------------------------------------------------------------
 *        Local definitions
 *----------------------------------------------------------------------------*/

#define BOARD_OSCOUNT   (CKGR_MOR_MOSCXTST(0x8))
#define BOARD_MCKR      (PMC_MCKR_PRES_CLK | PMC_MCKR_CSS_PLLA_CLK)

#if (BOARD_MCK == 48000000)
#if (BOARD_MAINOSC == 18432000)
/* Clock settings at 48MHz  for 18 MHz crystal */
#define BOARD_PLLAR     (CKGR_PLLAR_STUCKTO1 \
                       | CKGR_PLLAR_MULA(13-1) \
                       | CKGR_PLLAR_PLLACOUNT(0x1) \
                       | CKGR_PLLAR_DIVA(5))
#elif (BOARD_MAINOSC == 12000000)
/* QMod has 12 MHz clock, so multply by 8 (96 MHz) and divide by 2 */
#define BOARD_PLLAR     (CKGR_PLLAR_STUCKTO1 \
                       | CKGR_PLLAR_MULA(8-1) \
                       | CKGR_PLLAR_PLLACOUNT(0x1) \
                       | CKGR_PLLAR_DIVA(2))
#else
#error "Please define PLLA config for your MAINOSC frequency"
#endif /* MAINOSC */
#elif (BOARD_MCK == 64000000)
#if (BOARD_MAINOSC == 18432000)
/* Clock settings at 64MHz  for 18 MHz crystal: 64.512 MHz */
#define BOARD_PLLAR     (CKGR_PLLAR_STUCKTO1 \
                       | CKGR_PLLAR_MULA(7-1) \
                       | CKGR_PLLAR_PLLACOUNT(0x1) \
                       | CKGR_PLLAR_DIVA(2))
#elif (BOARD_MAINOSC == 12000000)
/* QMod has 12 MHz clock, so multply by 10 / div by 2: 60 MHz */
#define BOARD_PLLAR     (CKGR_PLLAR_STUCKTO1 \
                       | CKGR_PLLAR_MULA(10-1) \
                       | CKGR_PLLAR_PLLACOUNT(0x1) \
                       | CKGR_PLLAR_DIVA(2))
#error "Please define PLLA config for your MAINOSC frequency"
#endif /* MAINOSC */
#else
    #error "No PLL settings for current BOARD_MCK."
#endif

#if (BOARD_MAINOSC == 12000000)
#define PLLB_CFG (CKGR_PLLBR_DIVB(2)|CKGR_PLLBR_MULB(8-1)|CKGR_PLLBR_PLLBCOUNT_Msk)
#elif (BOARD_MAINOSC == 18432000)
#define PLLB_CFG (CKGR_PLLBR_DIVB(5)|CKGR_PLLBR_MULB(13-1)|CKGR_PLLBR_PLLBCOUNT_Msk)
#else
#error "Please configure PLLB for your MAINOSC freq"
#endif

/* Define clock timeout */
#define CLOCK_TIMEOUT    0xFFFFFFFF

/**
 * \brief Configure 48MHz Clock for USB
 */
static void _ConfigureUsbClock(void)
{
	/* Enable PLLB for USB */
	PMC->CKGR_PLLBR = PLLB_CFG;
	while ((PMC->PMC_SR & PMC_SR_LOCKB) == 0) ;

	/* USB Clock uses PLLB */
	PMC->PMC_USB = PMC_USB_USBDIV(0)	/* /1 (no divider)  */
			    | PMC_USB_USBS;	/* PLLB */
}

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Performs the low-level initialization of the chip.
 * This includes EFC and master clock configuration.
 * It also enable a low level on the pin NRST triggers a user reset.
 */
extern WEAK void LowLevelInit( void )
{
    uint32_t timeout = 0;

    /* Configure the Supply Monitor to reset the CPU in case VDDIO is
     * lower than 3.0V.  As we run the board on 3.3V, any lower voltage
     * might be some kind of leakage that creeps in some way, but is not
     * the "official" power supply */
    SUPC->SUPC_SMMR = SUPC_SMMR_SMTH_3_0V | SUPC_SMMR_SMSMPL_CSM |
	    	      SUPC_SMMR_SMRSTEN_ENABLE;

    /* enable both LED and green LED */
    PIOA->PIO_PER |= LED_RED | LED_GREEN;
    PIOA->PIO_OER |= LED_RED | LED_GREEN;
    PIOA->PIO_CODR |= LED_RED | LED_GREEN;

    /* Set 3 FWS for Embedded Flash Access */
    EFC->EEFC_FMR = EEFC_FMR_FWS(3);

    /* Select external slow clock */
/*    if ((SUPC->SUPC_SR & SUPC_SR_OSCSEL) != SUPC_SR_OSCSEL_CRYST)
    {
        SUPC->SUPC_CR = (uint32_t)(SUPC_CR_XTALSEL_CRYSTAL_SEL | SUPC_CR_KEY(0xA5));
        timeout = 0;
        while (!(SUPC->SUPC_SR & SUPC_SR_OSCSEL_CRYST) );
    }
*/

#ifndef qmod
    /* Initialize main oscillator */
    if ( !(PMC->CKGR_MOR & CKGR_MOR_MOSCSEL) )
    {
        PMC->CKGR_MOR = CKGR_MOR_KEY(0x37) | BOARD_OSCOUNT | CKGR_MOR_MOSCRCEN | CKGR_MOR_MOSCXTEN;
        timeout = 0;
        while (!(PMC->PMC_SR & PMC_SR_MOSCXTS) && (timeout++ < CLOCK_TIMEOUT));
    }

    /* Switch to 3-20MHz Xtal oscillator */
    PIOB->PIO_PDR = (1 << 8) | (1 << 9);
    PIOB->PIO_PUDR = (1 << 8) | (1 << 9);
    PIOB->PIO_PPDDR = (1 << 8) | (1 << 9);
    PMC->CKGR_MOR = CKGR_MOR_KEY(0x37) | BOARD_OSCOUNT | CKGR_MOR_MOSCRCEN | CKGR_MOR_MOSCXTEN | CKGR_MOR_MOSCSEL;
    /* wait for Main XTAL oscillator stabilization */
    timeout = 0;
    while (!(PMC->PMC_SR & PMC_SR_MOSCSELS) && (timeout++ < CLOCK_TIMEOUT));
#else
    /* QMOD has external 12MHz clock source */
    PIOB->PIO_PDR = (1 << 9);
    PIOB->PIO_PUDR = (1 << 9);
    PIOB->PIO_PPDDR = (1 << 9);
    PMC->CKGR_MOR = CKGR_MOR_KEY(0x37) | CKGR_MOR_MOSCRCEN | CKGR_MOR_MOSCXTBY| CKGR_MOR_MOSCSEL;
#endif

    /* disable the red LED after main clock initialization */
    PIOA->PIO_SODR = LED_RED;

    /* "switch" to main clock as master clock source (should already be the case */
    PMC->PMC_MCKR = (PMC->PMC_MCKR & ~(uint32_t)PMC_MCKR_CSS_Msk) | PMC_MCKR_CSS_MAIN_CLK;
    /* wait for master clock to be ready */
    for ( timeout = 0; !(PMC->PMC_SR & PMC_SR_MCKRDY) && (timeout++ < CLOCK_TIMEOUT) ; );

    /* Initialize PLLA */
    PMC->CKGR_PLLAR = BOARD_PLLAR;
    /* Wait for PLLA to lock */
    timeout = 0;
    while (!(PMC->PMC_SR & PMC_SR_LOCKA) && (timeout++ < CLOCK_TIMEOUT));

    /* Switch to main clock (again ?!?) */
    PMC->PMC_MCKR = (BOARD_MCKR & ~PMC_MCKR_CSS_Msk) | PMC_MCKR_CSS_MAIN_CLK;
    /* wait for master clock to be ready */
    for ( timeout = 0; !(PMC->PMC_SR & PMC_SR_MCKRDY) && (timeout++ < CLOCK_TIMEOUT) ; );

    /* switch to PLLA as master clock source */
    PMC->PMC_MCKR = BOARD_MCKR ;
    /* wait for master clock to be ready */
    for ( timeout = 0; !(PMC->PMC_SR & PMC_SR_MCKRDY) && (timeout++ < CLOCK_TIMEOUT) ; );

    /* Configure SysTick for 1ms */
    SysTick_Config(BOARD_MCK/1000);

    _ConfigureUsbClock();
}

/* SysTick based delay function */

volatile uint32_t jiffies;

/* Interrupt handler for SysTick interrupt */
void SysTick_Handler(void)
{
	jiffies++;
}

void mdelay(unsigned int msecs)
{
	uint32_t jiffies_start = jiffies;
	do {
	} while ((jiffies - jiffies_start) < msecs);
}
