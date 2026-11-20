#include "main.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

void StartBLETask(void *argument)
{
    char buf[64];

    for (;;)
    {
        osMutexAcquire(healthMutexHandle, osWaitForever);

        snprintf(buf, sizeof(buf),"HR:%d SpO2:%.1f Step:%d\r\n", g_health.heart, g_health.spo2, g_health.step_cnt);

        osMutexRelease(healthMutexHandle);

        HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
        osDelay(1000);   // 1秒上传一次
    }
}
