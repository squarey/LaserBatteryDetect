


#include "stm8s.h"
#include "delay.h"
#include "sys.h"
#include "control.h"
#include "watchdog.h"

  


int main( void )
{
   //��ʼ��ϵͳʱ��
  SystemClockInit();
  //��ʼ����ʱʱ��
  delay_init(16);
  WatchDogInit();
  ControlInit();
  while(1){
    ControlPrcoess();
  }
    
}





