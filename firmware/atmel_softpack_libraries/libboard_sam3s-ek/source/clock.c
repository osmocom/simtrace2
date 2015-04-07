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

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "board.h"

/*----------------------------------------------------------------------------
 *        Types
 *----------------------------------------------------------------------------*/

/*
 * \brief Describes a possible clock configuration (processor clock & master clock),
 * including the necessary register values.
 */
typedef struct _ClockConfiguration
{

    /** Processor clock frequency (in MHz). */
    uint16_t pck;
    /** Master clock frequency (in MHz). */
    uint16_t mck;
    /** CKGR_PLL reqister value. */
    uint32_t pllr;
    /** PMC_MCKR register value. */
    uint32_t mckr;
} ClockConfiguration ;

/*----------------------------------------------------------------------------
 *        Local variables
 *----------------------------------------------------------------------------*/

/* Clock configurations for the AT91SAM3S4-EK */
#define CKGR_MUL_SHIFT         16
#define CKGR_PLLCOUNT_SHIFT     8
#define CKGR_DIV_SHIFT          0

/* Clock configuration for the AT91SAM3S */
static const ClockConfiguration clockConfigurations[] = {

    /* PCK = 24 MHz, MCK = 24 MHz
     * PCK = 12000000 * (7+1) / 2 / 2 = 24 MHz
     */
    {24, 24, (CKGR_PLLAR_STUCKTO1 | (7 << CKGR_MUL_SHIFT) \
        | (0x3f << CKGR_PLLCOUNT_SHIFT) \
        | (2 << CKGR_DIV_SHIFT)),
        ( PMC_MCKR_PRES_CLK_2 | PMC_MCKR_CSS_PLLA_CLK)},
    /* PCK = 48 MHz, MCK = 48 MHz
     * PCK = 12000000 * (7+1) / 1 / 2 = 48 MHz
     */
    {48, 48, (CKGR_PLLAR_STUCKTO1 | (7 << CKGR_MUL_SHIFT) \
        | (0x3f << CKGR_PLLCOUNT_SHIFT) \
        | (1 << CKGR_DIV_SHIFT)),
        ( PMC_MCKR_PRES_CLK_2 | PMC_MCKR_CSS_PLLA_CLK)},
    /* PCK = 64 MHz, MCK = 64 MHz
     * PCK = 12000000 * (15+1) / 3 / 1 = 64 MHz
     */
    {64, 64, (CKGR_PLLAR_STUCKTO1 | (15 << CKGR_MUL_SHIFT) \
        | (0x3f << CKGR_PLLCOUNT_SHIFT) \
        | (3 << CKGR_DIV_SHIFT)),
        ( PMC_MCKR_PRES_CLK | PMC_MCKR_CSS_PLLA_CLK)}
};

/* Number of available clock configurations */
#define NB_CLOCK_CONFIGURATION (sizeof(clockConfigurations)/sizeof(clockConfigurations[0]))

/* Current clock configuration */
uint32_t currentConfig = 0; /* 0 have to be the default configuration */

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Sets the specified clock configuration.
 *
 * \param configuration  Index of the configuration to set.
 */
void CLOCK_SetConfig(uint8_t configuration)
{
    TRACE_DEBUG("Setting clock configuration #%d ... ", configuration);
    currentConfig = configuration;

    /* Switch to main oscillator in two operations */
    PMC->PMC_MCKR = (PMC->PMC_MCKR & (uint32_t)~PMC_MCKR_CSS_Msk) | PMC_MCKR_CSS_MAIN_CLK;
    while ((PMC->PMC_SR & PMC_SR_MCKRDY) == 0);

    /* Configure PLL */
    PMC->CKGR_PLLAR = clockConfigurations[configuration].pllr;
    while ((PMC->PMC_SR & PMC_SR_LOCKA) == 0);

    /* Configure master clock in two operations */
    PMC->PMC_MCKR = (clockConfigurations[configuration].mckr & (uint32_t)~PMC_MCKR_CSS_Msk) | PMC_MCKR_CSS_MAIN_CLK;
    while ((PMC->PMC_SR & PMC_SR_MCKRDY) == 0);
    PMC->PMC_MCKR = clockConfigurations[configuration].mckr;
    while ((PMC->PMC_SR & PMC_SR_MCKRDY) == 0);

    /* DBGU reconfiguration */
    UART_Configure(115200, clockConfigurations[configuration].mck*1000000);
    TRACE_DEBUG("done.\n\r");
}

/**
 * \brief Display the user menu on the DBGU.
 */
void CLOCK_DisplayMenu(void)
{
    uint32_t i;

    printf("\n\rMenu Clock configuration:\n\r");
    for (i = 0; i < NB_CLOCK_CONFIGURATION; i++) {

        printf("  %u: Set PCK = %3u MHz, MCK = %3u MHz   %s\n\r",
               (unsigned int)i,
               (unsigned int)clockConfigurations[i].pck,
               (unsigned int)clockConfigurations[i].mck,
               (currentConfig==i)?"(curr)":"");
    }
}

/**
 * \brief Get the current MCK
 */
uint16_t CLOCK_GetCurrMCK(void)
{
    return clockConfigurations[currentConfig].mck;
}

/**
 * \brief Get the current PCK
 */
uint16_t CLOCK_GetCurrPCK(void)
{
    return clockConfigurations[currentConfig].pck;
}

/**
 * \brief Change clock configuration.
 */
void CLOCK_UserChangeConfig(void)
{
    uint8_t key = 0;

    while (1)
    {
        CLOCK_DisplayMenu();
        key = UART_GetChar();

        if ((key >= '0') && (key <= ('0' + NB_CLOCK_CONFIGURATION - 1)))
        {
            CLOCK_SetConfig(key - '0');
            break;
        }
    }
}

