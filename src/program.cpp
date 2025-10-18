#include "program.h"
#include "GpioPin.h"
#include "main.h"
#include "c64_interface.h"
#include <stdio.h>
#include "circular_buffer.h"
#include "stm32f4xx_it.h"

GpioPin AtnPin(ATN_GPIO_Port, ATN_Pin);
GpioPin ResetPin(RESET_GPIO_Port, RESET_Pin);
GpioPin ClkPin(CLK_GPIO_Port, CLK_Pin);
GpioPin DataPin(DAT_GPIO_Port, DAT_Pin);

GpioPin LED(LED_GPIO_Port, LED_Pin);

#define PRINTER_WIDTH 80
#define PRINTER_HEIGHT 66
#define PRINTER_HEADER_LINES 5 // The amount of blank lines at the start of a new page

CircularBuffer<uint8_t, 30000> rx_buffer;

void setBusy(bool state)
{
	if (state)
	{
		HAL_GPIO_WritePin(Busy_GPIO_Port, Busy_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	}else {
		HAL_GPIO_WritePin(Busy_GPIO_Port, Busy_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
	}
}

#define ERROR_WAIT_TIME 50
#define TIMEOUT_TIME 50000 // 50 microseconds
#define TIMEOUT_RESET (htim7.Instance->CNT = 0)
#define IS_TIMEOUT (htim7.Instance->CNT >= TIMEOUT_TIME)
uint8_t temp_buffer[1000];
IEC iec;
C64_Basic printer;
/*
void readCMD()
{
	uint32_t len = 0;
	uint8_t data;

	// First byte indicates # of bytes being sent
	if (rx_buffer.read(&data) == false)
		return; // Should not happen

	if (data == 0 || data > 4)
		return; // invalid

	// Read length
	int n = data;
	for (int i = 0; i < n; i++)
	{
		TIMEOUT_RESET;
		while (rx_buffer.read(&data) == false)
		{
			if (IS_TIMEOUT)
				return;
		}

		if (data == 0) // delimiter byte
			return;

		len = (len << 8) | data;
	}

	// Read final byte, expect delimiter
	TIMEOUT_RESET;
	while (rx_buffer.read(&data) == false)
	{
		if (IS_TIMEOUT)
			return;
	}
	if (data != 0)
		return;

	// Verify data isn't too big
	if (len > 1000)
		return;

	// Echo back the length to confirm
	setBusy(true);
	temp_buffer[0] = n;
	for (int i = 0; i < n; i++)
	{
		int rsh = ((n - 1) - i) * 8;
		temp_buffer[i + 1] = (len >> rsh) & 0xFF;
	}
	temp_buffer[n + 1] = 0;
	HAL_UART_Transmit(&huart3, temp_buffer, n + 2, 50);

	// Now receive data
	while (len > 0)
	{
		len--;

		// Read a single byte
		TIMEOUT_RESET;
		while (rx_buffer.read(&data) == false)
		{
			if (IS_TIMEOUT)
			{
				setBusy(false);
				return;
			}
		}
		if (data == 0)
		{
			setBusy(false);
			return;
		}
	}

	// Once all data is receive, send it to the printer
	printer.send(temp_buffer, len);
	setBusy(false);
}
*/

void run()
{
	iec = IEC(ResetPin, AtnPin, ClkPin, DataPin);
	printer = C64_Basic(&iec);

	iec.reset();

	uint32_t count = 0;
	uint8_t data;
	while (1)
	{
		/*if (rx_buffer.available() > 0)
		{
			readCMD();
			HAL_Delay(ERROR_WAIT_TIME);
		}*/
		if (rx_buffer.read(&data))
		{
			if (data != 0)
			{
				temp_buffer[count] = data;
				count++;
				setBusy(true);
			} else if (count > 0) {
				printer.send(temp_buffer, count);
				count = 0;
				// BUSY signal must be active for a minimum time
				HAL_Delay(100);
				setBusy(false);
			}
		}
	}
}

void receiveUsartByte(uint8_t data)
{
	rx_buffer.put(data);
}

void USART3_IRQ_Handler(void)
{
	  uint32_t isrflags   = READ_REG(huart3.Instance->SR);
	  uint32_t cr1its     = READ_REG(huart3.Instance->CR1);
	  uint32_t errorflags = 0x00U;

	  /* If no error occurs */
	  errorflags = (isrflags & (uint32_t)(USART_SR_PE | USART_SR_FE | USART_SR_ORE | USART_SR_NE));
	  if (errorflags == RESET)
	  {
	    /* UART in mode Receiver -------------------------------------------------*/
	    if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
	    {
	        uint8_t data = (uint8_t)(huart3.Instance->DR & (uint8_t)0x00FF);
	        receiveUsartByte(data);
	      return;
	    }
	  }
}
