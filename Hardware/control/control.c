

#include "control.h"
#include "uart.h"
#include "adc.h"
#include "timer.h"
#include "delay.h"
#include "temperature_table.h"
#include "watchdog.h"

#define VERSION         2


#define DEF_OPEN_FIRE_TEMPEARTURE               60
#define DEF_TEMPERATURE_STABLE_TIME             200     //  200 * 25ms 5��
#define DEF_FIRST_OPEN_BATTERY_STABLE_TIME      60      //  60 * 25ms  1.5��
#define DEF_BATTERY_ERROR_CONFIRM_CNT           3
#define DEF_SPEED_BACK_TIME                     400     //  400 * 25ms 10��
#define DEF_BATTERY_CONTINUE_DETECT_TIME        40      //  40 * 25ms  1��
#define DEF_TEMP_CONTINUE_DETECT_TIME           4      //   4 * 25ms  100����
#define DEF_ONE_SECOND_COUNT                    40
#define DEF_SWITCH_SPEED_VAVLE_VALUE            75
#define DEF_CHAGE_BATTERY_AD_REF_VALUE_TIME     40      // 40 * 25ms 1��

#define DEF_TEMPERATURE_REF_R_VALUE             11000
#define DEF_VCC_DIGIT_VALUE                     1024

#define DEF_UART_RX_MAX_LEN                     15

#define ADC_CHANNEL_BATTERY     4
#define ADC_CHANNEL_TEMPERATURE 3

#define SPEED_HIGH              6
#define SPEED_MID               4
#define SPEED_LOW               2
#define SPEED_NONE              0

//��ǰADC���ֵ
static uint16_t _AdcValue = 0;
//ADCת����ɱ�־
static bool _FlagAdcConversionEnd = FALSE;
//��ʱ����ʱ�����ֵ
static uint8_t _TimingSendDataCount = 0;
//ADC���ͨ���л�����ֵ
static uint8_t _AdcSwitchChannelCount = 0;
//ǰһ��ADCͨ���л�����ֵ
static uint8_t _LastAdcSwitchChannelCount = 0;
//���������ֵ
static uint16_t _BatteryAdcResultValue = 0;
//�������ʱֵ
static uint16_t _BatteryAdcValueTemp = 0;
//����زο�ֵ
static uint16_t _BatteryAdcValueRef = 0;
//�ı����زο�ֵʱ��ȷ��
static uint16_t _BatteryChangeAdcValueRefCnt = 0;
//�����������������ʾȷ��ֵ
static uint8_t _BatteryAdcValueCount = 0;
//����صȴ��ȶ�ʱ��
static uint8_t _BatteryAdcValueStableCount = 0;
//��ǰADCת��ͨ�����
static uint8_t _AdcSwitchChannel = 0;
//�¶ȼ��ADCֵ
static uint16_t _AdcTemperatureValue = 0;
//������޼�����ֵ
static uint16_t _BatteryAdcErrorValueCount = 0;
//��ǰ�ϱ��ĵ�λ
static uint8_t _CurrentSpeed = 0;
//��λ����ʱ��
static uint16_t _SpeedBackCount = 0;
static uint16_t _SpeedBackCountSet = DEF_SPEED_BACK_TIME;
//�Ƿ��Ѿ�����
static bool _FireIsOpen = FALSE;
static bool _FocusFireStatus = FALSE;
//��ǰ�����¶�ֵ
static uint8_t _CurrentTemperature = 0;
//�����¶��ȶ�ʱ��
static uint16_t _FireOpenStableCount = 0;
//�ػ��¶��ȶ�ʱ��
static uint16_t _FireCloseStableCount = 0;
//���� �ػ��¶��ȶ�ʱ��
static uint16_t _FireStableCount = DEF_TEMPERATURE_STABLE_TIME;
//�����������ݱ��
static bool _FlagSendDataNow  = FALSE;
//�������ݽ���BUFFER
static uint8_t UartDataRxBuffer[DEF_UART_RX_MAX_LEN];
static uint8_t UartDataRxLen = 0;
static uint8_t FlagUartDataAck = 0;
//�����¶�
static uint8_t _OpenFireTemperature = DEF_OPEN_FIRE_TEMPEARTURE;
//����ز�������
static uint8_t _SwitchSpeedValveValue = DEF_SWITCH_SPEED_VAVLE_VALUE;
//��ֹ����¶����жϵ��״̬
static bool _ForbidenTempOpenFire = FALSE;


//��ʱ����ʱʱ�䵽�ص�����
static void _TimerCallback(void);
//ADCת����ɻص�����
static uint8_t _AdcValueGet(uint16_t Value);
//��ʱ�Ϸ����״̬
static void _TimingSendData(void);\
//���ʽ��ȡ�¶�ֵ
static uint8_t CheckTemperatureValue(uint16_t Value);
//���ڽ��ջص�����
static void UartDataGet(uint8_t c);
//�����ڽ��յ�������
static void UartDataProcess(void);
//�������ݻظ�
static bool UartDataAck(void);

//#define SEND_TEMP_AD_VALUE

#ifdef SEND_TEMP_AD_VALUE
static uint16_t TestRValue = 0;
#endif 


void ControlInit(void)
{
  //���ô������ݽ��ջص�
  SetUartReceivedCallback((void *)UartDataGet);
  //��ʼ������1
  UART1_Congfiguration();
  //���ö�ʱ����ʱ��ɵĻص�����
  SetTimerCompleteCallback((void *)_TimerCallback);
  //��ʱʱ��Ϊ25ms
  Timer2_Init(25000);
  //����Adcת��������Ļص�����
  SetAdcConversionEnd_cb((void *)_AdcValueGet);
  //��ʼ��ADCͨ��4->�����ADC
  _AdcSwitchChannel = ADC_CHANNEL_BATTERY;
  ADC_InitChannel(_AdcSwitchChannel);
}

void ControlPrcoess(void)
{
    //�ȴ�ADCת������
    if(TRUE == _FlagAdcConversionEnd){
        if(ADC_CHANNEL_BATTERY == _AdcSwitchChannel){
            _BatteryAdcValueCount++;
            //�������3��ȡһ��ƽ��ֵ
            if(_BatteryAdcValueCount >= 2){
                _BatteryAdcValueCount = 0;
                _BatteryAdcResultValue = _BatteryAdcValueTemp;
            }
            //����ǵ�һ�μ����ֱ��ȡֵ����ƽ��
            if((0 == _BatteryAdcValueTemp) || (0 == _BatteryAdcResultValue)){
                _BatteryAdcResultValue = _AdcValue;
                _BatteryAdcValueTemp = _AdcValue;
            }else{
                uint32_t Temp = 0;
                // �ж��Ƿ���������߸�������
                if(_AdcValue >= _BatteryAdcResultValue){
                    Temp = ((uint32_t)(_AdcValue - _BatteryAdcResultValue)) * 100;
                    if(Temp/_BatteryAdcResultValue > 50){
                        _BatteryAdcErrorValueCount++;
                        _BatteryAdcValueCount = 0;
                    }else{
                        _BatteryAdcErrorValueCount = 0;
                    }
                }else{
                    Temp = ((uint32_t)(_BatteryAdcResultValue - _AdcValue)) * 100;
                    if(Temp/_BatteryAdcResultValue > 50){
                        _BatteryAdcErrorValueCount++;
                        _BatteryAdcValueCount = 0;
                    }else{
                        _BatteryAdcErrorValueCount = 0;
                    }
                }
                //������������ֵ ��ֱ��ȡֵ
                if(0 == _BatteryAdcErrorValueCount){
                    _BatteryAdcValueTemp = (_BatteryAdcValueTemp + _AdcValue)/2;
                }else if(_BatteryAdcErrorValueCount >= DEF_BATTERY_ERROR_CONFIRM_CNT){
                    _BatteryAdcValueTemp = _AdcValue;
                    _BatteryAdcResultValue = _AdcValue;
                }
            }
            //ֻ�е������ʱ��������л���λ
            if(_FireIsOpen || _FocusFireStatus){
                uint32_t Temp = 0;
                uint8_t ChangeValue = 0;
                //�״ε���ӳ�1.5s�ټ���Ա�֤״̬�ȶ�
                if(_BatteryAdcValueStableCount < DEF_FIRST_OPEN_BATTERY_STABLE_TIME){
                    _BatteryAdcValueStableCount++;
                    if(_BatteryAdcValueStableCount == DEF_FIRST_OPEN_BATTERY_STABLE_TIME){
                        _BatteryAdcValueRef = _BatteryAdcResultValue;
                    }
                    goto __HERE__;
                }
                
                //������뿪��λ
                if(_CurrentSpeed == SPEED_NONE){
                    _CurrentSpeed = SPEED_LOW;
                    _FlagSendDataNow = TRUE;
                }
                //����ο�ֵΪ0 ��������
                if(0 == _BatteryAdcValueRef){
                    _BatteryAdcValueStableCount = 0;
                    goto __HERE__;
                }
                //ȷ����ֵ���ȶ��ĲŸı�AD�ο�ֵ
                if(_BatteryAdcValueRef < _BatteryAdcResultValue){
                    _BatteryChangeAdcValueRefCnt++;
                    if(_BatteryChangeAdcValueRefCnt >= DEF_CHAGE_BATTERY_AD_REF_VALUE_TIME){
                        _BatteryAdcValueRef = (_BatteryAdcResultValue + _BatteryAdcValueRef)/2;
                    }
                }else{
                    _BatteryChangeAdcValueRefCnt = 0;
                }
                //�ж��Ƿ�Ҫ����
                ChangeValue = 100;
                if(_BatteryAdcResultValue  < _BatteryAdcValueRef){
                    Temp = ((uint32_t)(_BatteryAdcValueRef - _BatteryAdcResultValue)) * 100;
                    Temp = 100 - Temp/_BatteryAdcValueRef;
                    ChangeValue = Temp & 0xff;
                }
                if(ChangeValue < _SwitchSpeedValveValue){
                    if(SPEED_HIGH != _CurrentSpeed){
                        _FlagSendDataNow = TRUE;
                    }
                    _CurrentSpeed = SPEED_HIGH;
                    _SpeedBackCount = 0;
                }else{
                    if(_SpeedBackCount < _SpeedBackCountSet){
                        _SpeedBackCount++;
                    }else{
                        _SpeedBackCount = 0;
                        switch(_CurrentSpeed){
                            case SPEED_LOW:
                            break;
                            case SPEED_MID:
                                _CurrentSpeed = SPEED_LOW;
                                _FlagSendDataNow = TRUE;
                            break;
                            case SPEED_HIGH:
                                _CurrentSpeed = SPEED_MID;
                                _FlagSendDataNow = TRUE;
                            break;
                        }
                    }
                }
            }else{
                _CurrentSpeed = SPEED_NONE;
                _BatteryAdcValueRef = 0;
                _BatteryAdcValueStableCount = 0;
                _BatteryChangeAdcValueRefCnt = 0;
                _SpeedBackCount = 0;
            }
__HERE__:
            //������������ʱ��Ϊ1��
            if(_AdcSwitchChannelCount >= DEF_BATTERY_CONTINUE_DETECT_TIME){
                _AdcSwitchChannelCount = 0;
                _AdcSwitchChannel = ADC_CHANNEL_TEMPERATURE;
                ADC_InitChannel(_AdcSwitchChannel);
            }
        }else if(ADC_CHANNEL_TEMPERATURE == _AdcSwitchChannel){
            if(0 == _AdcTemperatureValue){
                _AdcTemperatureValue = _AdcValue;
            }else{
                _AdcTemperatureValue = (_AdcTemperatureValue + _AdcValue)/2;
            }
            _CurrentTemperature = CheckTemperatureValue(_AdcTemperatureValue);
              //�¶��������ʱ��Ϊ100ms
            if(_AdcSwitchChannelCount >= DEF_TEMP_CONTINUE_DETECT_TIME){
                _AdcSwitchChannelCount = 0;
                _AdcSwitchChannel = ADC_CHANNEL_BATTERY;
                ADC_InitChannel(_AdcSwitchChannel);
            }
        }
        //�ȴ������ȶ�ʱ��
       if(_CurrentTemperature >= _OpenFireTemperature){
              _FireCloseStableCount = 0;
              if(_FireOpenStableCount >= _FireStableCount){
                  _FireIsOpen = TRUE;
              }else{
                  _FireOpenStableCount++;
              }   
        }else{
            _FireOpenStableCount = 0;
            if(_FireCloseStableCount >= _FireStableCount){
                _FireIsOpen = FALSE;
            }else{
                _FireCloseStableCount++;
            }  
        }
        if(TRUE == _ForbidenTempOpenFire){
          _FireIsOpen = FALSE;
        }
        //25msιһ�ι�
        ReloadWatchDog();
        _FlagAdcConversionEnd = FALSE;
    }
    //ADC 25msת��һ��
    if(_LastAdcSwitchChannelCount != _AdcSwitchChannelCount){
        _LastAdcSwitchChannelCount = _AdcSwitchChannelCount;
        StartAdcConversion();
    }
    if(TRUE == UartDataAck()){
      delay_ms(5);
    }else if((TRUE == _FlagSendDataNow) || (_TimingSendDataCount >= DEF_ONE_SECOND_COUNT)){
        _FlagSendDataNow = FALSE;
        _TimingSendDataCount = 0;
        _TimingSendData();
        delay_ms(5);
    }
    //���ж���û����Ҫ�ظ��������ٴ����ڽ��յ�����
    UartDataProcess();
}

static uint8_t _AdcValueGet(uint16_t Value)
{
  _AdcValue = Value;
  _FlagAdcConversionEnd = TRUE;
  return 0;
}

static void _TimerCallback(void)
{
  _AdcSwitchChannelCount++;
  _TimingSendDataCount++;
}



static void _TimingSendData(void)
{
  uint16_t AdjustValue = 0;
  uint8_t i = 0;
  uint8_t SendDataBuffer[10] = {0};
  SendDataBuffer[0] = 0x55;
  SendDataBuffer[1] = 0x55;
  SendDataBuffer[2] = 0x07;
  SendDataBuffer[3] = _CurrentSpeed;
  SendDataBuffer[4] = (_BatteryAdcResultValue >> 8) & 0xff;
  SendDataBuffer[5] = _BatteryAdcResultValue & 0xff;
#ifdef SEND_TEMP_AD_VALUE
  SendDataBuffer[6] = (TestRValue >> 8) & 0xff;
  SendDataBuffer[7] = TestRValue & 0xff;
#else 
  SendDataBuffer[6] = 0;
  SendDataBuffer[7] = 0;
#endif 
  for(i = 0; i < 8; i++){
    AdjustValue += SendDataBuffer[i];
  }
  SendDataBuffer[8] = (AdjustValue >> 8) & 0xff;
  SendDataBuffer[9] = AdjustValue & 0xff;
  for(i = 0; i < 10; i++){
    UART1_SendChar(SendDataBuffer[i]);
  }
}



static uint8_t CheckTemperatureValue(uint16_t Value)
{
    uint8_t i = 0;
    uint8_t Temp = 0;
    uint32_t RValue = 0;
    RValue = DEF_TEMPERATURE_REF_R_VALUE * (uint32_t)Value;
    //RValue = (Value * 10000)/(4826 - Value);
    RValue = RValue/(DEF_VCC_DIGIT_VALUE - Value);
#ifdef SEND_TEMP_AD_VALUE
    TestRValue = RValue;
#endif 
    if(RValue <= DEF_TEMPERATURE_MIN_RESISTANCE){
        Temp = 99;
    }  
    for(i = 0; i < 100; i++){
        if(RValue > TemperatureTable[i]){
            /*if(i >= 1){
              Temp = i - 1;
            }*/
            Temp = i;
            break;
        }
    }
    return Temp;
}

//static bool _FlagStartStorageData = FALSE;
static void UartDataGet(uint8_t c)
{
  UartDataRxBuffer[UartDataRxLen] = c;
  UartDataRxLen++;
  if(UartDataRxLen >= DEF_UART_RX_MAX_LEN){
    UartDataRxLen = 0;
  }
}

static void UartDataProcess(void)
{
  uint8_t i = 0, j = 0;
  uint8_t Cmd = 0;
  uint8_t AdjustValue = 0;
  if(UartDataRxLen < 5){
    return;
  }
  //�����Ƿ���������֡
  for(i = 0; i < UartDataRxLen; i++){
    if((i + 4) >= UartDataRxLen){
      return;
    }else{
       if(UartDataRxBuffer[i] == 0xaa){
        //����У��λ
        for(j = 0; j < 4; j++){
          AdjustValue += UartDataRxBuffer[i + j];
        }
        //У������򲻴���
        if(AdjustValue == UartDataRxBuffer[i + 4]){
          //�������ȷ��֡  ��λ���ڽ��յĳ���
          UartDataRxLen = 0;
          break;
        }
      }
    }
   
  }
  //��ȡ������
  Cmd = UartDataRxBuffer[i + 1];
  //д����
  if(Cmd & 0x80){
    switch(Cmd & 0x7f){
      //д����״̬
      case 1:
        if(0 != UartDataRxBuffer[i + 3]){
          _FocusFireStatus = TRUE;
        }else{
          _FocusFireStatus = FALSE;
          _FireIsOpen = FALSE;
        } 
        FlagUartDataAck = 0x82;
      break;
      //д�����¶�
      case 2:
        _OpenFireTemperature = UartDataRxBuffer[i + 3];
        FlagUartDataAck = 0x87;
      break;
      //д������  ��Χ  0 - 100
      case 3:
        _SwitchSpeedValveValue = UartDataRxBuffer[i + 3];
        FlagUartDataAck = 0x88;
      break;
      //��ֹ����¶��ж��Ƿ񿪻�
      case 4:
        _ForbidenTempOpenFire = (0 != UartDataRxBuffer[i + 3]) ? TRUE : FALSE;
        FlagUartDataAck = 0x89;
      break;
      //д��λ����ʱ��
      case 5:
        _SpeedBackCountSet = UartDataRxBuffer[i + 3] * 40;
        FlagUartDataAck = 0x85;
      break;
      //д�¶ȼ�⿪���ȶ�ʱ��
      case 6:
        _FireStableCount = UartDataRxBuffer[i + 3] * 40;
        FlagUartDataAck = 0x86;
      break;
      default:
      break;
    }
    //������
  }else{
    FlagUartDataAck |= 0x80;
    switch(Cmd & 0x7f){
      //��ȡ��ǰ�¶�
      case 1:
        FlagUartDataAck |= 0x01;
      break;
      //��ȡ��ǰ����״̬
      case 2:
        FlagUartDataAck |= 0x02;
      break;
      //��ȡ��ǰADC�ο�ֵ
      case 3:
        FlagUartDataAck |= 0x03;
      break;
      //��ȡ��ǰ����汾��
      case 4:
        FlagUartDataAck |= 0x04;
      break;
      //��ȡ��λ����ʱ��
      case 5:
        FlagUartDataAck |= 0x05;
      break;
      //��ȡ�¶ȿ����ȶ�ʱ��
      case 6:
        FlagUartDataAck |= 0x06;
      break;
      //�������¶�
      case 7:
        FlagUartDataAck |= 0x07;
      break;
      //��������
      case 8:
        FlagUartDataAck |= 0x08;
      break;
      //����ֹ�¶ȼ�⿪��״̬
      case 9:
        FlagUartDataAck |= 0x09;
      break;
      default:
      break;
    }
  }
}

static bool UartDataAck(void)
{
  if(FlagUartDataAck & 0x80){
    uint8_t i = 0;
    uint8_t AckDataBuf[5] = {0};
    AckDataBuf[0] = 0xaa;
    switch(FlagUartDataAck & 0x7f){
      //���¶�
      case 0x01:
        AckDataBuf[1] = 0x81;
        AckDataBuf[2] = 0;
        AckDataBuf[3] = _CurrentTemperature;
        //AckDataBuf[2] = (TestRValue >> 8) & 0xff;
        //AckDataBuf[3] = TestRValue & 0xff;
      break;
      //������״̬
      case 0x02:
        AckDataBuf[1] = 0x82;
        AckDataBuf[2] = (TRUE == _FireIsOpen) ? 0x01 : 0x00;
        AckDataBuf[3] = (TRUE == _FocusFireStatus) ? 0x01 : 0x00;
      break;
      //��ADC�ο�ֵ
      case 0x03:
        AckDataBuf[1] = 0x83;
        AckDataBuf[2] = (_BatteryAdcValueRef >> 8) & 0xff;
        AckDataBuf[3] = _BatteryAdcValueRef & 0xff;
      break;
      //������汾��
      case 0x04:
        AckDataBuf[1] = 0x84;
        AckDataBuf[2] = (VERSION >> 8) & 0xff;
        AckDataBuf[3] = VERSION & 0xff;
      break;
      //���ص�ʱ��
      case 0x05:
        AckDataBuf[1] = 0x85;
        AckDataBuf[2] = 0;
        AckDataBuf[3] = _SpeedBackCountSet/40;
      break;
      //�������ȶ�ʱ��
      case 0x06:
        AckDataBuf[1] = 0x86;
        AckDataBuf[2] = 0;
        AckDataBuf[3] = _FireStableCount/40;
      break;
      //�������¶�
      case 0x07:
        AckDataBuf[1] = 0x87;
        AckDataBuf[2] = 0;
        AckDataBuf[3] = _OpenFireTemperature;
      break;
      //��������
      case 0x08:
        AckDataBuf[1] = 0x88;
        AckDataBuf[2] = 0;
        AckDataBuf[3] = _SwitchSpeedValveValue;
      break;
      //����ֹ�¶ȼ�⿪��״̬
      case 0x09:
        AckDataBuf[1] = 0x89;
        AckDataBuf[2] = 0;
        AckDataBuf[3] = (TRUE == _ForbidenTempOpenFire) ? 1 : 0;
      break;
      default:
        FlagUartDataAck = 0;
        return FALSE;
      break;
    }
    //����У��λ
    for(i = 0; i < 4; i++){
      AckDataBuf[4] += AckDataBuf[i];
    }
    //��������
    for(i = 0; i < 5; i++){
      UART1_SendChar(AckDataBuf[i]);
    }
    FlagUartDataAck = 0;
    return TRUE;
  }
  return FALSE;
}

