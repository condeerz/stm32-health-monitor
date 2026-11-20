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
    status = ADXL345_WriteReg(hi2c, DATA_FORMAT, 0x08);  // 0x08 = ±2g 全分辨率（FULL_RES=1，13 位）
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

// 读取xyz数据存入均值滤波器，存满计算平均值（一次采样 = FILTER_CNT 次 I2C 读）
static void filter_calculate(filter_avg_t *filter, axis_info_t *sample)
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

/* 采样环形缓冲：每个任务周期只做一次 I2C 采样，峰值统计在缓冲上完成。
   原实现在 peak_update 内部重复采样 SAMPLE_SIZE*FILTER_CNT 次（50*4 次 I2C 读），
   100 kHz 总线上约 160 ms，导致计步任务的实际周期远大于设定的 20 ms。 */
static axis_info_t s_sample_buf[SAMPLE_SIZE];
static uint8_t      s_sample_idx = 0;
static uint8_t      s_sample_cnt = 0;

static void sample_buf_push(const axis_info_t *sample)
{
	s_sample_buf[s_sample_idx] = *sample;
	s_sample_idx = (uint8_t)((s_sample_idx + 1U) % SAMPLE_SIZE);
	if (s_sample_cnt < SAMPLE_SIZE)
	{
		s_sample_cnt++;
	}
}

// 动态阈值：在最近 s_sample_cnt 个采样点上取最大/最小值
static void peak_update(peak_value_t *peak)
{
	uint8_t i;

	peak->newmax.x = -32768;
	peak->newmax.y = -32768;
	peak->newmax.z = -32768;

	peak->newmin.x = 32767;
	peak->newmin.y = 32767;
	peak->newmin.z = 32767;

	for (i = 0; i < s_sample_cnt; i++)
	{
		const axis_info_t *s = &s_sample_buf[i];

		peak->newmax.x = MAX(peak->newmax.x, s->x);
		peak->newmax.y = MAX(peak->newmax.y, s->y);
		peak->newmax.z = MAX(peak->newmax.z, s->z);

		peak->newmin.x = MIN(peak->newmin.x, s->x);
		peak->newmin.y = MIN(peak->newmin.y, s->y);
		peak->newmin.z = MIN(peak->newmin.z, s->z);
	}

	peak->oldmax = peak->newmax;
	peak->oldmin = peak->newmin;
}

// 线性移位寄存器，过滤高频抖动后用于过阈值判断
static void slid_update(slid_reg_t *slid, const axis_info_t *cur_sample)
{
	if (ABS(cur_sample->x - slid->new_sample.x) > DYNAMIC_PRECISION) {
		slid->old_sample.x = slid->new_sample.x;
		slid->new_sample.x = cur_sample->x;
	} else {
		slid->old_sample.x = slid->new_sample.x;
	}
	if (ABS(cur_sample->y - slid->new_sample.y) > DYNAMIC_PRECISION) {
		slid->old_sample.y = slid->new_sample.y;
		slid->new_sample.y = cur_sample->y;
	} else {
		slid->old_sample.y = slid->new_sample.y;
	}

	if (ABS(cur_sample->z - slid->new_sample.z) > DYNAMIC_PRECISION) {
		slid->old_sample.z = slid->new_sample.z;
		slid->new_sample.z = cur_sample->z;
	} else {
		slid->old_sample.z = slid->new_sample.z;
	}
}

// 判断哪根轴的上下摆幅最大，作为计步判定轴
static char is_most_active(peak_value_t *peak)
{
	char res;
	short x_change;
	short y_change;
	short z_change;

	peak_update(peak);
	res = MOST_ACTIVE_NULL;

	x_change = ABS(peak->newmax.x - peak->newmin.x);
	y_change = ABS(peak->newmax.y - peak->newmin.y);
	z_change = ABS(peak->newmax.z - peak->newmin.z);

	if (x_change > y_change && x_change > z_change && x_change >= ACTIVE_PRECISION) {
		res = MOST_ACTIVE_X;
	} else if (y_change > x_change && y_change > z_change && y_change >= ACTIVE_PRECISION) {
		res = MOST_ACTIVE_Y;
	} else if (z_change > x_change && z_change > y_change && z_change >= ACTIVE_PRECISION) {
		res = MOST_ACTIVE_Z;
	}
	return res;
}

/* 判断是否走步：每调用一次只采一个样，历史样本来自环形缓冲 */
uint16_t detect_step(peak_value_t *peak, slid_reg_t *slid, axis_info_t *cur_sample)
{
	static uint16_t step_cnt = 0;
	filter_avg_t filter;
	char res;

	filter_calculate(&filter, cur_sample);
	sample_buf_push(cur_sample);

	res = is_most_active(peak);
	slid_update(slid, cur_sample);

	switch (res) {
		case MOST_ACTIVE_X: {
			short threshold_x = (peak->oldmax.x + peak->oldmin.x) / 2;
			if (slid->old_sample.x > threshold_x && slid->new_sample.x < threshold_x)
			{
				step_cnt++;
			}
			break;
		}
		case MOST_ACTIVE_Y: {
			short threshold_y = (peak->oldmax.y + peak->oldmin.y) / 2;
			if (slid->old_sample.y > threshold_y && slid->new_sample.y < threshold_y)
			{
				step_cnt++;
			}
			break;
		}
		case MOST_ACTIVE_Z: {
			short threshold_z = (peak->oldmax.z + peak->oldmin.z) / 2;
			if (slid->old_sample.z > threshold_z && slid->new_sample.z < threshold_z)
			{
				step_cnt++;
			}
			break;
		}
		default:
			break;
	}

	return step_cnt;
}
