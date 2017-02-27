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
 * \section Purpose
 *
 * ISO 7816 driver
 *
 * \section Usage
 *
 * Explanation on the usage of the code made available through the header file.
 */

/*------------------------------------------------------------------------------
 *         Headers
 *------------------------------------------------------------------------------*/

#include "board.h"

/*------------------------------------------------------------------------------
 *         Definitions
 *------------------------------------------------------------------------------*/
/** Case for APDU commands*/
#define CASE1  1
#define CASE2  2
#define CASE3  3

/** Flip flop for send and receive char */
#define USART_SEND 0
#define USART_RCV  1

/*-----------------------------------------------------------------------------
 *          Internal variables
 *-----------------------------------------------------------------------------*/
/** Pin reset master card */
static Pin *st_pinIso7816RstMC;

struct Usart_info usart_sim = {.base = USART_SIM, .id = ID_USART_SIM, .state = USART_RCV};

/*----------------------------------------------------------------------------
 *          Internal functions
 *----------------------------------------------------------------------------*/

/**
 * Get a character from ISO7816
 * \param pCharToReceive Pointer for store the received char
 * \return 0: if timeout else status of US_CSR
 */
uint32_t ISO7816_GetChar( uint8_t *pCharToReceive, Usart_info *usart)
{
    uint32_t status;
    uint32_t timeout=0;

    Usart *us_base = usart->base;
    uint32_t us_id = usart->id;

    if( usart->state == USART_SEND ) {
        while((us_base->US_CSR & US_CSR_TXEMPTY) == 0) {}
        us_base->US_CR = US_CR_RSTSTA | US_CR_RSTIT | US_CR_RSTNACK;
        usart->state = USART_RCV;
    }

    /* Wait USART ready for reception */
    while( ((us_base->US_CSR & US_CSR_RXRDY) == 0) ) {
        if(timeout++ > 12000 * (BOARD_MCK/1000000)) {
            TRACE_WARNING("TimeOut\n\r");
            return( 0 );
        }
    }

    /* At least one complete character has been received and US_RHR has not yet been read. */

    /* Get a char */
    *pCharToReceive = ((us_base->US_RHR) & 0xFF);

    status = (us_base->US_CSR&(US_CSR_OVRE|US_CSR_FRAME|
                                      US_CSR_PARE|US_CSR_TIMEOUT|US_CSR_NACK|
                                      (1<<10)));

    if (status != 0 ) {
        TRACE_DEBUG("R:0x%" PRIX32 "\n\r", status); 
        TRACE_DEBUG("R:0x%" PRIX32 "\n\r", us_base->US_CSR);
        TRACE_DEBUG("Nb:0x%" PRIX32 "\n\r", us_base->US_NER );
        us_base->US_CR = US_CR_RSTSTA;
    }

    /* Return status */
    return( status );
}


/**
 * Send a char to ISO7816
 * \param CharToSend char to be send
 * \return status of US_CSR
 */
uint32_t ISO7816_SendChar( uint8_t CharToSend, Usart_info *usart )
{
    uint32_t status;

    Usart *us_base = usart->base;
    uint32_t us_id = usart->id;

    if( usart->state == USART_RCV ) {
        us_base->US_CR = US_CR_RSTSTA | US_CR_RSTIT | US_CR_RSTNACK;
        usart->state = USART_SEND;
    }

    /* Wait USART ready for transmit */
    int i = 0;
    while((us_base->US_CSR & (US_CSR_TXRDY)) == 0)  {
        i++;
        if (!(i%1000000)) {
            printf("s: %x ", us_base->US_CSR);
            printf("s: %x\r\n", us_base->US_RHR & 0xFF);
            us_base->US_CR = US_CR_RSTTX;
            us_base->US_CR = US_CR_RSTRX;
      }
    }
    /* There is no character in the US_THR */

    /* Transmit a char */
    us_base->US_THR = CharToSend;

    TRACE_ERROR("Sx%02X\r\n", CharToSend);

    status = (us_base->US_CSR&(US_CSR_OVRE|US_CSR_FRAME|
                                      US_CSR_PARE|US_CSR_TIMEOUT|US_CSR_NACK|
                                      (1<<10)));

    if (status != 0 ) {
        TRACE_INFO("******* status: 0x%" PRIX32 " (Overrun: %" PRIX32
                    ", NACK: %" PRIX32 ", Timeout: %" PRIX32 ", underrun: %" PRIX32 ")\n\r",
                    status, ((status & US_CSR_OVRE)>> 5), ((status & US_CSR_NACK) >> 13),
                    ((status & US_CSR_TIMEOUT) >> 8), ((status & (1 << 10)) >> 10));
        TRACE_INFO("E (USART CSR reg):0x%" PRIX32 "\n\r", us_base->US_CSR);
        TRACE_INFO("Nb (Number of errors):0x%" PRIX32 "\n\r", us_base->US_NER );
        us_base->US_CR = US_CR_RSTSTA;
    }

    /* Return status */
    return( status );
}


/**
 *  Iso 7816 ICC power on
 */
static void ISO7816_IccPowerOn( void )
{
    /* Set RESET Master Card */
    if (st_pinIso7816RstMC) {
        PIO_Set(st_pinIso7816RstMC);
    }
}

/*----------------------------------------------------------------------------
 *          Exported functions
 *----------------------------------------------------------------------------*/

/**
 *  Iso 7816 ICC power off
 */
void ISO7816_IccPowerOff( void )
{
    /* Clear RESET Master Card */
    if (st_pinIso7816RstMC) {
        PIO_Clear(st_pinIso7816RstMC);
    }
}

/**
 * Transfert Block      TPDU T=0
 * \param pAPDU         APDU buffer
 * \param pMessage      Message buffer
 * \param wLength       Block length
 * \param indexMsg      Message index
 * \return              0 on success, content of US_CSR otherwise
 */
uint32_t ISO7816_XfrBlockTPDU_T0(const uint8_t *pAPDU,
                                        uint8_t *pMessage,
                                        uint16_t wLength,
                                        uint16_t *retlen )
{
    uint16_t NeNc;
    uint16_t indexApdu = 4;
    uint16_t indexMsg = 0;
    uint8_t SW1 = 0;
    uint8_t procByte;
    uint8_t cmdCase;
    uint32_t status = 0;

    TRACE_INFO("pAPDU[0]=0x%X\n\r",pAPDU[0]);
    TRACE_INFO("pAPDU[1]=0x%X\n\r",pAPDU[1]);
    TRACE_INFO("pAPDU[2]=0x%X\n\r",pAPDU[2]);
    TRACE_INFO("pAPDU[3]=0x%X\n\r",pAPDU[3]);
    TRACE_INFO("pAPDU[4]=0x%X\n\r",pAPDU[4]);
    TRACE_INFO("pAPDU[5]=0x%X\n\r",pAPDU[5]);
    TRACE_INFO("wlength=%d\n\r",wLength);

    ISO7816_SendChar( pAPDU[0], &usart_sim ); /* CLA */
    ISO7816_SendChar( pAPDU[1], &usart_sim ); /* INS */
    ISO7816_SendChar( pAPDU[2], &usart_sim ); /* P1 */
    ISO7816_SendChar( pAPDU[3], &usart_sim ); /* P2 */
    ISO7816_SendChar( pAPDU[4], &usart_sim ); /* P3 */

    /* Handle the four structures of command APDU */
    indexApdu = 5;

    if( wLength == 4 ) {
        cmdCase = CASE1;
        NeNc = 0;
    }
    else if( wLength == 5) {
        cmdCase = CASE2;
        NeNc = pAPDU[4]; /* C5 */
        if (NeNc == 0) {
            NeNc = 256;
        }
    }
    else if( wLength == 6) {
        NeNc = pAPDU[4]; /* C5 */
        cmdCase = CASE3;
    }
    else if( wLength == 7) {
        NeNc = pAPDU[4]; /* C5 */
        if( NeNc == 0 ) {
            cmdCase = CASE2;
            NeNc = (pAPDU[5]<<8)+pAPDU[6];
        }
        else {
            cmdCase = CASE3;
        }
    }
    else {
        NeNc = pAPDU[4]; /* C5 */
        if( NeNc == 0 ) {
            cmdCase = CASE3;
            NeNc = (pAPDU[5]<<8)+pAPDU[6];
        }
        else {
            cmdCase = CASE3;
        }
    }

    TRACE_DEBUG("CASE=0x%X NeNc=0x%X\n\r", cmdCase, NeNc);

    /* Handle Procedure Bytes */
    do {
        status = ISO7816_GetChar(&procByte, &usart_sim);
        if (status != 0) {
            return status;
        }
        TRACE_INFO("procByte: 0x%X\n\r", procByte);
        /* Handle NULL */
        if ( procByte == ISO_NULL_VAL ) {
            TRACE_INFO("INS\n\r");
            continue;
        }
        /* Handle SW1 */
        else if ( ((procByte & 0xF0) ==0x60) || ((procByte & 0xF0) ==0x90) ) {
            TRACE_INFO("SW1\n\r");
            SW1 = 1;
        }
        /* Handle INS */
        else if ( pAPDU[1] == procByte) {
            TRACE_INFO("HdlINS\n\r");
            if (cmdCase == CASE2) {
                /* receive data from card */
                do {
                    status = ISO7816_GetChar(&pMessage[indexMsg++], &usart_sim);
                } while(( 0 != --NeNc) && (status == 0) );
                if (status != 0) {
                    return status;
                }
            }
            else {
                 /* Send data */
                do {
                    TRACE_INFO("Send %X", pAPDU[indexApdu]);
                    ISO7816_SendChar(pAPDU[indexApdu++], &usart_sim);
                } while( 0 != --NeNc );
            }
        }
        /* Handle INS ^ 0xff */
        else
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wsign-compare"
        if ( pAPDU[1] == (procByte ^ 0xff)) {
        #pragma GCC diagnostic pop
            TRACE_INFO("HdlINS+\n\r");
            if (cmdCase == CASE2) {
                /* receive data from card */
                status = ISO7816_GetChar(&pMessage[indexMsg++], &usart_sim);
                if (status != 0) {
                    return status;
                }
                TRACE_INFO("Rcv: 0x%X\n\r", pMessage[indexMsg-1]);
            }
            else {
                status = ISO7816_SendChar(pAPDU[indexApdu++], &usart_sim);
                if (status != 0) {
                    return status;
                }
            }
            NeNc--;
        }
        else {
            /* ?? */
            TRACE_INFO("procByte=0x%X\n\r", procByte);
            break;
        }
    } while (NeNc != 0);

    /* Status Bytes */
    if (SW1 == 0) {
        status = ISO7816_GetChar(&pMessage[indexMsg++], &usart_sim); /* SW1 */
        if (status != 0) {
            return status;
        }
    }
    else {
        pMessage[indexMsg++] = procByte;
    }
    status = ISO7816_GetChar(&pMessage[indexMsg++], &usart_sim); /* SW2 */
    if (status != 0) {
        return status;
    }

    TRACE_WARNING("SW1=0x%X, SW2=0x%X\n\r", pMessage[indexMsg-2], pMessage[indexMsg-1]);

    *retlen = indexMsg;
    return status;

}

/**
 *  Escape ISO7816
 */
void ISO7816_Escape( void )
{
    TRACE_DEBUG("For user, if needed\n\r");
}

/**
 *  Restart clock ISO7816
 */
void ISO7816_RestartClock( void )
{
    TRACE_DEBUG("ISO7816_RestartClock\n\r");
    USART_SIM->US_BRGR = 13;
}

/**
 *  Stop clock ISO7816
 */
void ISO7816_StopClock( void )
{
    TRACE_DEBUG("ISO7816_StopClock\n\r");
    USART_SIM->US_BRGR = 0;
}

/**
 *  T0 APDU
 */
void ISO7816_toAPDU( void )
{
    TRACE_DEBUG("ISO7816_toAPDU\n\r");
    TRACE_DEBUG("Not supported at this time\n\r");
}

/**
 * Answer To Reset (ATR)
 * \param pAtr    ATR buffer
 * \param pLength Pointer for store the ATR length
 * \return 0: if timeout else status of US_CSR
 */
uint32_t ISO7816_Datablock_ATR( uint8_t* pAtr, uint8_t* pLength )
{
    uint32_t i;
    uint32_t j;
    uint32_t y;
    uint32_t status = 0; 

    *pLength = 0;

    /* Read ATR TS */
    // FIXME: There should always be a check for the GetChar return value..0 means timeout
    status = ISO7816_GetChar(&pAtr[0], &usart_sim);
    if (status != 0) {
        return status;
    }

    /* Read ATR T0 */
    status = ISO7816_GetChar(&pAtr[1], &usart_sim);
    if (status != 0) {
        return status;
    }
    y = pAtr[1] & 0xF0;
    i = 2;

    /* Read ATR Ti */
    while (y && (status == 0)) {

        if (y & 0x10) {  /* TA[i] */
            status = ISO7816_GetChar(&pAtr[i++], &usart_sim);
        }
        if (y & 0x20) {  /* TB[i] */
            status = ISO7816_GetChar(&pAtr[i++], &usart_sim);
        }
        if (y & 0x40) {  /* TC[i] */
            status = ISO7816_GetChar(&pAtr[i++], &usart_sim);
        }
        if (y & 0x80) {  /* TD[i] */
            status = ISO7816_GetChar(&pAtr[i], &usart_sim);
            y =  pAtr[i++] & 0xF0;
        }
        else {
            y = 0;
        }
    }
    if (status != 0) {
        return status;
    }

    /* Historical Bytes */
    y = pAtr[1] & 0x0F;
    for( j=0; (j < y) && (status == 0); j++ ) {
        status = ISO7816_GetChar(&pAtr[i++], &usart_sim);
    }

    if (status != 0) {
        return status;
    }

    *pLength = i;
    return status;
}

/**
 * Set data rate and clock frequency
 * \param dwClockFrequency ICC clock frequency in KHz.
 * \param dwDataRate       ICC data rate in bpd
 */
void ISO7816_SetDataRateandClockFrequency( uint32_t dwClockFrequency, uint32_t dwDataRate )
{
    uint8_t ClockFrequency;

    /* Define the baud rate divisor register */
    /* CD  = MCK / SCK */
    /* SCK = FIDI x BAUD = 372 x 9600 */
    /* BOARD_MCK */
    /* CD = MCK/(FIDI x BAUD) = 48000000 / (372x9600) = 13 */
    USART_SIM->US_BRGR = BOARD_MCK / (dwClockFrequency*1000);

    ClockFrequency = BOARD_MCK / USART_SIM->US_BRGR;

    USART_SIM->US_FIDI = (ClockFrequency)/dwDataRate;

}

/**
 * Pin status for ISO7816 RESET
 * \return 1 if the Pin RstMC is high; otherwise 0.
 */
uint8_t ISO7816_StatusReset( void )
{
    if (st_pinIso7816RstMC) {
        return PIO_Get(st_pinIso7816RstMC);
    }
    return 0;
}

/**
 *  cold reset
 */
void ISO7816_cold_reset( void )
{
    volatile uint32_t i;

    /* tb: wait ??? cycles*/
    for( i=0; i<(400*(BOARD_MCK/1000000)); i++ ) {
    }

    USART_SIM->US_RHR;
    USART_SIM->US_CR = US_CR_RSTSTA | US_CR_RSTIT | US_CR_RSTNACK;

    ISO7816_IccPowerOn();
}

/**
 *  Warm reset
 */
void ISO7816_warm_reset( void )
{
    volatile uint32_t i;

// Clears Reset
    ISO7816_IccPowerOff();

    /* tb: wait ??? cycles */
    for( i=0; i<(400*(BOARD_MCK/1000000)); i++ ) {
    }

    USART_SIM->US_RHR;
    USART_SIM->US_CR = US_CR_RSTSTA | US_CR_RSTIT | US_CR_RSTNACK;

// Sets Reset
    ISO7816_IccPowerOn();
}

/**
 * Decode ATR trace
 * \param pAtr pointer on ATR buffer
 */
void ISO7816_Decode_ATR( uint8_t* pAtr )
{
    uint32_t i;
    uint32_t j;
    uint32_t y;
    uint8_t offset;

    printf("\n\r");
    printf("ATR: Answer To Reset:\n\r");
    printf("TS = 0x%X Initial character ",pAtr[0]);
    if( pAtr[0] == 0x3B ) {

        printf("Direct Convention\n\r");
    }
    else {
        if( pAtr[0] == 0x3F ) {

            printf("Inverse Convention\n\r");
        }
        else {
            printf("BAD Convention\n\r");
        }
    }

    printf("T0 = 0x%X Format caracter\n\r",pAtr[1]);
    printf("    Number of historical bytes: K = %d\n\r", pAtr[1]&0x0F);
    printf("    Presence further interface byte:\n\r");
    if( pAtr[1]&0x80 ) {
        printf("TA ");
    }
    if( pAtr[1]&0x40 ) {
        printf("TB ");
    }
    if( pAtr[1]&0x20 ) {
        printf("TC ");
    }
    if( pAtr[1]&0x10 ) {
        printf("TD ");
    }
    if( pAtr[1] != 0 ) {
        printf(" present\n\r");
    }

    i = 2;
    y = pAtr[1] & 0xF0;

    /* Read ATR Ti */
    offset = 1;
    while (y) {

        if (y & 0x10) {  /* TA[i] */
            printf("TA[%d] = 0x%X ", offset, pAtr[i]);
            if( offset == 1 ) {
                printf("FI = %d ", (pAtr[i]>>8));
                printf("DI = %d", (pAtr[i]&0x0F));
            }
            printf("\n\r");
            i++;
        }
        if (y & 0x20) {  /* TB[i] */
            printf("TB[%d] = 0x%X\n\r", offset, pAtr[i]);
            i++;
        }
        if (y & 0x40) {  /* TC[i] */
            printf("TC[%d] = 0x%X ", offset, pAtr[i]);
            if( offset == 1 ) {
                printf("Extra Guard Time: N = %d", pAtr[i]);
            }
            printf("\n\r");
            i++;
        }
        if (y & 0x80) {  /* TD[i] */
            printf("TD[%d] = 0x%X\n\r", offset, pAtr[i]);
            y =  pAtr[i++] & 0xF0;
        }
        else {
            y = 0;
        }
        offset++;
    }

    /* Historical Bytes */
    printf("Historical bytes:\n\r");
    y = pAtr[1] & 0x0F;
    for( j=0; j < y; j++ ) {
        printf(" 0x%X", pAtr[i]);
        i++;
    }
    printf("\n\r\n\r");

}

void ISO7816_Set_Reset_Pin(const Pin *pPinIso7816RstMC) {
    /* Pin ISO7816 initialize */
    st_pinIso7816RstMC  = (Pin *)pPinIso7816RstMC;
}

/** Initializes a ISO driver
 *  \param pPinIso7816RstMC Pin ISO 7816 Rst MC
 */
void ISO7816_Init( Usart_info *usart, bool master_clock )
{
    uint32_t clk;
    TRACE_DEBUG("ISO_Init\n\r");

    Usart *us_base = usart->base;
    uint32_t us_id = usart->id;

    if (master_clock == true) {
        clk = US_MR_USCLKS_MCK;
    } else {
        clk = US_MR_USCLKS_SCK;
    }

    USART_Configure( us_base,
                     US_MR_USART_MODE_IS07816_T_0
                     | clk
                     | US_MR_NBSTOP_1_BIT
                     | US_MR_PAR_EVEN
                     | US_MR_CHRL_8_BIT
                     | US_MR_CLKO
                     | US_MR_INACK  /* Inhibit errors */
                     | (3<<24), /* MAX_ITERATION */
                     1,
                     0);

    /* Disable interrupts */
    us_base->US_IDR = (uint32_t) -1;

    /* Configure USART */
    PMC_EnablePeripheral(us_id);

    us_base->US_FIDI = 372;  /* by default */
    /* Define the baud rate divisor register */
    /* CD  = MCK / SCK */
    /* SCK = FIDI x BAUD = 372 x 9600 */
    /* BOARD_MCK */
    /* CD = MCK/(FIDI x BAUD) = 48000000 / (372x9600) = 13 */
    if (master_clock == true) {
        us_base->US_BRGR = BOARD_MCK / (372*9600);
    } else {
        us_base->US_BRGR = US_BRGR_CD(1);
    }
}

