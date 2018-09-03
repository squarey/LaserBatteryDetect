


#include "stm8s.h"
#include "delay.h"
#include "sys.h"
#include "control.h"
#include "watchdog.h"

  


int main( void )
{
   //初始化系统时钟
  SystemClockInit();
  //初始化延时时钟
  delay_init(16);
  WatchDogInit();
  ControlInit();
  while(1){
    ControlPrcoess();
  }
    
}





