#pragma once
#include <stdint.h>
#include <osmocom/core/gsmtap.h>

int osmo_st2_gsmtap_init(const char *gsmtap_host);
int osmo_st2_gsmtap_send_apdu(uint8_t sub_type, const uint8_t *apdu, unsigned int len);
