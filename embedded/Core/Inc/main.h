/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32l0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cmsis_os.h"
#include "string.h"
#include "sim800l.h"
#include "gps_reader.h"
#include "lcd5110.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern float battery_voltage;
extern uint8_t battery_voltage_str[];
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
extern const uint8_t BIKENAME[];
extern const uint8_t SERVER_LICENSE[];
extern const uint8_t SERVER[];
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
extern void GUI_BOOT_REGISTER_NET();
extern void GUI_BOOT_TCP_CONNECT();
extern void GUI_BOOT_TCP_FAIL();
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define BAT_ADC_Pin GPIO_PIN_0
#define BAT_ADC_GPIO_Port GPIOA
#define POS_LED_Pin GPIO_PIN_0
#define POS_LED_GPIO_Port GPIOB
#define GPS_PPS_Pin GPIO_PIN_8
#define GPS_PPS_GPIO_Port GPIOA
#define LCD_SCK_Pin GPIO_PIN_15
#define LCD_SCK_GPIO_Port GPIOA
#define LCD_DIN_Pin GPIO_PIN_3
#define LCD_DIN_GPIO_Port GPIOB
#define LCD_RST_Pin GPIO_PIN_4
#define LCD_RST_GPIO_Port GPIOB
#define LCD_DC_Pin GPIO_PIN_5
#define LCD_DC_GPIO_Port GPIOB
#define LCD_CE_Pin GPIO_PIN_6
#define LCD_CE_GPIO_Port GPIOB
#define LCD_BK_Pin GPIO_PIN_7
#define LCD_BK_GPIO_Port GPIOB
#define SIM_WKUP_Pin GPIO_PIN_8
#define SIM_WKUP_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
