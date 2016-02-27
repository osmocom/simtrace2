#include <stdint.h>

#include "board.h"
#include "card_emu.h"

#if 0
void card_emu_process_rx_byte(struct card_handle *ch, uint8_t byte);
int card_emu_tx_byte(struct card_handle *ch);
void card_emu_io_statechg(struct card_handle *ch, enum card_io io, int active);
#endif

static struct Usart_info usart_info[2] = {
	{
		.base = USART_PHONE,
		.id = ID_USART_PHONE,
		.state = USART_RCV
	}, {}
};

static Usart *get_usart_by_chan(uint8_t uart_chan)
{
	switch (uart_chan) {
	case 0:
		return USART_PHONE;
	}
	return NULL;
}

void card_emu_uart_enable(uint8_t uart_chan, uint8_t rxtx)
{
	Usart *usart = get_usart_by_chan(uart_chan);
	USART_SetTransmitterEnabled(usart, 0);
	USART_SetReceiverEnabled(usart, 1);
}

int card_emu_uart_tx(uint8_t uart_chan, uint8_t byte)
{
	Usart_info *ui = &usart_info[uart_chan];
	ISO7816_SendChar(byte, ui);
	return 1;
}
