#include "adxl345.h"
#include <math.h>

// ==================== 初始化ADXL345（配置测量模式、数据格式） ====================
HAL_StatusTypeDef ADXL345_Init(I2C_HandleTypeDef *hi2c) 
{
    uint8_t dev_id;
    HAL_StatusTypeDef status;

    // 1. 验证设备ID（应返回0xE5）
    status = ADXL345_ReadReg(hi2c, DEVID, &dev_id);
    if (status != HAL_OK || dev_id != 0xE5) {
        return HAL_ERROR;  // 设备未连接或ID错误
    }

    // 2. 配置电源控制寄存器（POWER_CTL）：启动测量模式（bit3=1）
    status = ADXL345_WriteReg(hi2c, POWER_CTL, 0x08);  // 0x08=00001000（测量模式）
    if (status != HAL_OK) return status;

    // 3. 配置数据格式寄存器（DATA_FORMAT）：±2g量程，全分辨率，右对齐
    status = ADXL345_WriteReg(hi2c, DATA_FORMAT, 0x00);  // 0x00=±2g，13位分辨率
    if (status != HAL_OK) return status;

    // 4. 配置带宽速率寄存器（BW_RATE）：100Hz采样率（0x0A=100Hz）
    status = ADXL345_WriteReg(hi2c, BW_RATE, 0x0A);
    return status;
}

// ==================== 写单个寄存器 ====================
HAL_StatusTypeDef ADXL345_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t data) 
{
    return HAL_I2C_Mem_Write(hi2c, ADXL345_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

// ==================== 读单个寄存器 ====================
HAL_StatusTypeDef ADXL345_ReadReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *data) 
{
    return HAL_I2C_Mem_Read(hi2c, ADXL345_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, 1, 100);
}

// ==================== 读多个寄存器（批量读取加速度数据） ====================
HAL_StatusTypeDef ADXL345_ReadRegs(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t *buf, uint8_t len) 
{
    return HAL_I2C_Mem_Read(hi2c, ADXL345_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
}

// ==================== 读取三轴加速度（转换为g单位） ====================
HAL_StatusTypeDef ADXL345_ReadAccel(I2C_HandleTypeDef *hi2c, short *ax, short *ay, short *az)	
{
    uint8_t data[6];
//    int16_t raw_x, raw_y, raw_z;
    HAL_StatusTypeDef status;

    // 1. 读取6字节数据（X低8+X高8+Y低8+Y高8+Z低8+Z高8）
    status = ADXL345_ReadRegs(hi2c, DATAX0, data, 6);
    if (status != HAL_OK) return status;

    // 2. 组合16位有符号整数（ADXL345数据右对齐，13位有效）
    *ax = (int16_t)((data[1] << 8) | data[0]);  // X轴原始值
    *ay = (int16_t)((data[3] << 8) | data[2]);  // Y轴原始值
    *az = (int16_t)((data[5] << 8) | data[4]);  // Z轴原始值
	
    return HAL_OK;
}

// 读取xyz数据存入均值滤波器，存满进行计算，滤波后样本存入sample
void filter_calculate(filter_avg_t *filter, axis_info_t *sample)
{
	uint8_t i;
	short x_sum = 0, y_sum = 0, z_sum = 0;
	
	for (i = 0; i < FILTER_CNT; i++) 
	{
		ADXL345_ReadAccel(&hi2c2, &filter->info[i].x, &filter->info[i].y, &filter->info[i].z);
		x_sum += filter->info[i].x;
		y_sum += filter->info[i].y;
		z_sum += filter->info[i].z;
	}
	sample->x = x_sum / FILTER_CNT;
	sample->y = y_sum / FILTER_CNT;
	sample->z = z_sum / FILTER_CNT;
}

//50个样本中找出最大最小值作为动态阈值
//找出最大值和最小值
void peak_update(peak_value_t *peak, axis_info_t *sample)
{
	int8_t i;
	filter_avg_t filter;
	short min_value = -32768;
	short max_value = 32767;
	peak->newmax.x = min_value;
	peak->newmax.y = min_value;
	peak->newmax.z = min_value;
	
	peak->newmin.x = max_value;
	peak->newmin.y = max_value;
	peak->newmin.z = max_value;
	for (i = 0; i < SAMPLE_SIZE; i++) 
 {
		filter_calculate(&filter,sample);
		
		peak->newmax.x = MAX(peak->newmax.x, sample->x);
		peak->newmax.y = MAX(peak->newmax.y, sample->y);
		peak->newmax.z = MAX(peak->newmax.z, sample->z);
	
		peak->newmin.x = MIN(peak->newmin.x, sample->x);
		peak->newmin.y = MIN(peak->newmin.y, sample->y);
		peak->newmin.z = MIN(peak->newmin.z, sample->z);
	
	}	
	peak->oldmax.x = peak->newmax.x;
	peak->oldmax.y = peak->newmax.y;
	peak->oldmax.z = peak->newmax.z;
	
	peak->oldmin.x = peak->newmin.x;
	peak->oldmin.y = peak->newmin.y;
	peak->oldmin.z = peak->newmin.z;
}

void slid_update(slid_reg_t *slid, axis_info_t *cur_sample)
{
	filter_avg_t filter;
	filter_calculate(&filter, cur_sample);
	cur_sample->x = cur_sample->x;
	cur_sample->y = cur_sample->y;
	cur_sample->z = cur_sample->z;
	
	if (ABS((cur_sample->x - slid->new_sample.x)) > DYNAMIC_PRECISION) {
		slid->old_sample.x = slid->new_sample.x;
		slid->new_sample.x = cur_sample->x;
	} else {
		slid->old_sample.x = slid->new_sample.x;
	}
	if (ABS((cur_sample->y - slid->new_sample.y)) > DYNAMIC_PRECISION) {
		slid->old_sample.y = slid->new_sample.y;
		slid->new_sample.y = cur_sample->y;
	} else {
		slid->old_sample.y = slid->new_sample.y;
	}
	
	if (ABS((cur_sample->z - slid->new_sample.z)) > DYNAMIC_PRECISION) {
		slid->old_sample.z = slid->new_sample.z;
		slid->new_sample.z = cur_sample->z;
	} else {
		slid->old_sample.z = slid->new_sample.z;
	}
}
 
char is_most_active(peak_value_t *peak)
{
	char res;
	short x_change;
	short y_change;
	short z_change;
	axis_info_t cur_sample;
	peak_update(peak,&cur_sample);
	res = MOST_ACTIVE_NULL;
	
	x_change = ABS((peak->newmax.x - peak->newmin.x));
	y_change = ABS((peak->newmax.y - peak->newmin.y));
	z_change = ABS((peak->newmax.z - peak->newmin.z));
	
	if (x_change > y_change && x_change > z_change && x_change >= ACTIVE_PRECISION) {
		res = MOST_ACTIVE_X;
	} else if (y_change > x_change && y_change > z_change && y_change >= ACTIVE_PRECISION) {
		res = MOST_ACTIVE_Y;
	} else if (z_change > x_change && z_change > y_change && z_change >= ACTIVE_PRECISION) {
		res = MOST_ACTIVE_Z;
	}
	return res;
}
 
/*判断是否走步*/
int8_t detect_step(peak_value_t *peak, slid_reg_t *slid, axis_info_t *cur_sample)
{
	static int8_t step_cnt = 0;
//	uint32_t step_interval;
//	filter_avg_t filter;
	char res;
	res = is_most_active(peak);
	slid_update(slid, cur_sample);
	
	switch (res) {
		case MOST_ACTIVE_NULL: {
			//fix
			break;
		}
		case MOST_ACTIVE_X: {
			short threshold_x = (peak->oldmax.x + peak->oldmin.x) / 2;
			if (slid->old_sample.x > threshold_x && slid->new_sample.x < threshold_x)
			{
				step_cnt ++;
			}
			break;
		}
		case MOST_ACTIVE_Y: {
			short threshold_y = (peak->oldmax.y + peak->oldmin.y) / 2;
			if (slid->old_sample.y > threshold_y && slid->new_sample.y < threshold_y) 
				{
					step_cnt ++;
				}
			break;
		}
		case MOST_ACTIVE_Z: {
			short threshold_z = (peak->oldmax.z + peak->oldmin.z) / 2;
			if (slid->old_sample.z > threshold_z && slid->new_sample.z < threshold_z) 
				{
					step_cnt ++;
				}
			break;
		}
		default: 
			break;
	}
	return step_cnt;
}
