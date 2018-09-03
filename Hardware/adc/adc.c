


#include "adc.h"
#include "uart.h"
#include "timer.h"

#define INFRARED_VAILD_VALUE    136


typedef uint8_t (*AdcConversionEndCallback)(uint16_t);


static AdcConversionEndCallback _AdcCallback;

void ADC_InitChannel(uint8_t Channels)
{
  /**< ����ת��ģʽ */
  /**< ʹ��ͨ��4 */
  /**< ADCʱ�ӣ�fADC2 = fcpu/18 */
  /**< ���������˴�TIM TRGO ����ת������ʵ����û���õ���*/
  /**  ��ʹ�� ADC2_ExtTriggerState**/
  /**< ת�������Ҷ��� */
  /**< ��ʹ��ͨ��10��˹���ش����� */
  /**  ��ʹ��ͨ��10��˹���ش�����״̬ */
  //��ʼ��I/O PD3Ϊ����  ADCͨ��4
  //         PD2         ADCͨ��3
  //����ADC����״̬
  ADC1_Channel_TypeDef ChannelSelect;
  ADC1_DeInit();
  //����ѡ���ͨ����ʼ����ص�GPIO
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

  //ʹ��ת������ж�
  ADC1_ITConfig(ADC1_IT_EOCIE, ENABLE);
  ADC1_Cmd(ENABLE);
  //��ʼһ��ת��
  ADC1_StartConversion();
}

void StartAdcConversion(void)
{
  ADC1_StartConversion();
}

//����ADCת������һ�κ�Ļص�����
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

