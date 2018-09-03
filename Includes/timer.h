

#ifndef __TIMER_H
#define __TIMER_H

#include "stm8s.h"

void Timer2_Init(uint32_t Period);
void SetTimerCompleteCallback(void *pFuntion);

#endif