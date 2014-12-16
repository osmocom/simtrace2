#include "board.h"

#include <stdio.h>
#include <string.h>

extern void UART_PutString(const char *str, int len);

void Configure_LEDs() {
    LED_Configure(LED_NUM_RED);
    LED_Configure(LED_NUM_GREEN);
}

int main() {
    size_t ret = 0;
    
    Configure_LEDs();

    ret = printf("Clockval: %d\r\n", BOARD_MCK);

    if (ret < 0){
        LED_Clear(LED_NUM_GREEN);
        LED_Set(LED_NUM_RED);
    } else {
        LED_Clear(LED_NUM_RED);
        LED_Set(LED_NUM_GREEN);

        while (1) {
            printf("Clockval**++????: %d\r\n", BOARD_MCK);
        }
    }

    return 0;
}
