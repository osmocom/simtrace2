/* Code to switch between local (physical) and remote (emulated) SIM */

#include "board.h"
#include "trace.h"
#include "sim_switch.h"

#ifdef PIN_SIM_SWITCH1
static const Pin pin_conn_usim1 = {PIO_PA20, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
#endif
#ifdef PIN_SIM_SWITCH2
static const Pin pin_conn_usim2 = {PIO_PA28, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
#endif

static int initialized = 0;

int sim_switch_use_physical(unsigned int nr, int physical)
{
	const Pin *pin;

	if (!initialized) {
		TRACE_ERROR("Somebody forgot to call sim_switch_init()\r\n");
		sim_switch_init();
	}

	TRACE_INFO("Modem %d: %s SIM\n\r", nr,
		   physical ? "physical" : "virtual");

	switch (nr) {
#ifdef PIN_SIM_SWITCH1
	case 1:
		pin = &pin_conn_usim1;
		break;
#endif
#ifdef PIN_SIM_SWITCH2
	case 2:
		pin = &pin_conn_usim2;
		break;
#endif
	default:
		TRACE_ERROR("Invalid SIM%u\n\r", nr);
		return -1;
	}

	if (physical)
		PIO_Clear(pin);
	else
		PIO_Set(pin);

	return 0;
}

int sim_switch_init(void)
{
	int num_switch = 0;
#ifdef PIN_SIM_SWITCH1
	PIO_Configure(&pin_conn_usim1, 1);
	num_switch++;
#endif
#ifdef PIN_SIM_SWITCH2
	PIO_Configure(&pin_conn_usim2, 1);
	num_switch++;
#endif
	return num_switch;
}
