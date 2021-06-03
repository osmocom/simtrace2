#include "board.h"
#include <stdbool.h>
#include "i2c.h"
#include "mcp23017.h"


//defines from https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library/blob/master/Adafruit_MCP23017.h under BSD license

// registers
#define MCP23017_IODIRA 0x00
#define MCP23017_IPOLA 0x02
#define MCP23017_GPINTENA 0x04
#define MCP23017_DEFVALA 0x06
#define MCP23017_INTCONA 0x08
#define MCP23017_IOCONA 0x0A
#define MCP23017_GPPUA 0x0C
#define MCP23017_INTFA 0x0E
#define MCP23017_INTCAPA 0x10
#define MCP23017_GPIOA 0x12
#define MCP23017_OLATA 0x14


#define MCP23017_IODIRB 0x01
#define MCP23017_IPOLB 0x03
#define MCP23017_GPINTENB 0x05
#define MCP23017_DEFVALB 0x07
#define MCP23017_INTCONB 0x09
#define MCP23017_IOCONB 0x0B
#define MCP23017_GPPUB 0x0D
#define MCP23017_INTFB 0x0F
#define MCP23017_INTCAPB 0x11
#define MCP23017_GPIOB 0x13
#define MCP23017_OLATB 0x15

#define MCP23017_INT_ERR 255


//bool i2c_write_byte(bool send_start, bool send_stop, uint8_t byte)
//uint8_t i2c_read_byte(bool nack, bool send_stop)
//static void i2c_stop_cond(void)

int mcp23017_write_byte(uint8_t slave, uint8_t addr, uint8_t byte)
{
	bool nack;

	WDT_Restart(WDT);

//	 Write slave address 
	nack = i2c_write_byte(true, false, slave << 1);
	if (nack)
		goto out_stop;
	nack = i2c_write_byte(false, false, addr);
	if (nack)
		goto out_stop;
	nack = i2c_write_byte(false, true, byte);
	if (nack)
		goto out_stop;

out_stop:
	i2c_stop_cond();
	if (nack)
		return -1;
	else
		return 0;
}

int mcp23017_read_byte(uint8_t slave, uint8_t addr)
{
	bool nack;

	WDT_Restart(WDT);

	// dummy write cycle
	nack = i2c_write_byte(true, false, slave << 1);
	if (nack)
		goto out_stop;
	nack = i2c_write_byte(false, false, addr);
	if (nack)
		goto out_stop;
	// Re-start with read
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

int mcp23017_init(uint8_t slave, uint8_t iodira, uint8_t iodirb)
{
	TRACE_DEBUG("mcp23017_init\n\r");

	// all gpio input
	if (mcp23017_write_byte(slave, MCP23017_IODIRA, iodira))
		goto out_err;
	// msb of portb output, rest input
	if (mcp23017_write_byte(slave, MCP23017_IODIRB, iodirb))
		goto out_err;
	if (mcp23017_write_byte(slave, MCP23017_IOCONA, 0x20)) //disable SEQOP (autoinc addressing)
		goto out_err;

	TRACE_DEBUG("mcp23017 found\n\r");
	return 0;

out_err:
	TRACE_WARNING("mcp23017 NOT found!\n\r");
	return -1;
}

int mcp23017_test(uint8_t slave)
{
	printf("mcp23017_test\n\r");
	printf("GPIOA 0x%x\n\r", mcp23017_read_byte(slave,MCP23017_GPIOA));
	printf("GPIOB 0x%x\n\r", mcp23017_read_byte(slave,MCP23017_GPIOB));
	printf("IODIRA 0x%x\n\r", mcp23017_read_byte(slave,MCP23017_IODIRA));
	printf("IODIRB 0x%x\n\r", mcp23017_read_byte(slave,MCP23017_IODIRB));
	printf("IOCONA 0x%x\n\r", mcp23017_read_byte(slave,MCP23017_IOCONA));
	printf("IOCONB 0x%x\n\r", mcp23017_read_byte(slave,MCP23017_IOCONB));

	return 0;
}

int mcp23017_set_output_a(uint8_t slave, uint8_t val)
{
	return mcp23017_write_byte(slave, MCP23017_OLATA, val);
}

int mcp23017_set_output_b(uint8_t slave, uint8_t val)
{
	return mcp23017_write_byte(slave, MCP23017_OLATB, val);
}

int mcp23017_toggle(uint8_t slave)
{
	// example writing MSB of gpio
	static bool foo=false;
	if (foo)
	{
		printf("+\n\r");
		mcp23017_write_byte(slave, MCP23017_OLATB, 0x80);
		foo=false;
	}
	else
	{
		printf("-\n\r");
		mcp23017_write_byte(slave, MCP23017_OLATB, 0x00);
		foo=true;
	}
	return 0;
}
