

#include "sys.h"


void SystemClockInit(void)
{
  //����ϵͳʱ�ӷ�Ƶϵ��Ϊ1��ϵͳʱ��Ϊ16M/1=16M
  CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
}