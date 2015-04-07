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
 * Implementation of memories configuration on board.
 *
 */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/
#include "board.h"

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/

/**
 * \brief Configures the EBI for NandFlash access.
 */
extern void BOARD_ConfigureNandFlash( Smc* pSmc )
{
    /* Enable peripheral clock */
    PMC_EnablePeripheral( ID_SMC ) ;

    /* NCS0 is assigned to a NAND Flash (NANDOE and NANWE used for NCS0) */
    MATRIX->CCFG_SMCNFCS = CCFG_SMCNFCS_SMC_NFCS0;

    pSmc->SMC_CS_NUMBER[0].SMC_SETUP = SMC_SETUP_NWE_SETUP(0)
                                     | SMC_SETUP_NCS_WR_SETUP(1)
                                     | SMC_SETUP_NRD_SETUP(0)
                                     | SMC_SETUP_NCS_RD_SETUP(1);

    pSmc->SMC_CS_NUMBER[0].SMC_PULSE = SMC_PULSE_NWE_PULSE(2)
                                     | SMC_PULSE_NCS_WR_PULSE(3)
                                     | SMC_PULSE_NRD_PULSE(4)
                                     | SMC_PULSE_NCS_RD_PULSE(4);

    pSmc->SMC_CS_NUMBER[0].SMC_CYCLE = SMC_CYCLE_NWE_CYCLE(4)
                                     | SMC_CYCLE_NRD_CYCLE(7);

    pSmc->SMC_CS_NUMBER[0].SMC_MODE = SMC_MODE_READ_MODE
                                    | SMC_MODE_WRITE_MODE
                                    | SMC_MODE_DBW_8_BIT;
}

/**
 * \brief Configures the EBI for %NorFlash access.
 */
extern void BOARD_ConfigureNorFlash( Smc* pSmc )
{
    /* Enable peripheral clock */
    PMC_EnablePeripheral( ID_SMC ) ;

    /* Configure SMC, NCS3 is assigned to a norflash */
    pSmc->SMC_CS_NUMBER[3].SMC_SETUP = SMC_SETUP_NWE_SETUP(2)
                                     | SMC_SETUP_NCS_WR_SETUP(0)
                                     | SMC_SETUP_NRD_SETUP(0)
                                     | SMC_SETUP_NCS_RD_SETUP(0);

    pSmc->SMC_CS_NUMBER[3].SMC_PULSE = SMC_PULSE_NWE_PULSE(6)
                                     | SMC_PULSE_NCS_WR_PULSE(0xA)
                                     | SMC_PULSE_NRD_PULSE(0xA)
                                     | SMC_PULSE_NCS_RD_PULSE(0xA);

    pSmc->SMC_CS_NUMBER[3].SMC_CYCLE = SMC_CYCLE_NWE_CYCLE(0xA)
                                     | SMC_CYCLE_NRD_CYCLE(0xA);

    pSmc->SMC_CS_NUMBER[3].SMC_MODE  = SMC_MODE_READ_MODE
                                     | SMC_MODE_WRITE_MODE
                                     | SMC_MODE_DBW_8_BIT
                                     | SMC_MODE_EXNW_MODE_DISABLED
                                     | SMC_MODE_TDF_CYCLES(0x1);
}

/**
 * \brief An accurate one-to-one comparison is necessary between PSRAM and SMC waveforms for
 *   a complete SMC configuration.
 *  \note The system is running at 48 MHz for the EBI Bus.
 *        Please refer to the "AC Characteristics" section of the customer product datasheet.
 */
extern void BOARD_ConfigurePSRAM( Smc* pSmc )
{
    uint32_t dwTmp ;

    /* Enable peripheral clock */
    PMC_EnablePeripheral( ID_SMC ) ;

    /* Configure SMC, NCS1 is assigned to a external PSRAM */
    /**
     * PSRAM IS66WV51216BLL
     * 55 ns Access time
     * tdoe = 25 ns max
     * SMC1 (timing SAM3S read mode SMC) = 21 ns of setup
     * 21 + 55 = 76 ns => at least 5 cycles at 64 MHz
     * Write pulse width minimum = 45 ns (PSRAM)
     */
    pSmc->SMC_CS_NUMBER[1].SMC_SETUP = SMC_SETUP_NWE_SETUP( 1 )
                                     | SMC_SETUP_NCS_WR_SETUP( 0 )
                                     | SMC_SETUP_NRD_SETUP( 2 )
                                     | SMC_SETUP_NCS_RD_SETUP( 0 ) ;

    pSmc->SMC_CS_NUMBER[1].SMC_PULSE = SMC_PULSE_NWE_PULSE( 3 )
                                     | SMC_PULSE_NCS_WR_PULSE( 4 )
                                     | SMC_PULSE_NRD_PULSE( 3 )
                                     | SMC_PULSE_NCS_RD_PULSE( 5 ) ;

    /* NWE_CYCLE:     The total duration of the write cycle.
                      NWE_CYCLE = NWE_SETUP + NWE_PULSE + NWE_HOLD
                      = NCS_WR_SETUP + NCS_WR_PULSE + NCS_WR_HOLD
                      (tWC) Write Cycle Time min. 70ns
       NRD_CYCLE:     The total duration of the read cycle.
                      NRD_CYCLE = NRD_SETUP + NRD_PULSE + NRD_HOLD
                      = NCS_RD_SETUP + NCS_RD_PULSE + NCS_RD_HOLD
                      (tRC) Read Cycle Time min. 70ns. */
    pSmc->SMC_CS_NUMBER[1].SMC_CYCLE = SMC_CYCLE_NWE_CYCLE( 4 )
                                     | SMC_CYCLE_NRD_CYCLE( 5 ) ;

    dwTmp = SMC->SMC_CS_NUMBER[0].SMC_MODE & (uint32_t)(~(SMC_MODE_DBW_Msk)) ;
    pSmc->SMC_CS_NUMBER[1].SMC_MODE  = dwTmp
                                     | SMC_MODE_READ_MODE
                                     | SMC_MODE_WRITE_MODE
                                     | SMC_MODE_DBW_8_BIT ;
}

