#pragma once

#include <stdint.h>

/* Table 7 of ISO 7816-3:2006 */
extern const uint16_t fi_table[];

/* Table 8 from ISO 7816-3:2006 */
extern const uint8_t di_table[];

/* compute the F/D ratio based on Fi and Di values */
int compute_fidi_ratio(uint8_t fi, uint8_t di);
