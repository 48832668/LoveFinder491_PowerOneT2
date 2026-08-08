/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32g0xx_hal.h"

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
#define MUX_SELECT_Pin GPIO_PIN_15
#define MUX_SELECT_GPIO_Port GPIOC
#define MUX_EN_Pin GPIO_PIN_0
#define MUX_EN_GPIO_Port GPIOA
#define ADC_TEMP_Pin GPIO_PIN_1
#define ADC_TEMP_GPIO_Port GPIOA
#define LCD_DC_Pin GPIO_PIN_2
#define LCD_DC_GPIO_Port GPIOA
#define LCD_EN_Pin GPIO_PIN_3
#define LCD_EN_GPIO_Port GPIOA
#define LCD_CS_Pin GPIO_PIN_4
#define LCD_CS_GPIO_Port GPIOA
#define LCD_SCK_Pin GPIO_PIN_5
#define LCD_SCK_GPIO_Port GPIOA
#define LCD_RESET_Pin GPIO_PIN_6
#define LCD_RESET_GPIO_Port GPIOA
#define LCD_MOSI_Pin GPIO_PIN_7
#define LCD_MOSI_GPIO_Port GPIOA
#define SYS_IIC3_SDA_Pin GPIO_PIN_1
#define SYS_IIC3_SDA_GPIO_Port GPIOB
#define SYS_IIC2_SCL_Pin GPIO_PIN_11
#define SYS_IIC2_SCL_GPIO_Port GPIOA
#define SYS_IIC2_SDA_Pin GPIO_PIN_12
#define SYS_IIC2_SDA_GPIO_Port GPIOA
#define SYS_IIC3_SCL_Pin GPIO_PIN_3
#define SYS_IIC3_SCL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

// MUX_SELECT (PC15) and MUX_EN (PA0) are no longer used after removing hardware MUX.
// C1 uses bit-bang I2C on PB1(SDA)/PB3(SCL) instead.
// Defines are retained for CubeMX compatibility; remove from .ioc if desired.

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
