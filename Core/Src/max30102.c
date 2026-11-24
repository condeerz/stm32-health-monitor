/**
 * ************************************************************************
 * 
 * @file max30102.c
 * @author 徐煜果
 * @brief MAX30102 驱动实现
 * 
 * ************************************************************************
 */
#include "MAX30102.h"

uint16_t fifo_red;  // FIFO 红光原始值
uint16_t fifo_ir;   // FIFO 红外原始值

/* 写寄存器：HAL_I2C_Mem_Write 的 memAddress 就是寄存器地址 */
uint8_t max30102_write_reg(uint8_t addr, uint8_t data)
{
  HAL_I2C_Mem_Write(&hi2c1, MAX30102_Device_address,	addr,	1,	&data,1,HAL_MAX_DELAY);
  return 1;
}


/* 读寄存器 */
uint8_t max30102_read_reg(uint8_t addr )
{
  uint8_t data=0;
  HAL_I2C_Mem_Read(&hi2c1, MAX30102_Device_address, addr, 1, &data, 1, HAL_MAX_DELAY);
  return data;
}


/* 软复位：MODE_CONFIG bit6 写 1，复位后要重新配置 */
uint8_t Max30102_reset(void)
{
	if(max30102_write_reg(REG_MODE_CONFIG, 0x40))
        return 1;
    else
        return 0;    
}

/* 初始化：开 FIFO 满/PPG 就绪中断，SpO2 模式，50 Hz、400 us 脉宽，LED 约 10 mA */
void MAX30102_Config(void)
{
	max30102_write_reg(REG_INTR_ENABLE_1,0xc0);// 开 FIFO 满和 PPG 就绪中断
	max30102_write_reg(REG_INTR_ENABLE_2,0x00);// 第二组中断不用
	max30102_write_reg(REG_FIFO_WR_PTR,0x00);// 写指针清零
	max30102_write_reg(REG_OVF_COUNTER,0x00);// 溢出计数清零
	max30102_write_reg(REG_FIFO_RD_PTR,0x00);// 读指针清零
	
	max30102_write_reg(REG_FIFO_CONFIG,0x0f);// 不平均、FIFO 不覆盖、几乎满阈值 17
	max30102_write_reg(REG_MODE_CONFIG,0x03);// SpO2 模式：红光 + 红外
	max30102_write_reg(REG_SPO2_CONFIG,0x27);	// ADC 量程 4096 nA，采样率 50 Hz，脉宽 400 us  
	max30102_write_reg(REG_LED1_PA,0x32);// LED1 电流约 10 mA
	max30102_write_reg(REG_LED2_PA,0x32);// LED2 电流约 10 mA
	max30102_write_reg(REG_PILOT_PA,0x7f);// Pilot LED 约 25 mA
}

/* 读 6 字节 FIFO：红光/红外各 3 字节，拼成 18 位原始值 */
void max30102_read_fifo(void)
{
  uint16_t un_temp;
  fifo_red=0;
  fifo_ir=0;
  uint8_t ach_i2c_data[6];
  
  // 读状态寄存器，清中断标志
  max30102_read_reg(REG_INTR_STATUS_1);
  max30102_read_reg(REG_INTR_STATUS_2);
  
  ach_i2c_data[0]=REG_FIFO_DATA;
	
	HAL_I2C_Mem_Read(&hi2c1,MAX30102_Device_address,REG_FIFO_DATA,1,ach_i2c_data,6,HAL_MAX_DELAY);
	
  // 三个字节拼 18 位：高 8 位左移 14，中 8 位左移 6，低 8 位右移 2
  un_temp=ach_i2c_data[0];
  un_temp<<=14;
  fifo_red+=un_temp;
  un_temp=ach_i2c_data[1];
  un_temp<<=6;
  fifo_red+=un_temp;
  un_temp=ach_i2c_data[2];
	un_temp>>=2;
  fifo_red+=un_temp;
  
  un_temp=ach_i2c_data[3];
  un_temp<<=14;
  fifo_ir+=un_temp;
  un_temp=ach_i2c_data[4];
  un_temp<<=6;
  fifo_ir+=un_temp;
  un_temp=ach_i2c_data[5];
	un_temp>>=2;
  fifo_ir+=un_temp;
	
	// 无手指时原始值很小，直接清零，避免后级算出假心率
	if(fifo_ir<=10000)
	{
		fifo_ir=0;
	}
	if(fifo_red<=10000)
	{
		fifo_red=0;
	}
}



