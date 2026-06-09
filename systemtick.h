#ifndef SYSTEMTICK_H
#define SYSTEMTICK_H

#include "stdint.h"

extern volatile uint32_t systick;

void SysTick_Init(void);
	
void delayms(uint32_t ms);


#endif