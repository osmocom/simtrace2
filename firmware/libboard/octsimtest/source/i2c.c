/* I2C EEPROM memory read and write utilities
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA
 */
#include "board.h"
#include <stdbool.h>

/* Low-Level I2C Routines */

static const Pin pin_sda = {PIO_PA30, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_OPENDRAIN };
static const Pin pin_sda_in = {PIO_PA30, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT };
static const Pin pin_scl = {PIO_PA31, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_OPENDRAIN };

static void i2c_delay()
{
	volatile int v;
	int i;

	/* 100 cycles results in SCL peak length of 44us, so it's about
	 * 440ns per cycle here */
	for (i = 0; i < 14; i++) {
		v = 0;
	}
}

void i2c_pin_init(void)
{
	PIO_Configure(&pin_scl, PIO_LISTSIZE(pin_scl));
	PIO_Configure(&pin_sda, PIO_LISTSIZE(pin_sda));
}

static void set_scl(void)
{
	PIO_Set(&pin_scl);
	i2c_delay();
}

static void set_sda(void)
{
	PIO_Set(&pin_sda);
	i2c_delay();
}

static void clear_scl(void)
{
	PIO_Clear(&pin_scl);
	i2c_delay();
}

static void clear_sda(void)
{
	PIO_Clear(&pin_sda);
	i2c_delay();
}

static bool read_sda(void)
{
	bool ret;

	PIO_Configure(&pin_sda_in, PIO_LISTSIZE(pin_sda_in));
	if (PIO_Get(&pin_sda_in))
		ret = true;
	else
		ret = false;
	PIO_Configure(&pin_sda, PIO_LISTSIZE(pin_sda));

	return ret;
}

/* Core I2C Routines */

static bool i2c_started = false;

static void i2c_start_cond(void)
{
	if (i2c_started) {
		set_sda();
		set_scl();
	}

	clear_sda();
	i2c_delay();
	clear_scl();
	i2c_started = true;
}

void i2c_stop_cond(void)
{
	clear_sda();
	set_scl();
	set_sda();
	i2c_delay();
	i2c_started = false;
}

static void i2c_write_bit(bool bit)
{
	if (bit)
		set_sda();
	else
		clear_sda();
	i2c_delay(); // ?
	set_scl();
	clear_scl();
}

static bool i2c_read_bit(void)
{
	bool bit;

	set_sda();
	set_scl();
	bit = read_sda();
	clear_scl();

	return bit;
}

bool i2c_write_byte(bool send_start, bool send_stop, uint8_t byte)
{
	uint8_t bit;
	bool nack;

	if (send_start)
		i2c_start_cond();

	for (bit = 0; bit < 8; bit++) {
		i2c_write_bit((byte & 0x80) != 0);
		byte <<= 1;
	}

	nack = i2c_read_bit();

	if (send_stop)
		i2c_stop_cond();

	return nack;
}

uint8_t i2c_read_byte(bool nack, bool send_stop)
{
	uint8_t byte = 0;
	uint8_t bit;

	for (bit = 0; bit < 8; bit++) {
		byte = (byte << 1) | i2c_read_bit();
	}

	i2c_write_bit(nack);

	if (send_stop)
		i2c_stop_cond();

	return byte;
}


/* EEPROM related code */

int eeprom_write_byte(uint8_t slave, uint8_t addr, uint8_t byte)
{
	bool nack;

	WDT_Restart(WDT);

	/* Write slave address */
	nack = i2c_write_byte(true, false, slave << 1);
	if (nack)
		goto out_stop;
	nack = i2c_write_byte(false, false, addr);
	if (nack)
		goto out_stop;
	nack = i2c_write_byte(false, true, byte);
	if (nack)
		goto out_stop;
	/* Wait tWR time to ensure EEPROM is writing correctly (tWR = 5 ms for AT24C02) */
	mdelay(5);

out_stop:
	i2c_stop_cond();
	if (nack)
		return -1;
	else
		return 0;
}

int eeprom_read_byte(uint8_t slave, uint8_t addr)
{
	bool nack;

	WDT_Restart(WDT);

	/* dummy write cycle */
	nack = i2c_write_byte(true, false, slave << 1);
	if (nack)
		goto out_stop;
	nack = i2c_write_byte(false, false, addr);
	if (nack)
		goto out_stop;
	/* Re-start with read */
	nack = i2c_write_byte(true, false, (slave << 1) | 1);
	if (nack)
		goto out_stop;

	return i2c_read_byte(true, true);

out_stop:
	i2c_stop_cond();
	if (nack)
		return -1;
	else
		return 0;
}
