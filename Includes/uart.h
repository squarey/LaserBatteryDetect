

#ifndef __UART_H
#define __UART_H

#include "stm8s.h"

#define USART_REC_LEN          30
void UART1_Congfiguration(void);
void PrintfReceivedData(void);
void UART1_SendChar(uint8_t c);
void UART1_SendString(char *str);
//void UART1_SendFloat(float f);
void UART1_SendInt(uint32_t i);
void SetUartReceivedCallback(void *pFuntion);

#endif 

