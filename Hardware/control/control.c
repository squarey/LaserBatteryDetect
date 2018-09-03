

#include "control.h"
#include "uart.h"
#include "adc.h"
#include "timer.h"
#include "delay.h"
#include "temperature_table.h"
#include "watchdog.h"

#define VERSION         2


#define DEF_OPEN_FIRE_TEMPEARTURE               60
#define DEF_TEMPERATURE_STABLE_TIME             200     //  200 * 25ms 5秒
#define DEF_FIRST_OPEN_BATTERY_STABLE_TIME      60      //  60 * 25ms  1.5秒
#define DEF_BATTERY_ERROR_CONFIRM_CNT           3
#define DEF_SPEED_BACK_TIME                     400     //  400 * 25ms 10秒
#define DEF_BATTERY_CONTINUE_DETECT_TIME        40      //  40 * 25ms  1秒
#define DEF_TEMP_CONTINUE_DETECT_TIME           4      //   4 * 25ms  100毫秒
#define DEF_ONE_SECOND_COUNT                    40
#define DEF_SWITCH_SPEED_VAVLE_VALUE            75
#define DEF_CHAGE_BATTERY_AD_REF_VALUE_TIME     40      // 40 * 25ms 1秒

#define DEF_TEMPERATURE_REF_R_VALUE             11000
#define DEF_VCC_DIGIT_VALUE                     1024

#define DEF_UART_RX_MAX_LEN                     15

#define ADC_CHANNEL_BATTERY     4
#define ADC_CHANNEL_TEMPERATURE 3

#define SPEED_HIGH              6
#define SPEED_MID               4
#define SPEED_LOW               2
#define SPEED_NONE              0

//当前ADC检测值
static uint16_t _AdcValue = 0;
//ADC转换完成标志
static bool _FlagAdcConversionEnd = FALSE;
//定时发送时间计数值
static uint8_t _TimingSendDataCount = 0;
//ADC检测通道切换计数值
static uint8_t _AdcSwitchChannelCount = 0;
//前一次ADC通道切换计数值
static uint8_t _LastAdcSwitchChannelCount = 0;
//硅光电池最终值
static uint16_t _BatteryAdcResultValue = 0;
//硅光电池临时值
static uint16_t _BatteryAdcValueTemp = 0;
//硅光电池参考值
static uint16_t _BatteryAdcValueRef = 0;
//改变硅光电池参考值时间确认
static uint16_t _BatteryChangeAdcValueRefCnt = 0;
//硅光电池连续检测次数表示确认值
static uint8_t _BatteryAdcValueCount = 0;
//硅光电池等待稳定时间
static uint8_t _BatteryAdcValueStableCount = 0;
//当前ADC转换通道标记
static uint8_t _AdcSwitchChannel = 0;
//温度检测ADC值
static uint16_t _AdcTemperatureValue = 0;
//硅光电池无检测计数值
static uint16_t _BatteryAdcErrorValueCount = 0;
//当前上报的档位
static uint8_t _CurrentSpeed = 0;
//档位回退时间
static uint16_t _SpeedBackCount = 0;
static uint16_t _SpeedBackCountSet = DEF_SPEED_BACK_TIME;
//是否已经开火
static bool _FireIsOpen = FALSE;
static bool _FocusFireStatus = FALSE;
//当前检测的温度值
static uint8_t _CurrentTemperature = 0;
//开火温度稳定时间
static uint16_t _FireOpenStableCount = 0;
//关火温度稳定时间
static uint16_t _FireCloseStableCount = 0;
//开火 关火温度稳定时间
static uint16_t _FireStableCount = DEF_TEMPERATURE_STABLE_TIME;
//立即发送数据标记
static bool _FlagSendDataNow  = FALSE;
//串口数据接收BUFFER
static uint8_t UartDataRxBuffer[DEF_UART_RX_MAX_LEN];
static uint8_t UartDataRxLen = 0;
static uint8_t FlagUartDataAck = 0;
//开火温度
static uint8_t _OpenFireTemperature = DEF_OPEN_FIRE_TEMPEARTURE;
//硅光电池波动比例
static uint8_t _SwitchSpeedValveValue = DEF_SWITCH_SPEED_VAVLE_VALUE;
//禁止检测温度来判断点火状态
static bool _ForbidenTempOpenFire = FALSE;


//定时器定时时间到回调函数
static void _TimerCallback(void);
//ADC转换完成回调函数
static uint8_t _AdcValueGet(uint16_t Value);
//定时上发检测状态
static void _TimingSendData(void);\
//查表方式获取温度值
static uint8_t CheckTemperatureValue(uint16_t Value);
//串口接收回调函数
static void UartDataGet(uint8_t c);
//处理串口接收到的数据
static void UartDataProcess(void);
//串口数据回复
static bool UartDataAck(void);

//#define SEND_TEMP_AD_VALUE

#ifdef SEND_TEMP_AD_VALUE
static uint16_t TestRValue = 0;
#endif 


void ControlInit(void)
{
  //设置串口数据接收回调
  SetUartReceivedCallback((void *)UartDataGet);
  //初始化串口1
  UART1_Congfiguration();
  //设置定时器定时完成的回调函数
  SetTimerCompleteCallback((void *)_TimerCallback);
  //定时时间为25ms
  Timer2_Init(25000);
  //设置Adc转换结束后的回调函数
  SetAdcConversionEnd_cb((void *)_AdcValueGet);
  //初始化ADC通道4->硅光电池ADC
  _AdcSwitchChannel = ADC_CHANNEL_BATTERY;
  ADC_InitChannel(_AdcSwitchChannel);
}

void ControlPrcoess(void)
{
    //等待ADC转换结束
    if(TRUE == _FlagAdcConversionEnd){
        if(ADC_CHANNEL_BATTERY == _AdcSwitchChannel){
            _BatteryAdcValueCount++;
            //连续检测3此取一次平均值
            if(_BatteryAdcValueCount >= 2){
                _BatteryAdcValueCount = 0;
                _BatteryAdcResultValue = _BatteryAdcValueTemp;
            }
            //如果是第一次检测则直接取值不求平均
            if((0 == _BatteryAdcValueTemp) || (0 == _BatteryAdcResultValue)){
                _BatteryAdcResultValue = _AdcValue;
                _BatteryAdcValueTemp = _AdcValue;
            }else{
                uint32_t Temp = 0;
                // 判断是否有误检测或者干扰现象
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
                //如果检测是正常值 则直接取值
                if(0 == _BatteryAdcErrorValueCount){
                    _BatteryAdcValueTemp = (_BatteryAdcValueTemp + _AdcValue)/2;
                }else if(_BatteryAdcErrorValueCount >= DEF_BATTERY_ERROR_CONFIRM_CNT){
                    _BatteryAdcValueTemp = _AdcValue;
                    _BatteryAdcResultValue = _AdcValue;
                }
            }
            //只有当开火的时候才允许切换档位
            if(_FireIsOpen || _FocusFireStatus){
                uint32_t Temp = 0;
                uint8_t ChangeValue = 0;
                //首次点火延迟1.5s再检测以保证状态稳定
                if(_BatteryAdcValueStableCount < DEF_FIRST_OPEN_BATTERY_STABLE_TIME){
                    _BatteryAdcValueStableCount++;
                    if(_BatteryAdcValueStableCount == DEF_FIRST_OPEN_BATTERY_STABLE_TIME){
                        _BatteryAdcValueRef = _BatteryAdcResultValue;
                    }
                    goto __HERE__;
                }
                
                //开火必须开档位
                if(_CurrentSpeed == SPEED_NONE){
                    _CurrentSpeed = SPEED_LOW;
                    _FlagSendDataNow = TRUE;
                }
                //如果参考值为0 则不作处理
                if(0 == _BatteryAdcValueRef){
                    _BatteryAdcValueStableCount = 0;
                    goto __HERE__;
                }
                //确保此值是稳定的才改变AD参考值
                if(_BatteryAdcValueRef < _BatteryAdcResultValue){
                    _BatteryChangeAdcValueRefCnt++;
                    if(_BatteryChangeAdcValueRefCnt >= DEF_CHAGE_BATTERY_AD_REF_VALUE_TIME){
                        _BatteryAdcValueRef = (_BatteryAdcResultValue + _BatteryAdcValueRef)/2;
                    }
                }else{
                    _BatteryChangeAdcValueRefCnt = 0;
                }
                //判断是否要跳档
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
            //硅光电池连续检测时间为1秒
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
              //温度连续检测时间为100ms
            if(_AdcSwitchChannelCount >= DEF_TEMP_CONTINUE_DETECT_TIME){
                _AdcSwitchChannelCount = 0;
                _AdcSwitchChannel = ADC_CHANNEL_BATTERY;
                ADC_InitChannel(_AdcSwitchChannel);
            }
        }
        //等待火焰稳定时间
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
        //25ms喂一次狗
        ReloadWatchDog();
        _FlagAdcConversionEnd = FALSE;
    }
    //ADC 25ms转换一次
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
    //先判断有没有需要回复的内容再处理串口接收的数据
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
  //查找是否有完整的帧
  for(i = 0; i < UartDataRxLen; i++){
    if((i + 4) >= UartDataRxLen){
      return;
    }else{
       if(UartDataRxBuffer[i] == 0xaa){
        //计算校验位
        for(j = 0; j < 4; j++){
          AdjustValue += UartDataRxBuffer[i + j];
        }
        //校验错误则不处理
        if(AdjustValue == UartDataRxBuffer[i + 4]){
          //如果有正确的帧  则复位串口接收的长度
          UartDataRxLen = 0;
          break;
        }
      }
    }
   
  }
  //获取命令字
  Cmd = UartDataRxBuffer[i + 1];
  //写命令
  if(Cmd & 0x80){
    switch(Cmd & 0x7f){
      //写火焰状态
      case 1:
        if(0 != UartDataRxBuffer[i + 3]){
          _FocusFireStatus = TRUE;
        }else{
          _FocusFireStatus = FALSE;
          _FireIsOpen = FALSE;
        } 
        FlagUartDataAck = 0x82;
      break;
      //写开火温度
      case 2:
        _OpenFireTemperature = UartDataRxBuffer[i + 3];
        FlagUartDataAck = 0x87;
      break;
      //写灵敏度  范围  0 - 100
      case 3:
        _SwitchSpeedValveValue = UartDataRxBuffer[i + 3];
        FlagUartDataAck = 0x88;
      break;
      //禁止检测温度判断是否开火
      case 4:
        _ForbidenTempOpenFire = (0 != UartDataRxBuffer[i + 3]) ? TRUE : FALSE;
        FlagUartDataAck = 0x89;
      break;
      //写档位回退时间
      case 5:
        _SpeedBackCountSet = UartDataRxBuffer[i + 3] * 40;
        FlagUartDataAck = 0x85;
      break;
      //写温度检测开火稳定时间
      case 6:
        _FireStableCount = UartDataRxBuffer[i + 3] * 40;
        FlagUartDataAck = 0x86;
      break;
      default:
      break;
    }
    //读命令
  }else{
    FlagUartDataAck |= 0x80;
    switch(Cmd & 0x7f){
      //读取当前温度
      case 1:
        FlagUartDataAck |= 0x01;
      break;
      //读取当前火焰状态
      case 2:
        FlagUartDataAck |= 0x02;
      break;
      //获取当前ADC参考值
      case 3:
        FlagUartDataAck |= 0x03;
      break;
      //获取当前软件版本号
      case 4:
        FlagUartDataAck |= 0x04;
      break;
      //读取档位回退时间
      case 5:
        FlagUartDataAck |= 0x05;
      break;
      //读取温度开火稳定时间
      case 6:
        FlagUartDataAck |= 0x06;
      break;
      //读开火温度
      case 7:
        FlagUartDataAck |= 0x07;
      break;
      //读灵敏度
      case 8:
        FlagUartDataAck |= 0x08;
      break;
      //读禁止温度检测开火状态
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
      //返温度
      case 0x01:
        AckDataBuf[1] = 0x81;
        AckDataBuf[2] = 0;
        AckDataBuf[3] = _CurrentTemperature;
        //AckDataBuf[2] = (TestRValue >> 8) & 0xff;
        //AckDataBuf[3] = TestRValue & 0xff;
      break;
      //返火焰状态
      case 0x02:
        AckDataBuf[1] = 0x82;
        AckDataBuf[2] = (TRUE == _FireIsOpen) ? 0x01 : 0x00;
        AckDataBuf[3] = (TRUE == _FocusFireStatus) ? 0x01 : 0x00;
      break;
      //返ADC参考值
      case 0x03:
        AckDataBuf[1] = 0x83;
        AckDataBuf[2] = (_BatteryAdcValueRef >> 8) & 0xff;
        AckDataBuf[3] = _BatteryAdcValueRef & 0xff;
      break;
      //返软件版本号
      case 0x04:
        AckDataBuf[1] = 0x84;
        AckDataBuf[2] = (VERSION >> 8) & 0xff;
        AckDataBuf[3] = VERSION & 0xff;
      break;
      //反回档时间
      case 0x05:
        AckDataBuf[1] = 0x85;
        AckDataBuf[2] = 0;
        AckDataBuf[3] = _SpeedBackCountSet/40;
      break;
      //反开火稳定时间
      case 0x06:
        AckDataBuf[1] = 0x86;
        AckDataBuf[2] = 0;
        AckDataBuf[3] = _FireStableCount/40;
      break;
      //反开火温度
      case 0x07:
        AckDataBuf[1] = 0x87;
        AckDataBuf[2] = 0;
        AckDataBuf[3] = _OpenFireTemperature;
      break;
      //反灵敏度
      case 0x08:
        AckDataBuf[1] = 0x88;
        AckDataBuf[2] = 0;
        AckDataBuf[3] = _SwitchSpeedValveValue;
      break;
      //反禁止温度检测开火状态
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
    //计算校验位
    for(i = 0; i < 4; i++){
      AckDataBuf[4] += AckDataBuf[i];
    }
    //发送数据
    for(i = 0; i < 5; i++){
      UART1_SendChar(AckDataBuf[i]);
    }
    FlagUartDataAck = 0;
    return TRUE;
  }
  return FALSE;
}

