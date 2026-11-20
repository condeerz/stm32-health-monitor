#include "adxl345.h"
#include "main.h"

// ADXL345 хннЯ
void StartADXL345Task(void *argument)
{
    peak_value_t peak;
    axis_info_t cur_sample;
    slid_reg_t slid;
    uint8_t step_cnt = 0;

    for (;;)
    {
        step_cnt = detect_step(&peak, &slid, &cur_sample);

        osMutexAcquire(healthMutexHandle, osWaitForever);
        g_health.step_cnt = step_cnt;
        osMutexRelease(healthMutexHandle);

        osDelay(20);
    }
}
