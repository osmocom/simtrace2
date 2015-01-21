#include "board.h"
#include "pio.h"

const Pin statusled = {PIO_PA18, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
const Pin statusled2 = {PIO_PA17, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
const Pin io = PIN_PHONE_IO;

int main() 
{
    register int i = 0;
    register int b = 0;
	
    PIO_Configure(&io, PIO_LISTSIZE(io));
	PIO_Clear(&io);

	PIO_Configure(&statusled, PIO_LISTSIZE(statusled));
	PIO_Clear(&statusled);
	PIO_Configure(&statusled2, PIO_LISTSIZE(statusled2));
	PIO_Clear(&statusled2);

    for(;;) {
        i = i+1;
        if ((i%100000) == 0) {
            switch(b) {
                case 0:
                    PIO_Set(&statusled);
                    b=1;
                    PIO_Set(&io);
                    break;
                case 1:
                    PIO_Set(&statusled2);
                    PIO_Set(&statusled);
                    b = 2;
                    PIO_Clear(&io);
                    break;
                case 2:
                    PIO_Clear(&statusled);
                    b = 3;
                    PIO_Set(&io);
                    break;
                case 3:
                    PIO_Clear(&statusled2);
                    b = 0;
                    break;
                    PIO_Clear(&io);
                default:
                    b = 0;
            } 
        }
    }
    
	return i;
}

/* vim: set ts=4: */
