#include "program.h"
#include "GpioPin.h"
#include "main.h"
#include "c64_interface.h"
#include <stdio.h>

GpioPin AtnPin(ATN_GPIO_Port, ATN_Pin);
GpioPin ResetPin(RESET_GPIO_Port, RESET_Pin);
GpioPin ClkPin(CLK_GPIO_Port, CLK_Pin);
GpioPin DataPin(DAT_GPIO_Port, DAT_Pin);

GpioPin LED(LED_GPIO_Port, LED_Pin);

#define PRINTER_WIDTH 80
#define PRINTER_HEIGHT 66
#define PRINTER_HEADER_LINES 5 // The amount of blank lines at the start of a new page

void run()
{
	//led();
	LED.setPull(GpioPull::None);
	LED.setMode(GpioMode::Output);
	LED.setOpenDrain(false);
	LED.setAlternateFunction(0);
	LED.setSpeed(GpioSpeed::Low);
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

	IEC iec(ResetPin, AtnPin, ClkPin, DataPin);
	C64_Basic printer(&iec);

	iec.reset();
	/*printer.send("LINE 1", 6);
	printer.send("1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890",100);
	printer.send("LINE 2", 6);
	printer.send("LINE 3", 6);*/
	char buf[50];
	for (int i = 0; i < 300; i++)
	{
		int len = snprintf(buf, 50, "LINE %d", i);
		printer.send(buf, len);
	}
}
