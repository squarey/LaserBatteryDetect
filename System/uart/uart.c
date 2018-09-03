

#include "uart.h" 
#include "adc.h"


//定时时间到的回调函数
typedef void (*UartReceivedCallback)(uint8_t c);

static UartReceivedCallback _ReceivedCallback;
static void send_u16(uint32_t in)
{
  uint32_t temp_i = 0;
  temp_i = in;
  if(0 == temp_i)
  {
    UART1_SendChar('0');
  }
  else
  {
    char str1[10];
    int j = 0,k = 0;
    while(temp_i > 0)
    {
      str1[j++] = temp_i%10+'0';
      temp_i /= 10;
    }
    for(k = 0 ;k < j ;k++)
    {
      UART1_SendChar(str1[j-1-k]);
    }
  }
}
/*
static void send_float(float f)
{
  char str1[10] = "";
  char str[10] = "";
  float temp_f = 0;
  int j = 0,k,i;
  temp_f = f;
  i = (int)temp_f;  //浮点数的整数部分
  do
  {
    str1[j++] = i%10 + '0';
    i /= 10;
  }while(i > 0);
  for(k = 0;k < j; k++)
  {
    str[k] = str1[j-1-k]; //
  }
  str[j++] = '.';
  temp_f -= (int)temp_f;//浮点型的小数部分
  for(i = 0 ;i < 5 ;i++)
  {
    temp_f *= 10;
    str[j++] = (int)temp_f+'0';
    temp_f -= (int)temp_f;
  }
  while(str[j] == '0');
  {
    str[++j] = '\0';
  }
  UART1_SendString(str);
}
void UART1_SendFloat(float f)
{
  //UART1_SendString(F2S(f));
  send_float(f);
}
*/
void UART1_SendInt(uint32_t i)
{
  send_u16(i);
}

void UART1_Congfiguration(void)
{
  UART1_DeInit();
  //初始化串口1  波特率:4800  8位数据位   1位停止位   无校验位   允许接收
  UART1_Init((uint32_t)9600, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO, UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);
  //使能串口接收
  UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);
  //启动串口1
  UART1_Cmd(ENABLE);
  //开启中断
  enableInterrupts();
}
void UART1_SendChar(uint8_t c)
{
  while(UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET);
   UART1_SendData8(c);
}
void UART1_SendString(char *str)
{
  uint8_t i = 0;
  while('\0' != (*(str + i)))
  {
     UART1_SendChar(*(str + i));
     i++;
  }
}

void SetUartReceivedCallback(void *pFuntion)
{
  _ReceivedCallback = (UartReceivedCallback)pFuntion;
}
#include "iostm8s103f3.h"
INTERRUPT_HANDLER(UART1_RX_IRQHandler, 18)
{
  uint8_t Temp = 0;
  //判断是否是溢出错误
  if(UART1_SR_OR_LHE){
    UART1_SR_OR_LHE = 0;
    UART1_SR_RXNE = 0;
    Temp = UART1_SR;
    Temp = UART1_DR;
  }else if(UART1_SR_RXNE){
    UART1_SR_RXNE = 0;
    UART1_SR_OR_LHE = 0;
    Temp = UART1_DR;
  }
  if(_ReceivedCallback){
    _ReceivedCallback(Temp);
  }
}
