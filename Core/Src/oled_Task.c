#include "main.h"
#include "oled.h"

// OLED显示任务：先在锁内取快照，再在锁外刷屏，避免 I2C 写屏期间阻塞采集任务
void StartOLEDTask(void *argument)
{
	uint32_t step;
	float    spo2;
	int      heart;

	for (;;)
	{
		osMutexAcquire(healthMutexHandle, osWaitForever);
		step  = g_health.step_cnt;
		spo2  = g_health.spo2;
		heart = g_health.heart;
		osMutexRelease(healthMutexHandle);

		OLED_ShowNum(68, 1, step, 3, 12, 0);
		OLED_Showdecimal(68, 2, spo2, 2, 2, 12, 0);
		OLED_ShowNum(68, 3, heart, 3, 12, 0);

		osDelay(200);
	}
}