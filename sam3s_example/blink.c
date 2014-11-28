// Xtal: Default internal 4MHz
#include <SAM3S2B.h>
//#include <SAM3S.h>

int main () {
    int i,j;
    //i=1000;
    //j=10000;
    PMC->PMC_WPMR = 0x504D4300;// Disable write protect
    PMC->PMC_PCER0 = (1<<11);// Enable PIO clock
    PIOA->PIO_PER = (1<<19); //Enable PIO
    PIOA->PIO_OER = (1<<19); //Enable Output

    while (1) {
        PIOA->PIO_CODR = (1<<18);// Turn LED on
        for (i=1;i<1000;i++) // Waiting
            for (j=1;j<3000;j++);
        PIOA->PIO_SODR = (1<<18);// Turn LED off
        for (i=1;i<1000;i++) // Waiting
            for (j=1;j<2000;j++);
    }
}
