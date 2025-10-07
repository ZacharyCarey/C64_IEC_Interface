#ifndef TEST_H
#define TEST_H

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

uint8_t tx_buff[1000] = {0};
uint8_t rx_buff[1000] = {0};

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
	if (HAL_UART_Receive(&huart2, rx_buff, 3, /*HAL_MAX_DELAY*/1000) == HAL_OK)
	{
		if (rx_buff[0] != 0) return;
		uint16_t len = ((uint16_t)rx_buff[1] << 8) | rx_buff[2];
		if (len == 0) return;
		if (len > 1000)
		{
			// Exceeded our buffer size :(
			tx_buff[0] = 0;
			tx_buff[1] = 0;
			tx_buff[2] = 0;
			HAL_UART_Transmit(&huart2, tx_buff, 3, 100);
			return;
		}

		// Send confirmation back
		tx_buff[0] = 0;
		tx_buff[1] = (len >> 8);
		tx_buff[2] = (len & 0xFF);
		HAL_UART_Transmit(&huart2, tx_buff, 3, 100);

		// TODO speed can be improved by sending bytes over IEC while we wait for serial to send data (i.e. send bytes at the same time as receiving them)
		// Receive expected bytes
		if (HAL_UART_Receive(&huart2, rx_buff, len, 1000) != HAL_OK)
			return;

		// Print bytes to IEC
		send(rx_buff, len);
		HAL_Delay(2);
		tx_buff[0] = 0;
		tx_buff[1] = 0;
		HAL_UART_Transmit(&huart2, tx_buff, 2, 100); // Signal operation finished
	}
}


void init()
{
	iec_init();
	iec_reset();

	send("LINE 1", 6);
	send("LINE 2", 6);
	send("LINE 3", 6);
}

#endif
