/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    joystick_rtos.h
  * @brief   Joystick RTOS Task and Queue Definitions
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __JOYSTICK_RTOS_H__
#define __JOYSTICK_RTOS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* ============================================================================
 * Data Structure Definitions
 * ========================================================================== */

/**
 * @brief ADC Data Structure
 * Contains 6 analog channel values from joystick
 */
typedef struct {
    uint16_t channels[6];
} AdcData_t;

/**
 * @brief Encoder Data Structure
 * Contains count values from 4 encoders (TIM1,2,3,4)
 */
typedef struct {
    uint32_t encoders[4];
} EncoderData_t;

/**
 * @brief Button Data Structure
 * Contains 24 button states packed in 3 bytes
 */
typedef struct {
    uint8_t states[3];
} ButtonData_t;

/**
 * @brief LED Indicator Data Structure
 * Contains 24 LED states packed in 3 bytes
 */
typedef struct {
    uint8_t states[3];
} LedData_t;

/**
 * @brief Complete HID Report Data
 * Contains all joystick data to be sent via HID
 */
typedef struct {
    AdcData_t adc;
    EncoderData_t encoders;
    ButtonData_t buttons;
} HidReportData_t;

/* ============================================================================
 * Queue and Semaphore Definitions
 * ========================================================================== */

/* Queue Attributes and Handles */
extern osMessageQueueId_t adcQueueHandle;           /* ADC data queue */
extern osMessageQueueId_t encoderQueueHandle;       /* Encoder data queue */
extern osMessageQueueId_t buttonQueueHandle;        /* Button data queue */
extern osMessageQueueId_t ledQueueHandle;           /* LED control queue */

/* Thread Handles */
extern osThreadId_t adcTaskHandle;                  /* ADC conversion task */
extern osThreadId_t encoderTaskHandle;              /* Encoder reading task */
extern osThreadId_t buttonTaskHandle;               /* Button reading task */
extern osThreadId_t ledTaskHandle;                  /* LED control task */
extern osThreadId_t hidTaskHandle;                  /* HID communication task */

/* ============================================================================
 * Task Stack Sizes
 * ========================================================================== */

#define ADC_TASK_STACK_SIZE        128  /* Simple ADC reading */
#define ENCODER_TASK_STACK_SIZE    128  /* Simple counter reading */
#define BUTTON_TASK_STACK_SIZE     128  /* SPI reading */
#define LED_TASK_STACK_SIZE        128  /* SPI writing */
#define HID_TASK_STACK_SIZE        384  /* USB HID send */

/* ============================================================================
 * Task Priorities
 * ========================================================================== */
/* Note: Priority levels from cmsis_os2.h are:
   osPriorityLow(8) < osPriorityBelowNormal(16) < osPriorityNormal(24) < 
   osPriorityAboveNormal(32) < osPriorityHigh(40) < osPriorityRealtime(48) */

#define ADC_TASK_PRIORITY          (osPriorityHigh)           /* 40: Fast ADC sampling */
#define ENCODER_TASK_PRIORITY      (osPriorityAboveNormal)    /* 32: Encoder reading */
#define BUTTON_TASK_PRIORITY       (osPriorityNormal)         /* 24: Button polling */
#define LED_TASK_PRIORITY          (osPriorityBelowNormal)    /* 16: LED output (passive) */
#define HID_TASK_PRIORITY          (osPriorityAboveNormal)    /* 32: USB HID send */

/* ============================================================================
 * Task Timing Constants (milliseconds)
 * ========================================================================== */

#define ADC_TASK_DELAY             10   /* ADC conversion every 10ms */
#define ENCODER_TASK_DELAY         10   /* Read encoders every 10ms */
#define BUTTON_TASK_DELAY          20   /* Read buttons every 20ms */
#define LED_TASK_DELAY             5    /* LED control delay 5ms */
#define HID_TASK_DELAY             10   /* HID update every 10ms */

/* ============================================================================
 * Function Prototypes
 * ========================================================================== */

/**
 * @brief Initialize all queues
 */
void Joystick_RTOS_Init_Queues(void);

/**
 * @brief ADC conversion task
 */
void AdcTask(void *argument);

/**
 * @brief Encoder reading task
 */
void EncoderTask(void *argument);

/**
 * @brief Button reading task
 */
void ButtonTask(void *argument);

/**
 * @brief LED control task
 */
void LedTask(void *argument);

/**
 * @brief HID communication task
 */
void HidTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __JOYSTICK_RTOS_H__ */
