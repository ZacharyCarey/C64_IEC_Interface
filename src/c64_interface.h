#ifndef C64_INTERFACE_H
#define C64_INTERFACE_H

#include "iec_driver.h"
#include <stdint.h>

#define CMD_GLOBAL(CMD) (CMD & 0x1F)
#define CMD_LISTEN(ADDR) (0x20 | (ADDR & 0x1F))
#define CMD_SECOND(ADDR) (0x60 | (ADDR & 0x1F))
#define CMD_UNLISTEN 0x3F
#define CMD_TALK(ADDR) (0x40 | (ADDR & 0x1F))
#define CMD_UNTALK 0x5F
#define CMD_CLOSE(ADDR) (0xE0 | (ADDR & 0x1F))
#define CMD_OPEN(ADDR) (0xF0 | (ADDR & 0x1F))

const uint8_t newline_char = 13;

class C64_Basic
{
public:
	C64_Basic(IEC* iec)
	{
		this->driver = iec;
	}

	void send(const void* data, uint16_t len)
	{
		const uint8_t* buf = (const uint8_t*)data;
		driver->command(true);
		driver->send((0x20 | (4 & 0x1F)), false); // printer addr
		driver->send((0x60 | (0 & 0x1F)), false); // Printer mode
		driver->command(false);

		while(len > 0)
		{
			driver->send(*buf, false);
			buf++;
			len--;
		}
		driver->send(newline_char, true);

		driver->command(true);
		driver->send(0x3F, false);
		driver->command(false);
		driver->end();
	}

private:
	IEC* driver;
};

#endif
