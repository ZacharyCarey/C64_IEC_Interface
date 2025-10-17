#ifndef C64_INTERFACE_H
#define C64_INTERFACE_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include "iec_driver.h"

#define CMD_GLOBAL(CMD) (CMD & 0x1F)
#define CMD_LISTEN(ADDR) (0x20 | (ADDR & 0x1F))
#define CMD_SECOND(ADDR) (0x60 | (ADDR & 0x1F))
#define CMD_UNLISTEN 0x3F
#define CMD_TALK(ADDR) (0x40 | (ADDR & 0x1F))
#define CMD_UNTALK 0x5F
#define CMD_CLOSE(ADDR) (0xE0 | (ADDR & 0x1F))
#define CMD_OPEN(ADDR) (0xF0 | (ADDR & 0x1F))

const uint8_t newline_char = 13;
void send(const uint8_t* data, uint16_t len)
{
	iec_command(true);
	iec_send((0x20 | (4 & 0x1F)), false); // printer addr
	iec_send((0x60 | (0 & 0x1F)), false); // Printer mode
	iec_command(false);

	while(len > 0)
	{
		iec_send(*data, false);
		data++;
		len--;
	}
	iec_send(newline_char, true);

	iec_command(true);
	iec_send(0x3F, false);
	iec_command(false);
	iec_end();
}

void run()
{
	iec_init();
	iec_reset();

	send("LINE 1", 6);
	send("LINE 2", 6);
	send("LINE 3", 6);
}

#endif
