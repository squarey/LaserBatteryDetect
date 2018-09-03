

#include "timer.h"
#include "uart.h"
#include "stm8s.h"

#define PWM_PERIOD      1000            //PWM的周期
//#define PWM_DUTY        250             //PWM的占空比  PWM_DUTY/PWM_PERIOD

typedef void (*TimerCallback)(void);

//定时时间到的回调函数
static TimerCallback _TimerCompleteCallback;


//定时器2初始化  Period:定时的时间  单位ms  
void Timer2_Init(uint32_t Period)
{
  //设置定时器2的时钟分频系数以及溢出时间
  //系统时钟16M进行16分频 溢出值设置为1000（实际值为999） 则PWM的周期为1ms
//  OPT2 |= 0x02;
  TIM2_TimeBaseInit(TIM2_PRESCALER_16, Period - 1);
  //设置定时器2预分频值
  TIM2_PrescalerConfig(TIM2_PRESCALER_16, TIM2_PSCRELOADMODE_UPDATE);
  TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
  //配置定时器2的PWM模式
//  TIM2_OC1Init(TIM2_OCMODE_PWM1, TIM2_OUTPUTSTATE_ENABLE,PWM_DUTY, TIM2_OCPOLARITY_HIGH);
  //使能PWM,并输出
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