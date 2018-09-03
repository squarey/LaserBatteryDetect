
#include "watchdog.h"


//���Ź���ʼ��
void WatchDogInit(void)
{
  //�������Ź�
  IWDG_Enable();
  //����д���Ź��Ĵ�������
  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  //���ÿ��Ź���Ƶʱ��256��Ƶ      4ms
  IWDG_SetPrescaler(IWDG_Prescaler_256);
  //���ÿ��Ź�����ֵ    1s��λ
  IWDG_SetReload(0xff);
  //�ر�д���Ź��Ĵ�������
//  IWDG_WriteAccessCmd(IWDG_WriteAccess_Disable);
  
}
//���ÿ��Ź����ʱ��
void ReloadWatchDog(void)
{
//  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_ReloadCounter();
//  IWDG_WriteAccessCmd(IWDG_WriteAccess_Disable);
}