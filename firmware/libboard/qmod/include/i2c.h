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
 */
#pragma once

void i2c_pin_init(void);
int eeprom_write_byte(uint8_t slave, uint8_t addr, uint8_t byte);
int eeprom_read_byte(uint8_t slave, uint8_t addr);
