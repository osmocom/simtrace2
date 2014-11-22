#include "include/pio.h"
#include "include/board.h"

#define sam3s2

const Pin statusled = {LED_STATUS, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
const Pin statusled2 = {LED_STATUS2, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};

int c_entry()
{
//    PIO_OER =  0x00060000;   
    PIO_Configure(&statusled, PIO_LISTSIZE(statusled));
    PIO_Clear(&statusled);
    
    PIO_Configure(&statusled2, PIO_LISTSIZE(statusled));
    PIO_Set(&statusled2);

    return 0;
}

