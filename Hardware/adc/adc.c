


#include "adc.h"
#include "uart.h"
#include "timer.h"

#define INFRARED_VAILD_VALUE    136


typedef uint8_t (*AdcConversionEndCallback)(uint16_t);


static AdcConversionEndCallback _AdcCallback;

void ADC_InitChannel(uint8_t Channels)
{
  /**< 单次转换模式 */
  /**< 使能通道4 */
  /**< ADC时钟：fADC2 = fcpu/18 */
  /**< 这里设置了从TIM TRGO 启动转换，但实际是没有用到的*/
  /**  不使能 ADC2_ExtTriggerState**/
  /**< 转换数据右对齐 */
  /**< 不使能通道10的斯密特触发器 */
  /**  不使能通道10的斯密特触发器状态 */
  //初始化I/O PD3为输入  ADC通道4
  //         PD2         ADC通道3
  //重置ADC所有状态
  ADC1_Channel_TypeDef ChannelSelect;
  ADC1_DeInit();
  //根据选择的通道初始化相关的GPIO
  switch(Channels){
    case 3:
      ChannelSelect = ADC1_CHANNEL_3;
      GPIO_Init(GPIOD, GPIO_PIN_2, GPIO_MODE_IN_FL_NO_IT);
    break;
    case 4:
      ChannelSelect = ADC1_CHANNEL_4;
      GPIO_Init(GPIOD, GPIO_PIN_3, GPIO_MODE_IN_FL_NO_IT);
    break;
  }
  ADC1_Init(ADC1_CONVERSIONMODE_SINGLE, ChannelSelect, ADC1_PRESSEL_FCPU_D2, 
            ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, ADC1_SCHMITTTRIG_CHANNEL3, DISABLE);

  //使能转换完成中断
  ADC1_ITConfig(ADC1_IT_EOCIE, ENABLE);
  ADC1_Cmd(ENABLE);
  //开始一次转换
  ADC1_StartConversion();
}

void StartAdcConversion(void)
{
  ADC1_StartConversion();
}

//设置ADC转换结束一次后的回调函数
void SetAdcConversionEnd_cb(void *pFunction)
{
  _AdcCallback = (AdcConversionEndCallback)pFunction;
}


INTERRUPT_HANDLER(ADC1_IRQHandler, 22)
{
  /* In order to detect unexpected events during development,
     it is recommended to set a breakpoint on the following instruction.
  */
  ADC1_ClearFlag(ADC1_FLAG_EOC);
  if(_AdcCallback){
    if(0 !=_AdcCallback(ADC1_GetConversionValue())){
      ADC1_StartConversion();
    }
  }
}

