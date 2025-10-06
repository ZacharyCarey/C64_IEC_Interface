/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CLK_Pin GPIO_PIN_10
#define CLK_GPIO_Port GPIOB
#define DATA_Pin GPIO_PIN_11
#define DATA_GPIO_Port GPIOB
#define ATN_Pin GPIO_PIN_12
#define ATN_GPIO_Port GPIOB
#define Reset_Pin GPIO_PIN_13
#define Reset_GPIO_Port GPIOB
#define SRQ_Pin GPIO_PIN_14
#define SRQ_GPIO_Port GPIOB
#define LED_Blue_Pin GPIO_PIN_8
#define LED_Blue_GPIO_Port GPIOC
#define LED_Green_Pin GPIO_PIN_9
#define LED_Green_GPIO_Port GPIOC

/* USER CODE BEGIN Private defines */
extern TIM_HandleTypeDef htim7;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
