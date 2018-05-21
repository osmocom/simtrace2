#include "board.h"
#include "boardver_adc.h"

/***********************************************************************
 * ADC for board version detection
 ***********************************************************************/

#ifdef PIN_VERSION_DET

static int adc_sam3s_reva_errata = 0;

static const Pin pin_version_det = PIN_VERSION_DET;

/* Warning: Don't call this while other code (like the SIM VCC voltage
 * reading) is running.  The idea is you call this once during board
 * startup and cache the result in a (global) variable if you need it
 * later throughout the code */
int get_board_version_adc(void)
{
	uint32_t chip_arch = CHIPID->CHIPID_CIDR & CHIPID_CIDR_ARCH_Msk;
	uint32_t chip_ver = CHIPID->CHIPID_CIDR & CHIPID_CIDR_VERSION_Msk;
	uint16_t sample;
	uint32_t uv;

	PIO_Configure(&pin_version_det, 1);

	PMC_EnablePeripheral(ID_ADC);

	ADC->ADC_CR |= ADC_CR_SWRST;
	if (chip_ver == 0 &&
	    (chip_arch == CHIPID_CIDR_ARCH_SAM3SxA ||
	     chip_arch == CHIPID_CIDR_ARCH_SAM3SxB ||
	     chip_arch == CHIPID_CIDR_ARCH_SAM3SxC)) {
		TRACE_INFO("Enabling Rev.A ADC Errata work-around\r\n");
		adc_sam3s_reva_errata = 1;
	}

	if (adc_sam3s_reva_errata) {
		/* Errata Work-Around to clear EOCx flags */
		volatile uint32_t foo;
		int i;
		for (i = 0; i < 16; i++)
			foo = ADC->ADC_CDR[i];
	}

	/* Initialize ADC for AD2, fADC=48/24=2MHz */
	ADC->ADC_MR = ADC_MR_TRGEN_DIS | ADC_MR_LOWRES_BITS_12 |
		      ADC_MR_SLEEP_NORMAL | ADC_MR_FWUP_OFF |
		      ADC_MR_FREERUN_OFF | ADC_MR_PRESCAL(23) |
		      ADC_MR_STARTUP_SUT8 | ADC_MR_SETTLING(3) |
		      ADC_MR_ANACH_NONE | ADC_MR_TRACKTIM(4) |
		      ADC_MR_TRANSFER(1) | ADC_MR_USEQ_NUM_ORDER;
	/* enable AD2 channel only */
	ADC->ADC_CHER = ADC_CHER_CH2;

	/* Make sure we don't use interrupts as that's what the SIM card
	 * VCC ADC code is using */
	ADC->ADC_IER = 0;
	NVIC_DisableIRQ(ADC_IRQn);

	ADC->ADC_CR |= ADC_CR_START;

	/* busy-wait, actually read the value */
	do { } while (!(ADC->ADC_ISR & ADC_ISR_EOC2));
	/* convert to voltage */
	sample = ADC->ADC_CDR[2];
	uv = adc2uv(sample);
	TRACE_INFO("VERSION_DET ADC=%u => %u uV\r\n", sample, uv);

	/* FIXME: convert to board version based on thresholds */

	return 0;
}

#endif /* PIN_VERSION_DET */
