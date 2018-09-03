


#include "flash.h"




void FlashInit(void)
{
  FLASH_SetProgrammingTime(FLASH_PROGRAMTIME_STANDARD);
  FLASH_Unlock(FLASH_MEMTYPE_DATA);
}


void FlashWriteOneByte(uint32_t Address, uint8_t Value)
{
  FLASH_ProgramByte(Address, Value);
}

uint8_t FlashReadOneByte(uint32_t Address)
{
  return FLASH_ReadByte(Address);
}