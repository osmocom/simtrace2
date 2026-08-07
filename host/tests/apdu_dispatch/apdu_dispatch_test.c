#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <osmocom/simtrace2/apdu_dispatch.h>

const uint8_t get_data_c2_ca[] = { 0x81, 0xCA, 0x00, 0x5A, 0x00 };
const uint8_t get_data_c2_cb[] = { 0x81, 0xCB, 0x00, 0x5A, 0x00 };

/* teset data for 6cXX test */
const uint8_t get_data_c2_ca_le_23[] = { 0x81, 0xCA, 0x00, 0x5A, 0x23 };
const uint8_t get_data_c2_cb_le_23[] = { 0x81, 0xCb, 0x00, 0x5A, 0x23 };

#define APDU_SEGMENT_IN(apdu, exp_rc)				\
	do {								\
		memset(&ac, 0, sizeof(ac));				\
		printf("Testing " #apdu "\n");				\
		int rc = osmo_apdu_segment_in(&ac, apdu, ARRAY_SIZE(apdu), true); \
		if (rc != exp_rc)					\
			printf("%d (actual) != %d (expected)\n", rc, exp_rc);\
		OSMO_ASSERT(rc == exp_rc);				\
	} while (0)

#define APDU_SEGMENT_IN2(apdu, exp_rc)				\
	do {								\
		memset(&ac, 0, sizeof(ac));				\
		memset(&prev_ac, 0, sizeof(prev_ac));			\
		printf("Testing " #apdu "\n");				\
		int rc = osmo_apdu_segment_in2(&ac, &prev_ac, apdu, ARRAY_SIZE(apdu), true); \
		if (rc != exp_rc)					\
			printf("%d (actual) != %d (expected)\n", rc, exp_rc);\
		OSMO_ASSERT(rc == exp_rc);				\
	} while (0)

void test_apdu_dispatch_simple(void)
{
	struct osmo_apdu_context ac, prev_ac;
	APDU_SEGMENT_IN(get_data_c2_ca, APDU_ACT_TX_CAPDU_TO_CARD);
	APDU_SEGMENT_IN(get_data_c2_cb, APDU_ACT_TX_CAPDU_TO_CARD);

	APDU_SEGMENT_IN2(get_data_c2_ca, APDU_ACT_TX_CAPDU_TO_CARD);
	APDU_SEGMENT_IN2(get_data_c2_cb, APDU_ACT_TX_CAPDU_TO_CARD);
}

void test_apdu_dispatch_context(void)
{
	struct osmo_apdu_context ac, prev_ac;
	int rc;

	printf("Testing GET DATA / 0xCA with SW 6Cxx\n");
	memset(&ac, 0, sizeof(ac));
	memset(&prev_ac, 0, sizeof(prev_ac));
	rc = osmo_apdu_segment_in2(&ac, &prev_ac, get_data_c2_ca, ARRAY_SIZE(get_data_c2_ca), 1);
	OSMO_ASSERT(rc == APDU_ACT_TX_CAPDU_TO_CARD);
	OSMO_ASSERT(ac.apdu_case == 2)
	ac.sw[0] = 0x6c;
	ac.sw[1] = 0x23;
	rc = osmo_apdu_segment_in2(&ac, &prev_ac, get_data_c2_ca_le_23, ARRAY_SIZE(get_data_c2_ca_le_23), 1);
	OSMO_ASSERT(rc == APDU_ACT_TX_CAPDU_TO_CARD);
	OSMO_ASSERT(ac.apdu_case == 2)

	printf("Testing GET DATA / 0xCB with SW 6Cxx\n");
	memset(&ac, 0, sizeof(ac));
	memset(&prev_ac, 0, sizeof(prev_ac));
	rc = osmo_apdu_segment_in2(&ac, &prev_ac, get_data_c2_cb, ARRAY_SIZE(get_data_c2_cb), 1);
	OSMO_ASSERT(rc == APDU_ACT_TX_CAPDU_TO_CARD);
	OSMO_ASSERT(ac.apdu_case == 2)
	ac.sw[0] = 0x6c;
	ac.sw[1] = 0x23;
	rc = osmo_apdu_segment_in2(&ac, &prev_ac, get_data_c2_cb_le_23, ARRAY_SIZE(get_data_c2_cb_le_23), 1);
	OSMO_ASSERT(rc == APDU_ACT_TX_CAPDU_TO_CARD);
	OSMO_ASSERT(ac.apdu_case == 2)
}

int main(int argc, char **argv)
{
	test_apdu_dispatch_simple();
	test_apdu_dispatch_context();

	printf("All tests passed.\n");
	return 0;
}
