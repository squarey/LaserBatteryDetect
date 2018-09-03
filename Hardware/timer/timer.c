

#include "timer.h"
#include "uart.h"
#include "stm8s.h"

#define PWM_PERIOD      1000            //PWM������
//#define PWM_DUTY        250             //PWM��ռ�ձ�  PWM_DUTY/PWM_PERIOD

typedef void (*TimerCallback)(void);

//��ʱʱ�䵽�Ļص�����
static TimerCallback _TimerCompleteCallback;


//��ʱ��2��ʼ��  Period:��ʱ��ʱ��  ��λms  
void Timer2_Init(uint32_t Period)
{
  //���ö�ʱ��2��ʱ�ӷ�Ƶϵ���Լ����ʱ��
  //ϵͳʱ��16M����16��Ƶ ���ֵ����Ϊ1000��ʵ��ֵΪ999�� ��PWM������Ϊ1ms
//  OPT2 |= 0x02;
  TIM2_TimeBaseInit(TIM2_PRESCALER_16, Period - 1);
  //���ö�ʱ��2Ԥ��Ƶֵ
  TIM2_PrescalerConfig(TIM2_PRESCALER_16, TIM2_PSCRELOADMODE_UPDATE);
  TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
  //���ö�ʱ��2��PWMģʽ
//  TIM2_OC1Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE,PWM_DUTY, TIM2_OCPOLARITY_HIGH);
  //ʹ��PWM,�����
//  TIM2_OC1PreloadConfig(ENABLE); 
  
//  TIM2_OC3Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE,PWM_DUTY, TIM2_OCPOLARITY_HIGH);
//  TIM2_OC3PreloadConfig(ENABLE); 
  
  TIM2_Cmd(ENABLE);
}

void SetTimerCompleteCallback(void *pFuntion)
{
  _TimerCompleteCallback = (TimerCallback)pFuntion;
}

INTERRUPT_HANDLER(TIM2_UPD_OVF_BRK_IRQHandler, 13)
{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
  TIM2_ClearFlag(TIM2_FLAG_UPDATE);
  if(_TimerCompleteCallback){
    _TimerCompleteCallback();
  }

}