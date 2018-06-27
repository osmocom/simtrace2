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
	_NUM_LED_BLINK
};

void led_init(void);
void led_fini(void);
void led_stop(void);
void led_start(void);

void led_blink(enum led led, enum led_pattern blink);
enum led_pattern led_get(enum led led);
