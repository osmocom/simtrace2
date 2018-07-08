/* This program is free software; you can redistribute it and/or modify
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
#pragma once

enum led {
	LED_RED,
	LED_GREEN,
	_NUM_LED
};

enum led_pattern {
	BLINK_ALWAYS_OFF	= 0,
	BLINK_ALWAYS_ON		= 1,
	BLINK_3O_5F		= 2,
	BLINK_3O_30F		= 3,
	BLINK_3O_1F_3O_30F	= 4,
	BLINK_3O_1F_3O_1F_3O_30F= 5,
	BLINK_2O_F		= 6,
	BLINK_200O_F		= 7,
	BLINK_600O_F		= 8,
	BLINK_CUSTOM		= 9,
	BLINK_2F_O,
	_NUM_LED_BLINK
};

void led_init(void);
void led_fini(void);
void led_stop(void);
void led_start(void);

void led_blink(enum led led, enum led_pattern blink);
enum led_pattern led_get(enum led led);
