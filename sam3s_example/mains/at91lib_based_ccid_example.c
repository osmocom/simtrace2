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

/** USB states */
/// Use for power management
#define STATE_IDLE    0
/// The USB device is in suspend state
#define STATE_SUSPEND 4
/// The USB device is in resume state
#define STATE_RESUME  5

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
/** USB state: suspend, resume, idle */
unsigned char USBState = STATE_IDLE;

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

static const Pin pPwr[] = {
    /* Enable power converter 4.5-6V to 3.3V; low: off */
    {SIM_PWEN, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT},
    
    /* Enable second power converter: VCC_PHONE to VCC_SIM; high: off */
    {VCC_FWD, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
};

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
    // PA10 is DTXD, which is the debug uart transmit pin

    printf("Interrupt!!\n\r");
    /*  Check all pending interrupts */
    // FIXME: this if condition is not always true...
//    if ( (pinSmartCard.pio->PIO_ISR & pinSmartCard.mask) != 0 )
    {
        /*  Check current level on pin */
        if ( PIO_Get( &pinSmartCard ) == 0 )
        {
            sim_inserted = 1;
            printf( "-I- Smartcard inserted\n\r" ) ;
            CCID_Insertion();
        }
        else
        {
            sim_inserted = 0;
            printf( "-I- Smartcard removed\n\r" ) ;
            CCID_Removal();
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


/*------------------------------------------------------------------------------
 *        USB 
 *------------------------------------------------------------------------------*/

// FIXME: Implement those two functions
//------------------------------------------------------------------------------
/// Put the CPU in 32kHz, disable PLL, main oscillator
/// Put voltage regulator in standby mode
//------------------------------------------------------------------------------
void LowPowerMode(void)
{
// FIXME: Implement low power consumption mode?
//    PMC_CPUInIdleMode();
}
//------------------------------------------------------------------------------
/// Put voltage regulator in normal mode
/// Return the CPU to normal speed 48MHz, enable PLL, main oscillator
//------------------------------------------------------------------------------
void NormalPowerMode(void)
{
}


/*------------------------------------------------------------------------------
 *        Main 
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

    printf("%s", "=====================================================\n\r");
    printf( "-- USART ISO7816 Example %s --\n\r", SOFTPACK_VERSION ) ;
    printf( "-- %s\n\r", BOARD_NAME ) ;
    printf( "-- Compiled: %s %s --\n\r", __DATE__, __TIME__ ) ;
    printf("%s", "=====================================================\n\r");


    PIO_InitializeInterrupts(0);    

    /*  Configure IT on Smart Card */
    ConfigureCardDetection() ;

    /* Configure Phone SIM connection*/
/*    Pin phone_sim_connect[] = {PIN_SC_SW, PIN_IO_SW};
    PIO_Configure( phone_sim_connect, PIO_LISTSIZE(phone_sim_connect) );
*/

    /*  Configure ISO7816 driver */
    PIO_Configure( pinsISO7816, PIO_LISTSIZE( pinsISO7816 ) ) ;
    PIO_Configure(pPwr, PIO_LISTSIZE(pPwr));

// Card has no power until now
    PIO_Set(&pPwr[0]);

// FIXME: RST seems to be allways low here 
    TRACE_DEBUG("******* Reset State1 (1 if the Pin RstMC is high): 0x%X", ISO7816_StatusReset());

    ISO7816_Init( pinIso7816RstMC ) ;
    
    TRACE_DEBUG("******* Reset State2 (1 if the Pin RstMC is high): 0x%X", ISO7816_StatusReset());

    /*  Read ATR */
    ISO7816_warm_reset() ;

    ISO7816_Datablock_ATR( pAtr, &ucSize ) ;
    TRACE_DEBUG("******* Reset State3 (1 if the Pin RstMC is high): 0x%X", ISO7816_StatusReset());

    /*  Decode ATR */
    ISO7816_Decode_ATR( pAtr ) ;

// FIXME: RST seems to be allways high
    TRACE_DEBUG("******* Reset State4 (1 if the Pin RstMC is high): 0x%X", ISO7816_StatusReset());
    //SendReceiveCommands() ;
    
    CCIDDriver_Initialize();

// FIXME: Or do I need to call VBUS_CONFIGURE() here instead, which will call USBD_Connect() later?
    USBD_Connect();
// FIXME: USB clock? USB PMC?
    NVIC_EnableIRQ( UDP_IRQn );

   // FIXME: At some point USBD_IrqHandler() should get called and set USBD_STATE_CONFIGURED
    while (USBD_GetState() < USBD_STATE_CONFIGURED) {
        int i = 1; 
        if ((i%10000) == 0) {
            TRACE_DEBUG("%d: USB State: %x\n\r", i, USBD_GetState());
        }
        i++;
    }

// FIXME. what if smcard is not inserted?
    if(PIO_Get(&pinSmartCard) == 0) {
        printf("SIM card inserted\n\r");
        CCID_Insertion();
    }

    // Infinite loop
    while (1) {

        if( USBState == STATE_SUSPEND ) {
            TRACE_DEBUG("suspend  !\n\r");
            LowPowerMode();
            USBState = STATE_IDLE;
        }
        if( USBState == STATE_RESUME ) {
            // Return in normal MODE
            TRACE_DEBUG("resume !\n\r");
            NormalPowerMode();
            USBState = STATE_IDLE;
        }
        CCID_SmartCardRequest();
    }

    return 0 ;
}

