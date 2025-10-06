#ifndef TEST_H
#define TEST_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include "iec_driver.h"


void run()
{

	/*
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
	delayMicroseconds(10);
	//HAL_Delay(1000);
	HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
	delayMicroseconds(15);
	//HAL_Delay(1000);
	HAL_GPIO_WritePin(CLK_GPIO_Port, CLK_Pin, GPIO_PIN_RESET);*/
	//HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
	//HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);

/*	uint16_t target = 25;
	__HAL_TIM_SET_COUNTER(&htim1, 0);
	for (int i = 0; i < 25; i++)
	{
		while (__HAL_TIM_GET_COUNTER(&htim1) < target);
		HAL_GPIO_TogglePin(CLK_GPIO_Port, CLK_Pin);
		target += 25;
	}


	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);*/
	//HAL_Delay(1000);
}

const uint16_t data_timer_thresh = 1056;
/*void sendBit(bool bit)
{
	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, bit ? 60000 : 1056);

	// Wait for timer to pass the data threshold
	while(__HAL_TIM_GET_COUNTER(&htim2) <= data_timer_thresh);

	// Wait for timer rollover
	while(__HAL_TIM_GET_COUNTER(&htim2) >= data_timer_thresh);
}*/

void init()
{
	iec_init();
	iec_reset();

	char data[7];

	for (int line = 0; line < 3; line++)
	{
		// Line (i)\r
		data[0] = 76; // L
		data[1] = 73; // I
		data[2] = 78; // N
		data[3] = 69; // E
		data[4] = 32; // space
		data[5] = 48 + line + 1; // i (1, 2, 3, etc)
		data[6] = 13; // \r

		iec_command(true);
		iec_send((0x20 | (4 & 0x1F)), false); // printer addr
		iec_send((0x60 | (0 & 0x1F)), false); // Printer mode
		iec_command(false);

		for (int i = 0; i < 7; i++)
		{
			iec_send(data[i], i == 6);
		}

		iec_command(true);
		iec_send(0x3F, false);
		iec_command(false);
		iec_end();

		HAL_Delay(2);
	}


	// Swap CLOCK to drive bus
/*	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = CLK_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
	HAL_GPIO_Init(CLK_GPIO_Port, &GPIO_InitStruct);
*/
/*
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
	htim2.Instance->CR1 &= ~1;


	HAL_GPIO_WritePin(Reset_GPIO_Port, Reset_Pin, GPIO_PIN_RESET);
	HAL_Delay(3000);
	HAL_GPIO_WritePin(Reset_GPIO_Port, Reset_Pin, GPIO_PIN_SET);
	HAL_Delay(1000);

	//__HAL_TIM_ENABLE(&htim1);
	HAL_GPIO_WritePin(Reset_GPIO_Port, ATN_Pin, GPIO_PIN_RESET);
	__HAL_TIM_SET_COUNTER(&htim2, 0);
	__HAL_TIM_ENABLE(&htim2);
	HAL_GPIO_WritePin(Reset_GPIO_Port, Reset_Pin, GPIO_PIN_RESET);
	sendBit(false);
	sendBit(false);
	sendBit(true);
	sendBit(true);
	sendBit(false);
	sendBit(true);
	sendBit(false);
	sendBit(true);

	// Wait for timers to finish/rollover
	//while(__HAL_TIM_GET_COUNTER(&htim2) >= data_timer_thresh);
	htim2.Instance->CR1 &= ~1;
	//HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
	//HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);
	HAL_GPIO_WritePin(Reset_GPIO_Port, ATN_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(Reset_GPIO_Port, Reset_Pin, GPIO_PIN_SET);
*/

	//HAL_NVIC_EnableIRQ(TIM2_IRQn);
	//__HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);



	/*while(1)
	{
		HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
		HAL_Delay(2000);
	}*/
}

#endif
