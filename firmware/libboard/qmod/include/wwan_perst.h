#pragma once

int wwan_perst_set(int modem_nr, int active);
int wwan_perst_do_reset_pulse(int modem_nr, unsigned int duration_ms);
int wwan_perst_init(void);
