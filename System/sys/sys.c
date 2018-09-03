

#include "sys.h"


void SystemClockInit(void)
{
  //设置系统时钟分频系数为1即系统时钟为16M/1=16M
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
}