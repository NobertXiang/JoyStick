/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "joystick_rtos.h"
#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "usbd_def.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* Queue Handles */
osMessageQueueId_t adcQueueHandle = NULL;
osMessageQueueId_t encoderQueueHandle = NULL;
osMessageQueueId_t buttonQueueHandle = NULL;
osMessageQueueId_t ledQueueHandle = NULL;

/* Thread Handles */
osThreadId_t adcTaskHandle = NULL;
osThreadId_t encoderTaskHandle = NULL;
osThreadId_t buttonTaskHandle = NULL;
osThreadId_t ledTaskHandle = NULL;
osThreadId_t hidTaskHandle = NULL;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for ADC Task */
const osThreadAttr_t adcTask_attributes = {
  .name = "ADC",
  .stack_size = ADC_TASK_STACK_SIZE * 4,
  .priority = (osPriority_t) ADC_TASK_PRIORITY,
};

/* Definitions for Encoder Task */
const osThreadAttr_t encoderTask_attributes = {
  .name = "Encoder",
  .stack_size = ENCODER_TASK_STACK_SIZE * 4,
  .priority = (osPriority_t) ENCODER_TASK_PRIORITY,
};

/* Definitions for Button Task */
const osThreadAttr_t buttonTask_attributes = {
  .name = "Button",
  .stack_size = BUTTON_TASK_STACK_SIZE * 4,
  .priority = (osPriority_t) BUTTON_TASK_PRIORITY,
};

/* Definitions for LED Task */
const osThreadAttr_t ledTask_attributes = {
  .name = "LED",
  .stack_size = LED_TASK_STACK_SIZE * 4,
  .priority = (osPriority_t) LED_TASK_PRIORITY,
};

/* Definitions for HID Task */
const osThreadAttr_t hidTask_attributes = {
  .name = "HID",
  .stack_size = HID_TASK_STACK_SIZE * 4,
  .priority = (osPriority_t) HID_TASK_PRIORITY,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void AdcTask(void *argument);
void EncoderTask(void *argument);
void ButtonTask(void *argument);
void LedTask(void *argument);
void HidTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* Create queues */
  adcQueueHandle = osMessageQueueNew(4, sizeof(AdcData_t), NULL);
  encoderQueueHandle = osMessageQueueNew(4, sizeof(EncoderData_t), NULL);
  buttonQueueHandle = osMessageQueueNew(4, sizeof(ButtonData_t), NULL);
  ledQueueHandle = osMessageQueueNew(4, sizeof(LedData_t), NULL);
  if ((adcQueueHandle == NULL) || (encoderQueueHandle == NULL) ||
      (buttonQueueHandle == NULL) || (ledQueueHandle == NULL))
  {
    Error_Handler();
  }
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
  if (defaultTaskHandle == NULL)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN RTOS_THREADS */
  /* Create joystick tasks */
  adcTaskHandle = osThreadNew(AdcTask, NULL, &adcTask_attributes);
  encoderTaskHandle = osThreadNew(EncoderTask, NULL, &encoderTask_attributes);
  buttonTaskHandle = osThreadNew(ButtonTask, NULL, &buttonTask_attributes);
  ledTaskHandle = osThreadNew(LedTask, NULL, &ledTask_attributes);
  hidTaskHandle = osThreadNew(HidTask, NULL, &hidTask_attributes);
  /* 非关键任务创建失败不阻塞启动，USB 已在 main 中初始化 */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  for(;;)
  {
    HAL_GPIO_TogglePin(SysLED_GPIO_Port, SysLED_Pin);
    osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* ============================================================================
 * ADC Task Implementation
 * ========================================================================== */
/**
  * @brief  ADC Conversion Task
  * Continuously reads ADC values and sends them to the ADC queue
  */
void AdcTask(void *argument)
{
  AdcData_t adcData;
  uint8_t retry;

  for(;;)
  {
    osDelay(ADC_TASK_DELAY);

    for (uint8_t i = 0; i < 6; i++)
    {
      adcData.channels[i] = joystick_adc_raw[i];
    }

    Joystick_PackHidReport();

    for (retry = 0U; retry < 5U; retry++)
    {
      if (Joystick_TransmitHidReport() != USBD_BUSY)
      {
        break;
      }
      osDelay(1);
    }

    osMessageQueuePut(adcQueueHandle, &adcData, 0, 0);
  }
}

/* ============================================================================
 * Encoder Task Implementation
 * ========================================================================== */
/**
  * @brief  Encoder Reading Task
  * Reads all encoder counter values and sends them to the encoder queue
  */
void EncoderTask(void *argument)
{
  EncoderData_t encoderData;
  
  for(;;)
  {
    osDelay(ENCODER_TASK_DELAY);
    
    /* Read encoder count from TIM1, TIM2, TIM3, TIM4 */
    encoderData.encoders[0] = __HAL_TIM_GET_COUNTER(&htim1);
    encoderData.encoders[1] = __HAL_TIM_GET_COUNTER(&htim2);
    encoderData.encoders[2] = __HAL_TIM_GET_COUNTER(&htim3);
    encoderData.encoders[3] = __HAL_TIM_GET_COUNTER(&htim4);
    
    /* Send encoder data to queue */
    osMessageQueuePut(encoderQueueHandle, &encoderData, 0, 0);
  }
}

/* ============================================================================
 * Button Task Implementation
 * ========================================================================== */
/**
  * @brief  Button Reading Task
  * Reads button states from SPI2 connected 74HC165 shift registers
  */
void ButtonTask(void *argument)
{
  ButtonData_t buttonData;
  uint8_t rxBuffer[3] = {0};
  
  for(;;)
  {
    osDelay(BUTTON_TASK_DELAY);
    
    /* Read 3 bytes from SPI2 (74HC165 parallel-to-serial converter)
     * PB12 is the latch pin for SPI2
     */
    
    /* Latch the input data */
    HAL_GPIO_WritePin(SPI2_LATCH_GPIO_Port, SPI2_LATCH_Pin, GPIO_PIN_RESET);
    osDelay(1);
    HAL_GPIO_WritePin(SPI2_LATCH_GPIO_Port, SPI2_LATCH_Pin, GPIO_PIN_SET);
    osDelay(1);
    
    /* Read button states via SPI2 (3 bytes = 24 buttons) */
    if (HAL_SPI_Receive(&hspi2, rxBuffer, 3, 100) == HAL_OK)
    {
      buttonData.states[0] = rxBuffer[0];
      buttonData.states[1] = rxBuffer[1];
      buttonData.states[2] = rxBuffer[2];
      
      /* Send button data to queue */
      osMessageQueuePut(buttonQueueHandle, &buttonData, 0, 0);
    }
  }
}

/* ============================================================================
 * LED Task Implementation
 * ========================================================================== */
/**
  * @brief  LED Control Task
  * Receives LED control data from queue and drives 74HC595 shift registers via SPI1
  */
void LedTask(void *argument)
{
  LedData_t ledData;
  uint32_t flags;
  
  for(;;)
  {
    /* Wait for LED data from the queue (with timeout) */
    flags = osMessageQueueGet(ledQueueHandle, &ledData, NULL, osWaitForever);
    
    if (flags == osOK)
    {
      /* Write LED states via SPI1 (3 bytes = 24 LEDs)
       * PB2 is the latch pin (RCLK) for SPI1
       */
      
      /* Send 3 bytes via SPI1 to 74HC595 shift registers */
      if (HAL_SPI_Transmit(&hspi1, (uint8_t *)ledData.states, 3, 100) == HAL_OK)
      {
        /* Latch the data on 74HC595 */
        osDelay(1);
        HAL_GPIO_WritePin(SPI1_LATCH_GPIO_Port, SPI1_LATCH_Pin, GPIO_PIN_RESET);
        osDelay(1);
        HAL_GPIO_WritePin(SPI1_LATCH_GPIO_Port, SPI1_LATCH_Pin, GPIO_PIN_SET);
      }
    }
  }
}

/* ============================================================================
 * HID Task Implementation
 * ========================================================================== */
/**
  * @brief  HID Communication Task
  * Aggregates data from all queues and sends HID reports
  * Also receives LED control data from HID and forwards to LED queue
  */
void HidTask(void *argument)
{
  for(;;)
  {
    /* HID 报文由 AdcTask 打包并发送，此任务预留给后续编码器/按键上报 */
    osDelay(HID_TASK_DELAY);
  }
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

