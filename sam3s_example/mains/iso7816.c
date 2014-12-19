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
 *  \page usart_iso7816 USART ISO7816 Example
 *
 *  \section Purpose
 *  This example sends ISO 7816 commands to a smartcard connected to the
 *  evaluation kits on sam3s.
 *
 *  \section Requirements
 *  This example can be used on sam3s evaluation kit. Please connect the
 *  smartcard contacts with following pins which could be easily wired from
 *  the board.
 *  - <b>SAM3S-EK   -- SMARTCARD</b>
 *   - PA11       -- RST
 *   - TXD1(PA22) -- I/O
 *   - SCK1(PA23) -- CLK
 *
 *  \section Description
 *  The iso7816 software provide in this examples is use to transform APDU
 *  commands to TPDU commands for the smart card.
 *  The iso7816 provide here is for the protocol T=0 only.
 *  The send and the receive of a character is made under polling.
 *  In the file ISO7816_Init is defined all pins of the card. User must have to
 *  change this pins according to his environment.
 *  The driver is compliant with CASE 1, 2, 3 of the ISO7816-4 specification.
 *
 *  \section Usage
 *  -# Build the program and download it inside the evaluation board. Please
 *     refer to the <a href="http://www.atmel.com/dyn/resources/prod_documents/doc6224.pdf">SAM-BA User Guide</a>,
 *     the <a href="http://www.atmel.com/dyn/resources/prod_documents/doc6310.pdf">GNU-Based Software Development</a>
 *     application note or to the <a href="ftp://ftp.iar.se/WWWfiles/arm/Guides/EWARM_UserGuide.ENU.pdf">IAR EWARM User Guide</a>,
 *     depending on your chosen solution.
 *  -# On the computer, open and configure a terminal application (e.g.
 *     HyperTerminal on Microsoft Windows) with these settings:
 *        - 115200 bauds
 *        - 8 data bits
 *        - No parity
 *        - 1 stop bit
 *        - No flow control
 *  -# Connect the card reader to sam3s-ek board:
 *        <table border="1" cellpadding="2" cellspacing="0">
 *        <tr><td>C1: Vcc:   7816_3V5V </td><td> C5: Gnd</td> <td> C4: RFU</td></tr>
 *        <tr><td> C2: Reset: 7816_RST</td> <td>  C6: Vpp</td> <td> C8: RFU</td></tr>
 *        <tr><td> C3: Clock: 7816_CLK</td> <td>  C7: 7816_IO</td> </tr>
 *        </table>
 *     If necessary,another pin must be connected on the card reader for detecting the
 *     insertion and removal: 7816_IRQ.
 *     On Atmel boards, all this pins can be easily connecting with jumpers.
 *  -# Start the application. The following traces shall appear on the terminal:
 *     \code
 *      -- USART ISO7816 Example xxx --
 *      -- xxxxxx-xx
 *      -- Compiled: xxx xx xxxx xx:xx:xx --
 *      Display the ATR
 *     \endcode
 *
 *   \section References
 *  - usart_iso7816/main.c
 *  - iso7816_4.c
 *  - pio.h
 *  - usart.h
 *
 */

/** \file
 *
 *  This file contains all the specific code for the usart_iso7816 example.
 *
 */


/*------------------------------------------------------------------------------
 *          Headers
 *------------------------------------------------------------------------------*/

#include "board.h"

#include <string.h>

/*------------------------------------------------------------------------------
 *         Internal definitions
 *------------------------------------------------------------------------------*/

/** Maximum ucSize in bytes of the smartcard answer to a command.*/
#define MAX_ANSWER_SIZE         10

/** Maximum ATR ucSize in bytes.*/
#define MAX_ATR_SIZE            55

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/

/** Test command #1.*/
static const uint8_t testCommand1[] = {0x00, 0x10, 0x00, 0x00};
/** Test command #2.*/
static const uint8_t testCommand2[] = {0x00, 0x20, 0x00, 0x00, 0x02};
/** Test command #3.*/
static const uint8_t testCommand3[] = {0x00, 0x30, 0x00, 0x00, 0x02, 0x0A, 0x0B};
/* Select  MF */
static const uint8_t testCommand4[] = {0x00, 0xa4, 0x00, 0x00, 0x02, 0x3f, 0x00};
/** ISO7816 pins */
static const Pin pinsISO7816[]    = {PINS_ISO7816};
/** ISO7816 RST pin */
static const Pin pinIso7816RstMC  = PIN_ISO7816_RSTMC;
static uint8_t sim_inserted = 0;
/*------------------------------------------------------------------------------
 *         Internal functions
 *------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *         Optional smartcard detection
 *------------------------------------------------------------------------------*/

#ifdef SMARTCARD_CONNECT_PIN

/** Smartcard detection pin.*/
static const Pin pinSmartCard = SMARTCARD_CONNECT_PIN;

/**
 * PIO interrupt service routine. Checks if the smartcard has been connected
 * or disconnected.
 */
static void ISR_PioSmartCard( const Pin *pPin )
{
/* FIXME: why is pinSmartCard.pio->PIO_ISR the wrong number?
    printf("+++++ Trying to check for pending interrupts (PIO ISR: 0x%X)\n\r", pinSmartCard.pio->PIO_ISR);
    printf("+++++ Mask: 0x%X\n\r", pinSmartCard.mask);
Output:
    +++++ Trying to check for pending interrupts (PIO ISR: 0x400)) = 1<<10
    +++++ Mask: 0x100 = 1<<8
*/
    /*  Check all pending interrupts */
    if ( (pinSmartCard.pio->PIO_ISR & pinSmartCard.mask) != 0 )
    {
        /*  Check current level on pin */
        if ( PIO_Get( &pinSmartCard ) == 0 )
        {
            sim_inserted = 1;
            printf( "-I- Smartcard inserted\n\r" ) ;
        }
        else
        {
            sim_inserted = 0;
            printf( "-I- Smartcard removed\n\r" ) ;
        }
    }
}

/**
 * Configures the smartcard detection pin to trigger an interrupt.
 */
static void ConfigureCardDetection( void )
{
    printf("+++++ Configure PIOs\n\r");
    PIO_Configure( &pinSmartCard, 1 ) ;
    NVIC_EnableIRQ( PIOA_IRQn );
// FIXME: Do we need to set priority?: NVIC_SetPriority( PIOA_IRQn, 10);
    PIO_ConfigureIt( &pinSmartCard, ISR_PioSmartCard ) ;
    PIO_EnableIt( &pinSmartCard ) ;
}

#else

/**
 * Dummy implementation.
 */
static void ConfigureCardDetection( void )
{
    printf( "-I- Smartcard detection not supported.\n\r" ) ;
}

#endif

/**
 * Displays a menu which enables the user to send several commands to the
 * smartcard and check its answers.
 */
static void SendReceiveCommands( void )
{
    uint8_t pMessage[MAX_ANSWER_SIZE];
    uint8_t ucSize ;
    uint8_t ucKey ;
    uint8_t command;
    uint8_t i;

    /*  Clear message buffer */
    memset( pMessage, 0, sizeof( pMessage ) ) ;

    /*  Display menu */
    printf( "-I- The following three commands can be sent:\n\r" ) ;
    printf( "  1. " ) ;
    for ( i=0 ; i < sizeof( testCommand1 ) ; i++ )
    {
        printf( "0x%X ", testCommand1[i] ) ;
    }
    printf( "\n\r  2. " ) ;

    for ( i=0 ; i < sizeof( testCommand2 ) ; i++ )
    {
        printf( "0x%X ", testCommand2[i] ) ;
    }
    printf( "\n\r  3. " ) ;

    for ( i=0 ; i < sizeof( testCommand3 ) ; i++ )
    {
        printf( "0x%X ", testCommand3[i] ) ;
    }
    printf( "\n\r  4. " ) ;

    for ( i=0 ; i < sizeof( testCommand4 ) ; i++ )
    {
        printf( "0x%X ", testCommand4[i] ) ;
    }
    printf( "\n\r" ) ;

    /*  Get user input */
    ucKey = 0 ;
    while ( ucKey != 'q' )
    {
        printf( "\r                        " ) ;
        printf( "\rChoice ? (q to quit): " ) ;
#if defined (  __GNUC__  )
        fflush(stdout);
#endif
        ucKey = UART_GetChar() ;
        printf( "%c", ucKey ) ;
        command = ucKey - '0';

        /*  Check user input */
        ucSize = 0 ;
        if ( command == 1 )
        {
            printf( "\n\r-I- Sending command " ) ;
            for ( i=0 ; i < sizeof( testCommand1 ) ; i++ )
            {
                printf( "0x%02X ", testCommand1[i] ) ;
            }
            printf( "...\n\r" ) ;
            ucSize = ISO7816_XfrBlockTPDU_T0( testCommand1, pMessage, sizeof( testCommand1 ) ) ;
        }
        else
        {
            if ( command == 2 )
            {
                printf( "\n\r-I- Sending command " ) ;
                for ( i=0 ; i < sizeof( testCommand2 ) ; i++ )
                {
                    printf("0x%02X ", testCommand2[i] ) ;
                }
                printf( "...\n\r" ) ;
                ucSize = ISO7816_XfrBlockTPDU_T0( testCommand2, pMessage, sizeof( testCommand2 ) ) ;
            }
            else
            {
                if ( command == 3 )
                {
                    printf( "\n\r-I- Sending command " ) ;
                    for ( i=0 ; i < sizeof( testCommand3 ) ; i++ )
                    {
                        printf( "0x%02X ", testCommand3[i] ) ;
                    }
                    printf( "...\n\r" ) ;
                    ucSize = ISO7816_XfrBlockTPDU_T0( testCommand3, pMessage, sizeof( testCommand3 ) ) ;
                }   
                else
                {
                    if ( command == 4 )
                    {
                        printf( "\n\r-I- Sending command " ) ;
                        for ( i=0 ; i < sizeof( testCommand4 ) ; i++ )
                        {
                            printf( "0x%02X ", testCommand4[i] ) ;
                        }
                        printf( "...\n\r" ) ;
                        ucSize = ISO7816_XfrBlockTPDU_T0( testCommand4, pMessage, sizeof( testCommand4 ) ) ;
                    }
                }
            }
       }

        /*  Output smartcard answer */
        if ( ucSize > 0 )
        {
            printf( "\n\rAnswer: " ) ;
            for ( i=0 ; i < ucSize ; i++ )
            {
                printf( "0x%02X ", pMessage[i] ) ;
            }
            printf( "\n\r" ) ;
        }
    }

    printf( "Quitting ...\n\r" ) ;
}

/*------------------------------------------------------------------------------
 *         Exported functions
 *------------------------------------------------------------------------------*/

/**
 * Initializes the DBGU and ISO7816 driver, and starts some tests.
 * \return Unused (ANSI-C compatibility)
 */
extern int main( void )
{
    uint8_t pAtr[MAX_ATR_SIZE] ;
    uint8_t ucSize ;

    LED_Configure(LED_NUM_GREEN);
    LED_Set(LED_NUM_GREEN);

    /* Disable watchdog*/
    WDT_Disable( WDT ) ;

    /*  Initialize Atr buffer */
    memset( pAtr, 0, sizeof( pAtr ) ) ;

    printf( "-- USART ISO7816 Example %s --\n\r", SOFTPACK_VERSION ) ;
    printf( "-- %s\n\r", BOARD_NAME ) ;
    printf( "-- Compiled: %s %s --\n\r", __DATE__, __TIME__ ) ;

    /*  Configure IT on Smart Card */
    ConfigureCardDetection() ;

    /*  Configure ISO7816 driver */
    PIO_Configure( pinsISO7816, PIO_LISTSIZE( pinsISO7816 ) ) ;

// FIXME: RST seems to be allways low here 
    printf("******* Reset State1 (1 if the Pin RstMC is high): 0x%X\n\r", ISO7816_StatusReset());

    ISO7816_Init( pinIso7816RstMC ) ;
    
    printf("******* Reset State2 (1 if the Pin RstMC is high): 0x%X\n\r", ISO7816_StatusReset());

    /*  Read ATR */
    ISO7816_warm_reset() ;

    ISO7816_Datablock_ATR( pAtr, &ucSize ) ;
    printf("******* Reset State3 (1 if the Pin RstMC is high): 0x%X\n\r", ISO7816_StatusReset());

    /*  Decode ATR */
    ISO7816_Decode_ATR( pAtr ) ;

// FIXME: RST seems to be allways high
    printf("******* Reset State4 (1 if the Pin RstMC is high): 0x%X\n\r", ISO7816_StatusReset());

    /*  Allow user to send some commands */
    SendReceiveCommands() ;

    return 0 ;
}

