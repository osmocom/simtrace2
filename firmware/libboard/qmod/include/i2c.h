#pragma once

void i2c_pin_init(void);
int eeprom_write_byte(uint8_t slave, uint8_t addr, uint8_t byte);
int eeprom_read_byte(uint8_t slave, uint8_t addr);
