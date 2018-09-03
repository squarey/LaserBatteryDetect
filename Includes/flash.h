

#ifndef __FLASH_H
#define __FLASH_H

#include "stm8s.h"


void FlashInit(void);
void FlashWriteOneByte(uint32_t Address, uint8_t Value);
uint8_t FlashReadOneByte(uint32_t Address);



#endif

