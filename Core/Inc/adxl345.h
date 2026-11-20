#ifndef __ADXL345_H
#define __ADXL345_H

#include "stm32f1xx_hal.h"

extern I2C_HandleTypeDef hi2c2;

// ==================== ADXL345 I2C地址（SDO接地=0x53，接VCC=0x1D） ====================
#define ADXL345_ADDR (0x53 << 1)  // I2C设备地址（7位地址左移1位，HAL库要求）

// ==================== ADXL345寄存器地址 ====================
#define DEVID        0x00    // 设备ID（固定0xE5）
#define POWER_CTL    0x2D    // 电源控制寄存器
#define DATA_FORMAT  0x31    // 数据格式寄存器
#define DATAX0       0x32    // X轴数据低8位
#define DATAX1       0x33    // X轴数据高8位
#define DATAY0       0x34    // Y轴数据低8位
#define DATAY1       0x35    // Y轴数据高8位
#define DATAZ0       0x36    // Z轴数据低8位
#define DATAZ1       0x37    // Z轴数据高8位
#define BW_RATE      0x2C    // 带宽速率寄存器（采样率）

/* ==================== 全局 ==================== */
#define FILTER_CNT			4
 
#define MAX(a,b) ((a) > (b) ? (a) : (b)) 
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define SAMPLE_SIZE         50
 
 
#define ABS(a) (((a) < 0) ? -(a) : (a))
#define DYNAMIC_PRECISION     			30   	 /*动态精度*/
 
#define MOST_ACTIVE_NULL      			0      	 /*未找到最活跃轴*/
#define MOST_ACTIVE_X					1		 /*最活跃轴X*/	
#define MOST_ACTIVE_Y					2        /*最活跃轴Y*/	
#define MOST_ACTIVE_Z					3        /*最活跃轴Z*/	
 
#define ACTIVE_PRECISION      			40       /*活跃轴最小变化值*/

// 结构体
typedef struct {
  short x;
  short y;
  short z;
}axis_info_t;

typedef struct filter_avg
{
	axis_info_t info[FILTER_CNT];
}filter_avg_t;

typedef struct {
	axis_info_t newmax;
	axis_info_t newmin;
	axis_info_t oldmax;
	axis_info_t oldmin;
}peak_value_t;

/*一个线性移位寄存器，用于过滤高频噪声*/
typedef struct slid_reg
{
	axis_info_t new_sample;
	axis_info_t old_sample;
}slid_reg_t;

// ==================== 函数声明 ====================
HAL_StatusTypeDef ADXL345_Init(I2C_HandleTypeDef *hi2c);  // 初始化ADXL345
HAL_StatusTypeDef ADXL345_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t data);  // 写寄存器
HAL_StatusTypeDef ADXL345_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data);   // 读单个寄存器
HAL_StatusTypeDef ADXL345_ReadRegs(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *buf, uint8_t len);  // 读多个寄存器
HAL_StatusTypeDef ADXL345_ReadAccel(I2C_HandleTypeDef *hi2c, short *ax, short *ay, short *az);  // 读取三轴加速度（g）
uint16_t detect_step(peak_value_t *peak, slid_reg_t *slid, axis_info_t *cur_sample);

#endif
