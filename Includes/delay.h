

#ifndef __DELAY_H
#define __DELAY_H

#include "stm8s.h"

void delay_init(uint8_t clk); //��ʱ������ʼ��
void delay_us(uint16_t xus);  //us����ʱ����,���65536us.
void delay_ms(uint32_t xms);  //ms����ʱ����


#endif