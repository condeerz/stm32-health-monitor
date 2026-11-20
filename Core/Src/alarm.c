#include "main.h"
#include "alarm.h"

void Health_Alarm_Check(int heart, float spo2)
{
    uint8_t alarm = 0;

    /* 无手指或算法判无效时 heart 与 SpO2 同时为 0：
       此时不判异常，避免蜂鸣器一直长鸣 */
    if (heart <= 0 && spo2 <= 0.0f)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
        return;
    }

    if (heart < HEART_LOW_TH || heart > HEART_HIGH_TH)
        alarm = 1;

    if (spo2 < SPO2_LOW_TH)
        alarm = 1;

    if (alarm)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET); 	// LED1 亮
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); 	// LED2 亮
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);   	// 蜂鸣器响
    }
    else
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET); 		// LED1 灭
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET); 		// LED2 灭
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);   // 蜂鸣器响
    }
}
