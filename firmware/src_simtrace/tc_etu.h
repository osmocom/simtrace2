#pragma once

#include <stdint.h>
void tc_etu_set_wtime(uint8_t chan_nr, uint16_t wtime);
void tc_etu_set_etu(uint8_t chan_nr, uint16_t etu);
void tc_etu_init(uint8_t chan_nr, void *handle);

extern void tc_etu_wtime_half_expired(void *handle);
extern void tc_etu_wtime_expired(void *handle);
