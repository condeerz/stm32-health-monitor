#include "main.h"
#include "alarm.h"

void StartAlarmTask(void *argument)
{
    for (;;)
    {
        osMutexAcquire(healthMutexHandle, osWaitForever);
        Health_Alarm_Check(g_health.heart, g_health.spo2);
        osMutexRelease(healthMutexHandle);

        osDelay(500);
    }
}
