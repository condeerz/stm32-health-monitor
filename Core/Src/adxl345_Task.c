#include "adxl345.h"
#include "main.h"

// ADXL345 任务：20 ms 一拍，把最新步数发布到共享的 g_health
void StartADXL345Task(void *argument)
{
	peak_value_t peak = {0};
	axis_info_t cur_sample = {0};
	slid_reg_t  slid = {0};
	uint32_t    step_cnt = 0;

	for (;;)
	{
		step_cnt = detect_step(&peak, &slid, &cur_sample);

		osMutexAcquire(healthMutexHandle, osWaitForever);
		g_health.step_cnt = step_cnt;
		osMutexRelease(healthMutexHandle);

		osDelay(20);
	}
}