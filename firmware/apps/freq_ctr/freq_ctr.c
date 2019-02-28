#include <stdint.h>
#include "utils.h"
#include "tc_etu.h"
#include "chip.h"


/* pins for Channel 0 of TC-block 0 */
#define PIN_TIOA0	{PIO_PA0, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}

/* pins for Channel 1 of TC-block 0 */
#define PIN_TIOA1	{PIO_PA15, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
#define PIN_TCLK1	{PIO_PA28, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}

static const Pin pins_tc[] = { PIN_TIOA0, PIN_TIOA1, PIN_TCLK1 };

static TcChannel *tc1 = &TC0->TC_CHANNEL[1];

void TC1_IrqHandler(void)
{
	uint32_t sr = tc1->TC_SR;
	printf("TC1=%lu; SR=0x%08lx\r\n", tc1->TC_RA, sr);
}

void freq_ctr_init(void)
{
	TcChannel *tc0 = &TC0->TC_CHANNEL[0];

	PIO_Configure(pins_tc, ARRAY_SIZE(pins_tc));

	PMC_EnablePeripheral(ID_TC0);
	PMC_EnablePeripheral(ID_TC1);

	/* route TCLK1 to XC1 */
	TC0->TC_BMR &= ~TC_BMR_TC1XC1S_Msk;
	TC0->TC_BMR |= TC_BMR_TC1XC1S_TCLK1;

	/* TC0 in wveform mode: Run from SCLK. Raise TIOA on RA; lower TIOA on RC + trigger */
	tc0->TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_BURST_NONE |
		      TC_CMR_EEVTEDG_NONE | TC_CMR_WAVSEL_UP_RC | TC_CMR_WAVE |
		      TC_CMR_ACPA_SET | TC_CMR_ACPC_CLEAR;
	tc0->TC_RA = 16384; /* set high at 16384 */
	tc0->TC_RC = 32786; /* set low at 32786 */

	/* TC1 in capture mode: Run from XC1. Trigger on TIOA rising. Load RA on rising */
	tc1->TC_CMR = TC_CMR_TCCLKS_XC1 | TC_CMR_BURST_NONE |
		      TC_CMR_ETRGEDG_RISING | TC_CMR_ABETRG | TC_CMR_LDRA_RISING;
	/* Interrupt us if the external trigger happens */
	tc1->TC_IER = TC_IER_ETRGS;
	NVIC_EnableIRQ(TC1_IRQn);

	TC0->TC_BCR = TC_BCR_SYNC;

	tc0->TC_CCR = TC_CCR_CLKEN|TC_CCR_SWTRG;
	tc1->TC_CCR = TC_CCR_CLKEN|TC_CCR_SWTRG;
}
