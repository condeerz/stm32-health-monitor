/**
 * ************************************************************************
 * 
 * @file blood.h
 * @author 徐煜果
 * @brief 心率/血氧计算接口
 * 
 * ************************************************************************
 */
#ifndef _BLOOD_H
#define _BLOOD_H

#include "main.h"
#include "MAX30102.h"
#include "algorithm.h"
#include "math.h"

void blood_data_translate(void);
void blood_data_update(void);
void blood_Loop(void);
void blood_Loop_nonblock(void);

#endif



