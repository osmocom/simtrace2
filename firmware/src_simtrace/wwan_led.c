/* Code to read/track the status of the WWAN LEDs of attached modems
 *
 * Depending on the board this is running on,  it might be possible
 * for the controller to read the status of the WWAN LED output lines of
 * the cellular modem.  If the board supports this, it sets the
 * PIN_WWAN1 and/or PIN_WWAN2 defines in its board.h file.
 */

#include "board.h"
#include "wwan_led.h"

#ifdef PIN_WWAN1
static const Pin pin_wwan1 = PIN_WWAN1;

static void wwan1_irqhandler(const Pin *pPin)
{
	int active = wwan_led_active(1);

	TRACE_INFO("WWAN1 LED %u\r\n", active);

	/* TODO: notify host via USB */
}
#endif

#ifdef PIN_WWAN2
static const Pin pin_wwan2 = PIN_WWAN2;

static void wwan2_irqhandler(const Pin *pPin)
{
	int active = wwan_led_active(2);
	TRACE_INFO("WWAN2 LED %u\r\n", active);

	/* TODO: notify host via USB */
}
#endif

/* determine if a tiven WWAN led is currently active or not */
int wwan_led_active(int wwan)
{
	const Pin *pin;
	int active;

	switch (wwan) {
#ifdef PIN_WWAN1
	case 1:
		pin = &pin_wwan1;
		break;
#endif
#ifdef PIN_WWAN2
	case 2:
		pin = &pin_wwan2;
		break;
#endif
	default:
		return -1;
	}

	active = PIO_Get(&pin_wwan1) ? 0 : 1;
	return active;
}

int wwan_led_init(void)
{
	int num_leds = 0;

#ifdef PIN_WWAN1
	PIO_Configure(&pin_wwan1, 1);
	PIO_ConfigureIt(&pin_wwan1, wwan1_irqhandler);
	PIO_EnableIt(&pin_wwan1);
	num_leds++;
#endif

#ifdef PIN_WWAN2
	PIO_Configure(&pin_wwan2, 1);
	PIO_ConfigureIt(&pin_wwan2, wwan2_irqhandler);
	PIO_EnableIt(&pin_wwan2);
	num_leds++;
#endif
	return num_leds;
}
