/**
 * ************************************************************************
 * 
 * @file blood.c
 * @author 徐煜果
 * @brief 心率/血氧计算实现
 * 
 * ************************************************************************
 */
#include "blood.h"
#include "usart.h"
#include "adxl345.h"
#include <stdio.h>
#include <string.h>

int heart;		// 心率（bpm），0 表示无效
float SpO2;		// 血氧饱和度（%），0 表示无效

// MAX30102 驱动里的 FIFO 原始值
extern uint16_t fifo_red;		// 红光原始值
extern uint16_t fifo_ir;		// 红外原始值

uint16_t g_fft_index = 0;         	 	// FFT 输入数组的下标
struct compx s1[FFT_N+16];           	// FFT 输入/输出共用数组；实部存采样值，虚部先清零
struct compx s2[FFT_N+16];           	// FFT 输入/输出共用数组；实部存采样值，虚部先清零

#define CORRECTED_VALUE			47   			// 血氧公式系数

/* 把 MAX30102 FIFO 里的红光/红外原始值搬进 FFT 输入数组 */
void blood_data_update(void)
{
	// 等 FIFO 里有新数据再读
	g_fft_index=0;
	
	uint8_t status = max30102_read_reg(REG_INTR_STATUS_1);
	
	while (g_fft_index < FFT_N)
	{		

		if ((status & 0x40))
		{
			// 读一次 FIFO
			max30102_read_fifo();  // 原始值在驱动里拼好
			
			if(g_fft_index < FFT_N)
			{
				// 实部存原始值，虚部清零
				s1[g_fft_index].real = fifo_red;
				s1[g_fft_index].imag= 0;
				s2[g_fft_index].real = fifo_ir;
				s2[g_fft_index].imag= 0;
				g_fft_index++;
			}
		}
	}
}


/* 去直流 -> 平滑 -> FFT -> 找峰 -> 算心率和 SpO2 */
void blood_data_translate(void)
{	
	float n_denom;
	uint16_t i;
	int s1_max_index;
	uint16_t Heart_Rate;
	float R;
	float sp02_num;

	// 去直流：先求均值，再逐点减掉
	float dc_red =0; 
	float dc_ir =0;
	float ac_red =0; 
	float ac_ir =0;
	
	for (i=0 ; i<FFT_N ; i++ ) 
	{
		dc_red += s1[i].real ;
		dc_ir +=  s2[i].real ;
	}
		dc_red =dc_red/FFT_N ;
		dc_ir =dc_ir/FFT_N ;
	for (i=0 ; i<FFT_N ; i++ )  
	{
		s1[i].real =  s1[i].real - dc_red ; 
		s2[i].real =  s2[i].real - dc_ir ; 
	}

	// 三点滑动平均
	for(i = 1;i < FFT_N-1;i++) 
	{
		n_denom= ( s1[i-1].real + 2*s1[i].real + s1[i+1].real);
		s1[i].real=  n_denom/4.00; 
		
		n_denom= ( s2[i-1].real + 2*s2[i].real + s2[i+1].real);
		s2[i].real=  n_denom/4.00; 			
	}

	// 八点滑动平均，再平滑一次
	for(i = 0;i < FFT_N-8;i++) 
	{
		n_denom= ( s1[i].real+s1[i+1].real+ s1[i+2].real+ s1[i+3].real+ s1[i+4].real+ s1[i+5].real+ s1[i+6].real+ s1[i+7].real);
		s1[i].real=  n_denom/8.00; 
		
		n_denom= ( s2[i].real+s2[i+1].real+ s2[i+2].real+ s2[i+3].real+ s2[i+4].real+ s2[i+5].real+ s2[i+6].real+ s2[i+7].real);
		s2[i].real=  n_denom/8.00; 
		
	}

	// 开始 FFT	
	g_fft_index = 0;	
	// 两路分别做 FFT
	FFT(s1);
	FFT(s2);
	
	for(i = 0;i < FFT_N;i++) 
	{
		s1[i].real=sqrtf(s1[i].real*s1[i].real+s1[i].imag*s1[i].imag);
		s2[i].real=sqrtf(s2[i].real*s2[i].real+s2[i].imag*s2[i].imag);
	}
	// 交流分量 = 各次谐波幅值之和（跳过直流）
	for (i=1 ; i<FFT_N ; i++ ) 
	{
		ac_red += s1[i].real ;
		ac_ir +=  s2[i].real ;
	}
	
	for(i = 0;i < 50;i++) 
	{
		if(s1[i].real<=10)
			break;
	}
	
	// 信号有效性检查：无手指或信号过弱时直流/交流分量会退化成 0，直接判无效
	if (i < 45 || dc_red <= 0.0f || dc_ir <= 0.0f || ac_red <= 0.0f || ac_ir <= 0.0f)
	{
		heart = 0;
		SpO2 = 0;
		return;
	}

	// 在低频段内找幅值最大的谱线序号
	s1_max_index = find_max_num_index(s1, 60);

	// 心率 = 60 * 采样率 * 峰值谱线序号 / FFT 点数
	Heart_Rate = (uint16_t)(60.00 * SAMPLES_PER_SECOND * s1_max_index / FFT_N);
	if (Heart_Rate < 30 || Heart_Rate > 220)
	{
		// 超出人体心率范围，按无效数据处理
		heart = 0;
		SpO2 = 0;
		return;
	}
	heart = Heart_Rate;

	// 血氧：R = (AC_red / DC_red) / (AC_ir / DC_ir)
	R = (ac_red * dc_ir) / (dc_red * ac_ir);
	sp02_num = -45.060f * R * R + 30.354f * R + 94.845f;

	if (sp02_num > 99.99f) sp02_num = 99.99f;
	if (sp02_num < 0.0f)   sp02_num = 0.0f;
	SpO2 = sp02_num;

	
}

/* 采集一帧并解算，结果写到全局 heart/SpO2 */
void blood_Loop(void)
{
	// 先采集
	blood_data_update();
	// 再解算
	blood_data_translate();

	SpO2 = (SpO2 > 99.99) ? 99.99:SpO2;  
}
