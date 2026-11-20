#include "max30102.h"
#include "blood.h"
#include "main.h"

extern uint16_t fifo_red;		//定义FIFO中的红光数据
extern uint16_t fifo_ir;		//定义FIFO中的红外光数据
extern struct compx s1[FFT_N+16];           	//FFT输入和输出：从S[1]开始存放，根据大小自己定义
extern struct compx s2[FFT_N+16];           	//FFT输入和输出：从S[1]开始存放，根据大小自己定义
extern uint16_t g_fft_index;         	 	//fft输入输出下标
extern int heart;		//定义心率
extern float SpO2;		//定义血氧饱和度

// max30102任务
void StartMAX30102Task(void *argument)
{
    uint8_t status;

    for (;;)
    {
        status = max30102_read_reg(REG_INTR_STATUS_1);
        if (status & 0x40)
        {
            max30102_read_fifo();

            s1[g_fft_index].real = fifo_red;
            s1[g_fft_index].imag = 0;
            s2[g_fft_index].real = fifo_ir;
            s2[g_fft_index].imag = 0;

            g_fft_index++;

            if (g_fft_index >= FFT_N)
            {
                blood_data_translate();

                osMutexAcquire(healthMutexHandle, osWaitForever);
                g_health.heart = heart;
                g_health.spo2  = SpO2;
                osMutexRelease(healthMutexHandle);

                g_fft_index = 0;
            }
        }
        osDelay(20);
    }
}
