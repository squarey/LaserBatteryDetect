
#include "watchdog.h"


//看门狗初始化
void WatchDogInit(void)
{
  //启动看门狗
  IWDG_Enable();
  //开启写看门狗寄存器功能
  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  //设置看门狗分频时钟256分频      4ms
  IWDG_SetPrescaler(IWDG_Prescaler_256);
  //设置看门狗重载值    1s复位
  IWDG_SetReload(0xff);
  //关闭写看门狗寄存器功能
//  IWDG_WriteAccessCmd(IWDG_WriteAccess_Disable);
  
}
//重置看门狗溢出时间
void ReloadWatchDog(void)
{
//  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_ReloadCounter();
//  IWDG_WriteAccessCmd(IWDG_WriteAccess_Disable);
}