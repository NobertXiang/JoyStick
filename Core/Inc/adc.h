/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.h
  * @brief   This file contains all the function prototypes for
  *          the adc.c file
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
#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern ADC_HandleTypeDef hadc1;

/* USER CODE BEGIN Private defines */
#define JOYSTICK_ADC_CHANNEL_COUNT  6U
#define JOYSTICK_HID_REPORT_SIZE    12U
#define JOYSTICK_ADC_INPUT_MAX      3050U   /* CH0/1/4/6 实测满量程 */
#define JOYSTICK_OUTPUT_ANGLE_MAX   360U    /* HID 输出角度 0-360 */
/* USER CODE END Private defines */

void MX_ADC1_Init(void);

/* USER CODE BEGIN Prototypes */
extern uint16_t joystick_adc_raw[JOYSTICK_ADC_CHANNEL_COUNT];
extern uint8_t joystick_hid_report[JOYSTICK_HID_REPORT_SIZE];
HAL_StatusTypeDef Joystick_ADC_Start(void);
void Joystick_PackHidReport(void);
uint8_t Joystick_TransmitHidReport(void);
uint8_t Joystick_SendHidReport(void);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H__ */

