

#include "control.h"
#include "uart.h"
#include "adc.h"
#include "timer.h"
#include "delay.h"
#include "temperature_table.h"
#include "watchdog.h"

#define VERSION         2



#define DEF_SENSITIVITY_VALUE					85		//默认灵敏度
#define DEF_BACK_TIME_VALUE				        8		//默认档位回退时间  8秒
#define DEF_ONE_SECOND_CNT						50		//一秒钟所需的计数值 
#define DEF_TIMING_SEND_DATA_CNT				2		//一秒钟所需的计数值 

#define DEF_TEMPERATURE_REF_R_VALUE             11000
#define DEF_VCC_DIGIT_VALUE                     1024

#define DEF_UART_RX_MAX_LEN                     21

#define ADC_CHANNEL_BATTERY                     4
#define ADC_CHANNEL_TEMPERATURE                 3

#define SMOKE_LARGE								2
#define SMOKE_SAMLL								1
#define SMOKE_NONE								0

#define USE_GPIO_CONTROL_STATUS					1

//当前ADC检测值
static uint16_t _AdcValue = 0;
//ADC转换完成标志
static bool _FlagAdcConversionEnd = FALSE;
//定时发送时间计数值
static uint8_t _TimingSendDataCount = 0;
//立即发送数据标记
static bool _FlagSendDataNow  = FALSE;
//串口数据接收BUFFER
static uint8_t _UartDataRxBuffer[DEF_UART_RX_MAX_LEN];
static uint8_t _UartDataRxLen = 0;


//当前设置的电源状态
static bool _PowerSetStatus = FALSE;
//当前设置的灵敏度
static uint8_t _SensitivitySetValue = DEF_SENSITIVITY_VALUE;
//当前设置的档位回退时间
static uint8_t _BackTimeSetValue = DEF_BACK_TIME_VALUE;
//当前串口数据需要返回
static bool _FlagUartDataAck = FALSE;
//当前烟雾状态
static uint8_t _FlagSmokeStatus = FALSE;
//当前无烟雾状态的AD值
static uint16_t _BatteryBaseADValue = 0;
//上一次检测电池的AD值
static uint16_t _LastBatteryADValue = 0;
//上一次检测电池的AD值
static uint16_t _BatteryADStorageValue = 0;
//检测的AD值抖动状态
static uint8_t _BatteryADValueShakeCnt = 0;
//第一次开启电源时的校准
static uint8_t _FlagStartStableTimeCnt = 0;
//校准时间计数器
static uint16_t _StableTimeCnt = 0;
//没有烟雾状态
static bool _FlagNoSmokeStatus = FALSE;
//没有烟雾状态时间
static uint16_t _NoSmokeStatusCnt = 0;

//定时器定时时间到回调函数
static void _TimerCallback(void);
//ADC转换完成回调函数
static uint8_t _AdcValueGet(uint16_t Value);
//定时上发检测状态
static void _TimingSendData(void);
//查表方式获取温度值
//static uint8_t CheckTemperatureValue(uint16_t Value);
//串口接收回调函数
static void _UartDataGet(uint8_t c);
//处理串口接收到的数据
static void _UartDataProcess(void);
//串口数据回复
static bool _UartDataAck(void);
//是否有烟雾状态判断
static void _CompareSmokeStatus(uint16_t CurValue);

#if USE_GPIO_CONTROL_STATUS
static void _GPIO_ControlInit(void);
static void _RefreshGPIOStatus(void);
#endif 

void ControlInit(void)
{
	//设置串口数据接收回调
	SetUartReceivedCallback((void *)_UartDataGet);
	//初始化串口1
	UART1_Congfiguration();
	//设置定时器定时完成的回调函数
	SetTimerCompleteCallback((void *)_TimerCallback);
	//定时时间为20ms
	Timer2_Init(20000);
	//设置Adc转换结束后的回调函数
	SetAdcConversionEnd_cb((void *)_AdcValueGet);
	//初始化ADC通道4->硅光电池ADC
	ADC_InitChannel(ADC_CHANNEL_BATTERY);
#if USE_GPIO_CONTROL_STATUS
	_GPIO_ControlInit();
#endif 
}

void ControlPrcoess(void)
{
	//等待ADC转换结束
	if(TRUE == _FlagAdcConversionEnd){
		_FlagAdcConversionEnd = FALSE;
		if(TRUE == _PowerSetStatus){
			_CompareSmokeStatus(_AdcValue);
		}else{
			_BatteryADStorageValue = _AdcValue;
			_LastBatteryADValue = 0;
		}
		StartAdcConversion();
	}
#if USE_GPIO_CONTROL_STATUS
	_RefreshGPIOStatus();
#endif 
	if(TRUE == _UartDataAck()){
		delay_ms(5);
	}else if((TRUE == _FlagSendDataNow) || (_TimingSendDataCount >= DEF_TIMING_SEND_DATA_CNT)){
		_FlagSendDataNow = FALSE;
		_TimingSendDataCount = 0;
		_TimingSendData();
		delay_ms(5);
	}
	//先判断有没有需要回复的内容再处理串口接收的数据
	_UartDataProcess();
	ReloadWatchDog();
}
static uint16_t _SubtractionResultABS(int16_t Value1, int16_t Value2)
{
	int16_t Diff = 0;
	Diff = Value1 - Value2;
	if(Diff < 0){
		return -Diff;
	}else{
		return Diff;
	}
}
//是否有烟雾状态判断
static void _CompareSmokeStatus(uint16_t CurValue)
{
	if(0 != _LastBatteryADValue){
		uint16_t Temp = 0;
		Temp = _SubtractionResultABS((int16_t)CurValue, (int16_t)_LastBatteryADValue);
		Temp *= 100;
		//防止检测的AD值过度抖动
		if(Temp/_LastBatteryADValue >= 60){
			_BatteryADValueShakeCnt++;
			//如果连续4次检测都是相近的值 则说明检测值没有问题
			if(_BatteryADValueShakeCnt < 3){
				return;
			}
		}
	}
	_BatteryADValueShakeCnt = 0;
	_LastBatteryADValue = CurValue;
	_BatteryADStorageValue = _LastBatteryADValue;
	//第一次开启电源校准
	if(1 == _FlagStartStableTimeCnt){
		_BatteryBaseADValue += CurValue;
		_BatteryBaseADValue /= 2;
	}else{
		//过滤微抖动状态
		uint16_t DiffValue = 0;
		DiffValue = _SubtractionResultABS((int16_t)CurValue, (int16_t)_BatteryBaseADValue);
		if((DiffValue < 15) || (CurValue >= _BatteryBaseADValue)){
			_BatteryBaseADValue += CurValue;
			_BatteryBaseADValue /= 2;
			_FlagNoSmokeStatus = TRUE;
		}else{
			uint16_t TatioValue = 0;
			DiffValue *= 100;
			TatioValue = DiffValue/(_BatteryBaseADValue + 1);
			TatioValue = 100 - TatioValue;
			//判断是否有持续的大烟
			if(TatioValue <= _SensitivitySetValue){
				if(_FlagSmokeStatus != SMOKE_LARGE){
					_FlagSendDataNow = TRUE;
				}
				_FlagSmokeStatus = SMOKE_LARGE;
				_FlagNoSmokeStatus = FALSE;
				_NoSmokeStatusCnt = 0;
			}else{
				_FlagNoSmokeStatus = TRUE;
			}
		}
	}
}

static uint8_t _AdcValueGet(uint16_t Value)
{
	_AdcValue = Value;
	_FlagAdcConversionEnd = TRUE;
	return 0;
}

static void _TimerCallback(void)
{
	_TimingSendDataCount++;
	if(1 == _FlagStartStableTimeCnt){
		_StableTimeCnt++;
		//校准时间  100 * 20Ms = 2s
		if(_StableTimeCnt > 100){
			_FlagStartStableTimeCnt = 2;
		}
	}
	//无烟档位回退计算
	if(TRUE == _FlagNoSmokeStatus){
		if(_FlagSmokeStatus > SMOKE_NONE){
			if(_NoSmokeStatusCnt < _BackTimeSetValue * DEF_ONE_SECOND_CNT){
				_NoSmokeStatusCnt++;
			}else{
				_FlagSmokeStatus--;
				_NoSmokeStatusCnt = 0;
			}
		}else{
			_NoSmokeStatusCnt = 0;
		}
	}else{
		_NoSmokeStatusCnt = 0;
	}
}

static void _TimingSendData(void)
{
	uint16_t AdjustValue = 0;
	uint8_t i = 0;
	uint8_t SendDataBuffer[10] = {0};
	SendDataBuffer[0] = 0x55;
	SendDataBuffer[1] = 0x55;
	SendDataBuffer[2] = 0x07; 
	SendDataBuffer[3] = _FlagSmokeStatus;
	SendDataBuffer[4] = (_BatteryADStorageValue >> 8) & 0xff;
	SendDataBuffer[5] = _BatteryADStorageValue & 0xff;
	SendDataBuffer[6] = (_BatteryBaseADValue >> 8) & 0xff;;
	SendDataBuffer[7] = _BatteryBaseADValue & 0xff;;
	for(i = 0; i < 8; i++){
		AdjustValue += SendDataBuffer[i];
	}
	SendDataBuffer[8] = (AdjustValue >> 8) & 0xff;
	SendDataBuffer[9] = AdjustValue & 0xff;
	for(i = 0; i < 10; i++){
		UART1_SendChar(SendDataBuffer[i]);
	}
}


/*
static uint8_t CheckTemperatureValue(uint16_t Value)
{
    uint8_t i = 0;
    uint8_t Temp = 0;
    uint32_t RValue = 0;
    RValue = DEF_TEMPERATURE_REF_R_VALUE * (uint32_t)Value;
    //RValue = (Value * 10000)/(4826 - Value);
    RValue = RValue/(DEF_VCC_DIGIT_VALUE - Value);
    if(RValue <= DEF_TEMPERATURE_MIN_RESISTANCE){
        Temp = 99;
    }  
    for(i = 0; i < 100; i++){
        if(RValue > TemperatureTable[i]){
            Temp = i;
            break;
        }
    }
    return Temp;
}
*/
//static bool _FlagStartStorageData = FALSE;
static void _UartDataGet(uint8_t c)
{
	_UartDataRxBuffer[_UartDataRxLen] = c;
	_UartDataRxLen++;
	if(_UartDataRxLen >= DEF_UART_RX_MAX_LEN - 1){
		_UartDataRxLen = 0;
	}
}
//串口数据接收处理
static void _UartDataProcess(void)
{
	uint8_t IsDo = 0;
	uint8_t DataPos = 0;
	uint8_t i = 0, j = 0;
	uint8_t Cmd = 0;
	uint16_t AdjustValue1 = 0;
	uint16_t AdjustValue2 = 0;
	if(_UartDataRxLen < 10){
		return;
	}
	//查找是否有完整的帧
	for(i = 0; i < _UartDataRxLen; i++){
		if((0x55 == _UartDataRxBuffer[i]) && (0xaa == _UartDataRxBuffer[i + 1])){
			//判断是否是有效帧
			if((i + 9) >= 10){
				return;
			}
			//计算校验位
			for(j = 0; j < 8; j++){
				AdjustValue1 += _UartDataRxBuffer[i + j];
			}
			AdjustValue2 = (_UartDataRxBuffer[i + 8] << 8) | _UartDataRxBuffer[i + 9];
			//校验错误则不处理
			if(AdjustValue1 != AdjustValue2){
				return;
			}else{
				IsDo = 1;
				//记录帧开始的位置
				DataPos = i;
				//如果有正确的帧  则复位串口接收的长度
				_UartDataRxLen = 0;
				break;
			}
		}
	}
	if(0 == IsDo){
		return;
	}else{
		//获取命令字
		Cmd = _UartDataRxBuffer[DataPos + 2];
		//命令是写数据
		if(Cmd & 0x08){
			uint8_t Temp = 0;
			Temp = _UartDataRxBuffer[DataPos + 3];
			if(0 == Temp){
				_FlagStartStableTimeCnt = 0;
				_PowerSetStatus = FALSE;
			}else{
				//判断原来是否是关闭的状态
				if(FALSE == _PowerSetStatus){
					_FlagStartStableTimeCnt = 1;
				}
				_PowerSetStatus = TRUE;
			}
			Temp = _UartDataRxBuffer[DataPos + 4];
			if(0 != Temp){
				_SensitivitySetValue = Temp;
			}
			Temp = _UartDataRxBuffer[DataPos + 5];
			if(0 != Temp){
				_BackTimeSetValue = Temp;
			}
		}
		_FlagUartDataAck = TRUE;
	}
}

static bool _UartDataAck(void)
{
	if(TRUE == _FlagUartDataAck){
		uint8_t i = 0;
		uint16_t AdjustValue = 0;
		uint8_t AckDataBuf[10] = {0};
		AckDataBuf[0] = 0x55;
		AckDataBuf[1] = 0xaa;
		AckDataBuf[2] = 0x10;
		AckDataBuf[3] = _PowerSetStatus;
		AckDataBuf[4] = _SensitivitySetValue;
		AckDataBuf[5] = _BackTimeSetValue;
		AckDataBuf[6] = 0x00;
		AckDataBuf[7] = VERSION;
		//计算校验位
		for(i = 0; i < 8; i++){
			AdjustValue += AckDataBuf[i];
		}
		AckDataBuf[8] = (AdjustValue >> 8) & 0xff;
		AckDataBuf[9] = AdjustValue & 0xff;
		//发送数据
		for(i = 0; i < 10; i++){
			UART1_SendChar(AckDataBuf[i]);
		}
		_FlagUartDataAck = FALSE;
		return TRUE;
	}
	return FALSE;
}

#if USE_GPIO_CONTROL_STATUS
static void _GPIO_ControlInit(void)
{
	GPIO_Init(GPIOC, GPIO_PIN_4, GPIO_MODE_OUT_PP_LOW_SLOW);
	GPIO_Init(GPIOC, GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_SLOW);
	GPIO_Init(GPIOC, GPIO_PIN_6, GPIO_MODE_OUT_PP_LOW_SLOW);
}

static void _RefreshGPIOStatus(void)
{
	if(TRUE == _PowerSetStatus){
		if(SMOKE_LARGE == _FlagSmokeStatus){
		GPIO_WriteLow(GPIOC, GPIO_PIN_4);
		GPIO_WriteLow(GPIOC, GPIO_PIN_5);
		GPIO_WriteHigh(GPIOC, GPIO_PIN_6);
		}else if(SMOKE_SAMLL == _FlagSmokeStatus){
			GPIO_WriteLow(GPIOC, GPIO_PIN_4);
			GPIO_WriteHigh(GPIOC, GPIO_PIN_5);
			GPIO_WriteLow(GPIOC, GPIO_PIN_6);
		}else{
			GPIO_WriteHigh(GPIOC, GPIO_PIN_4);
			GPIO_WriteLow(GPIOC, GPIO_PIN_5);
			GPIO_WriteLow(GPIOC, GPIO_PIN_6);
		}
	}else{
		GPIO_WriteLow(GPIOC, GPIO_PIN_4);
		GPIO_WriteLow(GPIOC, GPIO_PIN_5);
		GPIO_WriteLow(GPIOC, GPIO_PIN_6);
	}
	
}
#endif 