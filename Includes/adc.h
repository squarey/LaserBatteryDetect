
#ifndef __MYADC_H
#define __MYADC_H

#include "stm8s.h"


void ADC_InitChannel(uint8_t Channels);
void StartAdcConversion(void);
void SetAdcConversionEnd_cb(void *pFunction);

#endif








