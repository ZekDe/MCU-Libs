/*
 * steady_clock.c
 *
 *  Created on: Feb 23, 2024
 *      Author: Duatepe
 */

#include "steady_clock.h"
#include "NuMicro.h"
#include "stdio.h"

void SteadyClock_Init(void)
{
	// NuTool Code Generator performed it.
}

uint32_t getus(void)
{
//	uint64_t x = TIMER_GetCounter(TIMER0) * 1000000ULL;
//	uint64_t y = x / (uint64_t) TIMER_GetModuleClock(TIMER0);
//	return (uint32_t)y;
	// zaten sayici 1us ile artiyor 
	//return (TIMER_GetCounter(TIMER0) * 1000000ULL) / (uint64_t)TIMER_GetModuleClock(TIMER0);
	return TIMER_GetCounter(TIMER0);
   //return (uint32_t)(((uint64_t)DWT->CYCCNT * 1000000ULL) / SystemCoreClock);
}
//TIMER_Delay(TIMER0, 1000000);
