#include "main.h"
#include "oled.h"

// OLEDœ‘ æ»ŒŒÒ
void StartOLEDTask(void *argument)
{
    for (;;)
    {
        osMutexAcquire(healthMutexHandle, osWaitForever);
        OLED_ShowNum(68, 1, g_health.step_cnt, 3, 12, 0);
        OLED_Showdecimal(68, 2, g_health.spo2, 2, 2, 12, 0);
        OLED_ShowNum(68, 3, g_health.heart, 3, 12, 0);
        osMutexRelease(healthMutexHandle);

        osDelay(200);
    }
}
