#include "systemtick.h"
#include "NuMicro.h"
#include "display7seg.h"

volatile uint32_t systick;

void SysTick_Init(void) 
{
  uint32_t ticks = SystemCoreClock / 1000; // 1 ms için tick sayisi
  SysTick_Config(ticks); 
  NVIC_SetPriority(SysTick_IRQn, 15);
}


void SysTick_Handler(void)
{
	static uint32_t time_divisor;
	++systick;
	++time_divisor;
	
	if(time_divisor == 3)
	{
		time_divisor = 0;
		displayScan();
	}
	
}

void delayms(uint32_t ms)
{
	uint32_t start = systick;
	while(systick - start < ms)
	{
		//todo: loop control
	}
}
