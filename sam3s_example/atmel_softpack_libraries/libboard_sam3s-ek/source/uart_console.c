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
 * Implements UART console.
 *
 */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "board.h"

#include <stdio.h>
#include <stdint.h>

/*----------------------------------------------------------------------------
 *        Definitions
 *----------------------------------------------------------------------------*/

/** Console baudrate always using 115200. */
#define CONSOLE_BAUDRATE    115200
/** Usart Hw interface used by the console (UART0). */
#define CONSOLE_USART       UART0
/** Usart Hw ID used by the console (UART0). */
#define CONSOLE_ID          ID_UART0
/** Pins description corresponding to Rxd,Txd, (UART pins) */
#define CONSOLE_PINS        {PINS_UART}

/*----------------------------------------------------------------------------
 *        Variables
 *----------------------------------------------------------------------------*/

/** Is Console Initialized. */
static uint8_t _ucIsConsoleInitialized=0 ;

/**
 * \brief Configures an USART peripheral with the specified parameters.
 *
 * \param baudrate  Baudrate at which the USART should operate (in Hz).
 * \param masterClock  Frequency of the system master clock (in Hz).
 */
extern void UART_Configure( uint32_t baudrate, uint32_t masterClock)
{
    const Pin pPins[] = CONSOLE_PINS;
    Uart *pUart = CONSOLE_USART;

    /* Configure PIO */
    PIO_Configure(pPins, PIO_LISTSIZE(pPins));

    /* Configure PMC */
    PMC->PMC_PCER0 = 1 << CONSOLE_ID;

    /* Reset and disable receiver & transmitter */
    pUart->UART_CR = UART_CR_RSTRX | UART_CR_RSTTX
                   | UART_CR_RXDIS | UART_CR_TXDIS;

    /* Configure mode */
    pUart->UART_MR =  UART_MR_PAR_NO;

    /* Configure baudrate */
    /* Asynchronous, no oversampling */
    pUart->UART_BRGR = (masterClock / baudrate) / 16;

    /* Disable PDC channel */
    pUart->UART_PTCR = UART_PTCR_RXTDIS | UART_PTCR_TXTDIS;

    /* Enable receiver and transmitter */
    pUart->UART_CR = UART_CR_RXEN | UART_CR_TXEN;

    _ucIsConsoleInitialized=1 ;
}

/**
 * \brief Outputs a character on the UART line.
 *
 * \note This function is synchronous (i.e. uses polling).
 * \param c  Character to send.
 */
extern void UART_PutChar( uint8_t c )
{
    Uart *pUart=CONSOLE_USART ;

    if ( !_ucIsConsoleInitialized )
    {
        UART_Configure(CONSOLE_BAUDRATE, BOARD_MCK);
    }

    /* Wait for the transmitter to be ready */
    while ( (pUart->UART_SR & UART_SR_TXEMPTY) == 0 ) ;

    /* Send character */
    pUart->UART_THR=c ;

}

/**
 * \brief Input a character from the UART line.
 *
 * \note This function is synchronous
 * \return character received.
 */
extern uint32_t UART_GetChar( void )
{
    Uart *pUart=CONSOLE_USART ;

    if ( !_ucIsConsoleInitialized )
    {
        UART_Configure(CONSOLE_BAUDRATE, BOARD_MCK);
    }

    while ( (pUart->UART_SR & UART_SR_RXRDY) == 0 ) ;

    return pUart->UART_RHR ;
}

/**
 * \brief Check if there is Input from UART line.
 *
 * \return true if there is Input.
 */
extern uint32_t UART_IsRxReady( void )
{
    Uart *pUart=CONSOLE_USART ;

    if ( !_ucIsConsoleInitialized )
    {
        UART_Configure( CONSOLE_BAUDRATE, BOARD_MCK ) ;
    }

    return (pUart->UART_SR & UART_SR_RXRDY) > 0 ;
}

/**
 *  Displays the content of the given frame on the UART0.
 *
 *  \param pucFrame Pointer to the frame to dump.
 *  \param dwSize   Buffer size in bytes.
 */
extern void UART_DumpFrame( uint8_t* pucFrame, uint32_t dwSize )
{
    uint32_t dw ;

    for ( dw=0 ; dw < dwSize ; dw++ )
    {
        printf( "%02X ", pucFrame[dw] ) ;
    }

    printf( "\n\r" ) ;
}

/**
 *  Displays the content of the given buffer on the UART0.
 *
 *  \param pucBuffer  Pointer to the buffer to dump.
 *  \param dwSize     Buffer size in bytes.
 *  \param dwAddress  Start address to display
 */
extern void UART_DumpMemory( uint8_t* pucBuffer, uint32_t dwSize, uint32_t dwAddress )
{
    uint32_t i ;
    uint32_t j ;
    uint32_t dwLastLineStart ;
    uint8_t* pucTmp ;

    for ( i=0 ; i < (dwSize / 16) ; i++ )
    {
        printf( "0x%08X: ", (unsigned int)(dwAddress + (i*16)) ) ;
        pucTmp = (uint8_t*)&pucBuffer[i*16] ;

        for ( j=0 ; j < 4 ; j++ )
        {
            printf( "%02X%02X%02X%02X ", pucTmp[0], pucTmp[1], pucTmp[2], pucTmp[3] ) ;
            pucTmp += 4 ;
        }

        pucTmp=(uint8_t*)&pucBuffer[i*16] ;

        for ( j=0 ; j < 16 ; j++ )
        {
            UART_PutChar( *pucTmp++ ) ;
        }

        printf( "\n\r" ) ;
    }

    if ( (dwSize%16) != 0 )
    {
        dwLastLineStart=dwSize - (dwSize%16) ;

        printf( "0x%08X: ", (unsigned int)(dwAddress + dwLastLineStart) ) ;
        for ( j=dwLastLineStart ; j < dwLastLineStart+16 ; j++ )
        {
            if ( (j!=dwLastLineStart) && (j%4 == 0) )
            {
                printf( " " ) ;
            }

            if ( j < dwSize )
            {
                printf( "%02X", pucBuffer[j] ) ;
            }
            else
            {
                printf("  ") ;
            }
        }

        printf( " " ) ;
        for ( j=dwLastLineStart ; j < dwSize ; j++ )
        {
            UART_PutChar( pucBuffer[j] ) ;
        }

        printf( "\n\r" ) ;
    }
}

/**
 *  Reads an integer
 *
 *  \param pdwValue  Pointer to the uint32_t variable to contain the input value.
 */
extern uint32_t UART_GetInteger( uint32_t* pdwValue )
{
    uint8_t ucKey ;
    uint8_t ucNbNb=0 ;
    uint32_t dwValue=0 ;

    while ( 1 )
    {
        ucKey=UART_GetChar() ;
        UART_PutChar( ucKey ) ;

        if ( ucKey >= '0' &&  ucKey <= '9' )
        {
            dwValue = (dwValue * 10) + (ucKey - '0');
            ucNbNb++ ;
        }
        else
        {
            if ( ucKey == 0x0D || ucKey == ' ' )
            {
                if ( ucNbNb == 0 )
                {
                    printf( "\n\rWrite a number and press ENTER or SPACE!\n\r" ) ;
                    return 0 ;
                }
                else
                {
                    printf( "\n\r" ) ;
                    *pdwValue=dwValue ;

                    return 1 ;
                }
            }
            else
            {
                printf( "\n\r'%c' not a number!\n\r", ucKey ) ;

                return 0 ;
            }
        }
    }
}

/**
 *  Reads an integer and check the value
 *
 *  \param pdwValue  Pointer to the uint32_t variable to contain the input value.
 *  \param dwMin     Minimum value
 *  \param dwMax     Maximum value
 */
extern uint32_t UART_GetIntegerMinMax( uint32_t* pdwValue, uint32_t dwMin, uint32_t dwMax )
{
    uint32_t dwValue=0 ;

    if ( UART_GetInteger( &dwValue ) == 0 )
    {
        return 0 ;
    }

    if ( dwValue < dwMin || dwValue > dwMax )
 {
        printf( "\n\rThe number have to be between %d and %d\n\r", (int)dwMin, (int)dwMax ) ;

        return 0 ;
    }

    printf( "\n\r" ) ;

    *pdwValue = dwValue ;

    return 1 ;
}

/**
 *  Reads an hexadecimal number
 *
 *  \param pdwValue  Pointer to the uint32_t variable to contain the input value.
 */
extern uint32_t UART_GetHexa32( uint32_t* pdwValue )
{
    uint8_t ucKey ;
    uint32_t dw = 0 ;
    uint32_t dwValue = 0 ;

    for ( dw=0 ; dw < 8 ; dw++ )
    {
        ucKey = UART_GetChar() ;
        UART_PutChar( ucKey ) ;

        if ( ucKey >= '0' &&  ucKey <= '9' )
        {
            dwValue = (dwValue * 16) + (ucKey - '0') ;
        }
        else
        {
            if ( ucKey >= 'A' &&  ucKey <= 'F' )
            {
                dwValue = (dwValue * 16) + (ucKey - 'A' + 10) ;
            }
            else
            {
                if ( ucKey >= 'a' &&  ucKey <= 'f' )
                {
                    dwValue = (dwValue * 16) + (ucKey - 'a' + 10) ;
                }
                else
                {
                    printf( "\n\rIt is not a hexa character!\n\r" ) ;

                    return 0 ;
                }
            }
        }
    }

    printf("\n\r" ) ;
    *pdwValue = dwValue ;

    return 1 ;
}

#if defined __ICCARM__ /* IAR Ewarm 5.41+ */
/**
 * \brief Outputs a character on the UART.
 *
 * \param c  Character to output.
 *
 * \return The character that was output.
 */
extern WEAK signed int putchar( signed int c )
{
    UART_PutChar( c ) ;

    return c ;
}
#endif // defined __ICCARM__

