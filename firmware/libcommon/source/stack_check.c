#include <stdint.h>
#include <osmocom/core/panic.h>

/* This is what's minimally required to fix builds on Ubuntu 20.04,
 * where stack smashing protection is enabled by default when using dpkg
 * - even when cross-compiling: https://osmocom.org/issues/4687
 */

uintptr_t __stack_chk_guard = 0xdeadbeef;

void __stack_chk_fail(void)
{
	osmo_panic("Stack smashing detected!\r\n");
}
