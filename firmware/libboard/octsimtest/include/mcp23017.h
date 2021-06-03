/* mcp23017 i2c gpio expander read and write utilities
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
#pragma once

#define MCP23017_ADDRESS 0x20

int mcp23017_init(uint8_t slave, uint8_t iodira, uint8_t iodirb);
int mcp23017_test(uint8_t slave);
int mcp23017_toggle(uint8_t slave);
int mcp23017_set_output_a(uint8_t slave, uint8_t val);
int mcp23017_set_output_b(uint8_t slave, uint8_t val);
//int mcp23017_write_byte(uint8_t slave, uint8_t addr, uint8_t byte);
//int mcp23017_read_byte(uint8_t slave, uint8_t addr);
