

#ifndef __DELAY_H
#define __DELAY_H

#include "stm8s.h"

void delay_init(uint8_t clk); //延时函数初始化
void delay_us(uint16_t xus);  //us级延时函数,最大65536us.
void delay_ms(uint32_t xms);  //ms级延时函数


#endif