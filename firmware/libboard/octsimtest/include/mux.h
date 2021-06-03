#pragma once

void mux_init(void);
int mux_set_slot(uint8_t s);
int mux_get_slot(void);
void mux_set_freq(uint8_t s);

/* this reflects the wiring between U5 and U4 */
#define MUX_FREQ_DIV_2		0
#define MUX_FREQ_DIV_4		1
#define MUX_FREQ_DIV_16		2
#define MUX_FREQ_DIV_32		3
#define MUX_FREQ_DIV_32		3
#define MUX_FREQ_DIV_128	4
#define MUX_FREQ_DIV_512	5
#define MUX_FREQ_DIV_2048	6
#define MUX_FREQ_DIV_4096	7
