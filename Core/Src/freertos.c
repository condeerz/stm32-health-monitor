/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
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

/* USER CODE END Variables */
/* Definitions for MAX30102_Task */
osThreadId_t MAX30102_TaskHandle;
const osThreadAttr_t MAX30102_Task_attributes = {
  .name = "MAX30102_Task",
  .priority = (osPriority_t) osPriorityHigh,
  .stack_size = 512 * 4
};
/* Definitions for ADXL345_Task */
osThreadId_t ADXL345_TaskHandle;
const osThreadAttr_t ADXL345_Task_attributes = {
  .name = "ADXL345_Task",
  .priority = (osPriority_t) osPriorityHigh,
  .stack_size = 512 * 4
};
/* Definitions for OLED_Task */
osThreadId_t OLED_TaskHandle;
const osThreadAttr_t OLED_Task_attributes = {
  .name = "OLED_Task",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 256 * 4
};
/* Definitions for BLE_Task */
osThreadId_t BLE_TaskHandle;
const osThreadAttr_t BLE_Task_attributes = {
  .name = "BLE_Task",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 256 * 4
};
/* Definitions for Alarm_Task */
osThreadId_t Alarm_TaskHandle;
const osThreadAttr_t Alarm_Task_attributes = {
  .name = "Alarm_Task",
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 128 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartMAX30102Task(void *argument);
void StartADXL345Task(void *argument);
void StartOLEDTask(void *argument);
void StartBLETask(void *argument);
void StartAlarmTask(void *argument);

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
  healthMutexHandle = osMutexNew(NULL);
  if (healthMutexHandle == NULL)
  {
    Error_Handler();
  }
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of MAX30102_Task */
  MAX30102_TaskHandle = osThreadNew(StartMAX30102Task, NULL, &MAX30102_Task_attributes);

  /* creation of ADXL345_Task */
  ADXL345_TaskHandle = osThreadNew(StartADXL345Task, NULL, &ADXL345_Task_attributes);

  /* creation of OLED_Task */
  OLED_TaskHandle = osThreadNew(StartOLEDTask, NULL, &OLED_Task_attributes);

  /* creation of BLE_Task */
  BLE_TaskHandle = osThreadNew(StartBLETask, NULL, &BLE_Task_attributes);

  /* creation of Alarm_Task */
  Alarm_TaskHandle = osThreadNew(StartAlarmTask, NULL, &Alarm_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartMAX30102Task */
/**
  * @brief  Function implementing the MAX30102_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartMAX30102Task */
__weak void StartMAX30102Task(void *argument)
{
  /* USER CODE BEGIN StartMAX30102Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartMAX30102Task */
}

/* USER CODE BEGIN Header_StartADXL345Task */
/**
* @brief Function implementing the ADXL345_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartADXL345Task */
__weak void StartADXL345Task(void *argument)
{
  /* USER CODE BEGIN StartADXL345Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartADXL345Task */
}

/* USER CODE BEGIN Header_StartOLEDTask */
/**
* @brief Function implementing the OLED_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartOLEDTask */
__weak void StartOLEDTask(void *argument)
{
  /* USER CODE BEGIN StartOLEDTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartOLEDTask */
}

/* USER CODE BEGIN Header_StartBLETask */
/**
* @brief Function implementing the BLE_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartBLETask */
__weak void StartBLETask(void *argument)
{
  /* USER CODE BEGIN StartBLETask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartBLETask */
}

/* USER CODE BEGIN Header_StartAlarmTask */
/**
* @brief Function implementing the Alarm_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartAlarmTask */
__weak void StartAlarmTask(void *argument)
{
  /* USER CODE BEGIN StartAlarmTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartAlarmTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
