/* SimTrace TC (Timer / Clock) support code
 * (C) 2006 by Harald Welte <hwelte@hmw-consulting.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*------------------------------------------------------------------------------
 *          Headers
 *------------------------------------------------------------------------------*/
#include "board.h"

#include <string.h>

//FIXME:
static const Pin pTC[] = {{PIO_PA4B_TCLK0, PIO_PA0B_TIOA0, PIO_PA1B_TIOB0}};

/** Global timestamp in milliseconds since start of application */
volatile uint32_t dwTimeStamp = 0;
volatile uint8_t timeout_occured = 0;

// FIXME: Do I need the function?:
/**
 *  \brief Handler for Sytem Tick interrupt.
 *
 *  Process System Tick Event
 *  Increments the timestamp counter.
 */
void SysTick_Handler( void )
{
    dwTimeStamp ++;
    
}



void TC0_IrqHandler( void )
{
    volatile uint32_t dummy;
    /* Clear status bit to acknowledge interrupt */
    dummy = TC0->TC_CHANNEL[ 0 ].TC_SR;

    timeout_occured++;
//    printf("++++ TC0_Irq %d\n\r", timeout_occured);
}


void TC0_Counter_Reset( void )
{
    TC0->TC_CHANNEL[ 0 ].TC_CCR = TC_CCR_SWTRG ;
    timeout_occured = 0;
}

/*  == Timeouts ==
 *  One symbol is about 2ms --> Timeout = BUFLEN * 2ms ?
 *  For BUFLEN = 64 that is 7.8 Hz
 */
void Timer_Init() 
{
    uint32_t div;
    uint32_t tcclks;

    /** Enable peripheral clock. */
    PMC_EnablePeripheral(ID_TC0);

    /** Configure TC for a $ARG1 Hz frequency and trigger on RC compare. */
    TC_FindMckDivisor( 20, BOARD_MCK, &div, &tcclks, BOARD_MCK );
    TRACE_INFO("Chosen div, tcclk: %d, %d", div, tcclks);
    /* TC_CMR: TC Channel Mode Register: Capture Mode */
    /* CPCTRG: RC Compare resets the counter and starts the counter clock. */
    TC_Configure( TC0, 0, tcclks | TC_CMR_CPCTRG );
    /* TC_RC: TC Register C: contains the Register C value in real time. */
    TC0->TC_CHANNEL[ 0 ].TC_RC = ( BOARD_MCK / div ) / 4; 

    /* Configure and enable interrupt on RC compare */
    NVIC_EnableIRQ( (IRQn_Type)ID_TC0 );

    TC0->TC_CHANNEL[ 0 ].TC_IER = TC_IER_CPCS;  /* CPCS: RC Compare */
    TC_Start( TC0, 0 );

    return;

    /*** From here on we have code based on old simtrace code */

    /* Cfg PA4(TCLK0), PA0(TIOA0), PA1(TIOB0) */    

    PIO_Configure( pTC, PIO_LISTSIZE( pTC ) );



// FIXME:
//    PIO_ConfigureIt( &pinPhoneRST, ISR_PhoneRST ) ;
//    PIO_EnableIt( &pinPhoneRST ) ;

    /* enable interrupts for Compare-C and External Trigger */
    TC0->TC_CHANNEL[0].TC_IER = TC_IER_CPCS | TC_IER_ETRGS;

    //...
     /* Enable master clock for TC0 */
//     TC0->TC_CHANNEL[0].TC_CCR

    /* Reset to start timers */
    //...
}
