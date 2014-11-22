#ifndef _BOARD_
#define _BOARD_

#include "chip.h"
#include "syscalls.h" /** RedHat Newlib minimal stub */

/** Name of the board */
#define BOARD_NAME "SAM3S-EK"
/** Board definition */
#define sam3sek
/** Family definition (already defined) */
#define sam3s
/** Core definition */
#define cortexm3


#define BOARD_MAINOSC 12000000
#define BOARD_MCK     48000000

//#define LED_STATUS PIO_PA8
#define LED_STATUS PIO_PA17
#define LED_STATUS2 PIO_PA18

#endif
