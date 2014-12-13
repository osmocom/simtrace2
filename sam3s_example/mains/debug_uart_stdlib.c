#include "board.h"

#include <stdio.h>
#include <string.h>

extern void UART_PutString(const char *str, int len);

/*----------------------------------------------------------------------------
 *  *        Variables
 *   *----------------------------------------------------------------------------*/

const Pin redled = {LED_RED, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
const Pin greenled = {LED_GREEN, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};

static const Pin *led;

void Configure_LED() {
    PIO_Configure(&greenled, PIO_LISTSIZE(greenled));
    PIO_Configure(&redled, PIO_LISTSIZE(redled));
    PIO_Set(&redled);
    PIO_Set(&greenled);
    led = &redled;
}

int main() {
    size_t ret = 0;
    Configure_LED();

    ret = printf("Clockval: %d\r\n", BOARD_MCK);

    if (ret < 0){
        PIO_Clear(&redled);
    } else {
        PIO_Clear(&greenled);
        while (1) {
            printf("Clockval**++????: %d\r\n", BOARD_MCK);
        }
    }

    return 0;
}
