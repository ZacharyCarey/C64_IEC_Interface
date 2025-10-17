#include "program.h"
#include "GpioPin.h"
#include "main.h"
#include "c64_interface.h"

GpioPin AtnPin(ATN_GPIO_Port, ATN_Pin);
GpioPin ResetPin(RESET_GPIO_Port, RESET_Pin);
GpioPin ClkPin(CLK_GPIO_Port, CLK_Pin);
GpioPin DataPin(DAT_GPIO_Port, DAT_Pin);

GpioPin LED(LED_GPIO_Port, LED_Pin);


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
	printer.send("LINE 1", 6);
	printer.send("LINE 2", 6);
	printer.send("LINE 3", 6);
}
