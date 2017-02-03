/* Code to control the PERST lines of attached modems
 *
 * Depending on the board this is running on,  it might be possible
 * for the controller to set the status of the PERST input line of
 * the cellular modem.  If the board supports this, it sets the
 * PIN_PERST1 and/or PIN_PERST2 defines in its board.h file.
 */

#include "board.h"
#include "wwan_perst.h"

#ifdef PIN_PERST1
static const Pin pin_perst1 = PIN_PERST1;
#endif

#ifdef PIN_PERST2
static const Pin pin_perst2 = PIN_PERST2;
#endif

int wwan_perst_do_reset(int modem_nr)
{
	static const Pin *pin;

	switch (modem_nr) {
#ifdef PIN_PERST1
	case 1:
		pin = &pin_perst1;
		break;
#endif
#ifdef PIN_PERST2
	case 2:
		pin = &pin_perst2;
		break;
#endif
	default:
		return -1;
	}
	PIO_Clear(pin);
	mdelay(1);
	PIO_Set(pin);
	return 0;
}

int wwan_perst_init(void)
{
	int num_perst = 0;
#ifdef PIN_PERST1
	PIO_Configure(&pin_perst1, 1);
	num_perst++;
#endif

#ifdef PIN_PERST2
	PIO_Configure(&pin_perst2, 1);
	num_perst++;
#endif
	return num_perst;
}
