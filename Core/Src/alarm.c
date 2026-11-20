#include "main.h"
#include "alarm.h"

void Health_Alarm_Check(int heart, float spo2)
{
    uint8_t alarm = 0;

    if (heart < HEART_LOW_TH || heart > HEART_HIGH_TH)
        alarm = 1;

    if (spo2 < SPO2_LOW_TH)
        alarm = 1;

    if (alarm)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET); 	// LED1 ÁÁ
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); 	// LED2 ÁÁ
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);   	// ·äÃùÆ÷Ïì
    }
    else
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET); 		// LED1 Ãð
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET); 		// LED2 Ãð
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);   // ·äÃùÆ÷Ïì
    }
}
