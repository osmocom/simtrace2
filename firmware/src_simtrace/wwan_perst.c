/* Code to control the PERST lines of attached modems
 *
 * Depending on the board this is running on,  it might be possible
 * for the controller to set the status of the PERST input line of
 * the cellular modem.  If the board supports this, it sets the
 * PIN_PERST1 and/or PIN_PERST2 defines in its board.h file.
 */

#include "board.h"
#include "wwan_perst.h"
#include "osmocom/core/timer.h"

#define PERST_DURATION_MS 300

#ifdef PIN_PERST1
static const Pin pin_perst1 = PIN_PERST1;
static struct osmo_timer_list perst1_timer;
#endif

#ifdef PIN_PERST2
static const Pin pin_perst2 = PIN_PERST2;
static struct osmo_timer_list perst2_timer;
#endif

static void perst_tmr_cb(void *data)
{
	const Pin *pin = data;
	/* release the (low-active) reset */
	PIO_Set(pin);
}

int wwan_perst_do_reset(int modem_nr)
{
	const Pin *pin;
	struct osmo_timer_list *tmr;

	switch (modem_nr) {
#ifdef PIN_PERST1
	case 1:
		pin = &pin_perst1;
		tmr = &perst1_timer;
		break;
#endif
#ifdef PIN_PERST2
	case 2:
		pin = &pin_perst2;
		tmr = &perst2_timer;
		break;
#endif
	default:
		return -1;
	}
	PIO_Clear(pin);
	osmo_timer_schedule(tmr, PERST_DURATION_MS/1000, (PERST_DURATION_MS%1000)*1000);

	return 0;
}

int wwan_perst_init(void)
{
	int num_perst = 0;
#ifdef PIN_PERST1
	PIO_Configure(&pin_perst1, 1);
	perst1_timer.cb = perst_tmr_cb;
	perst1_timer.data = (void *) &pin_perst1;
	num_perst++;
#endif

#ifdef PIN_PERST2
	PIO_Configure(&pin_perst2, 1);
	perst2_timer.cb = perst_tmr_cb;
	perst2_timer.data = (void *) &pin_perst2;
	num_perst++;
#endif
	return num_perst;
}
