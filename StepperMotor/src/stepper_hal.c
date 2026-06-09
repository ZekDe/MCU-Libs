/**
 * @file    stepper_hal.c
 * @brief   Nuvoton HAL implementation for stepper driver
 * @date    06.02.2026
 * @author Emrah Duatepe
 * 
 * Port this file for different MCU platforms.
 */

#include "stepper_hal.h"
#include "stepper_config.h"
#include "stepper_motion.h"
#include "main.h"
#include "systemtick.h"

/* ===== Extern ============================================================= */



/* ===== Private Helpers ==================================================== */

/**
 * @brief   Inline busy-wait delay for step pulse width
 * @note    Calculated from STEPPER_SYSTEM_CLOCK_HZ and STEPPER_PULSE_WIDTH_US
 */
static inline void pulseDelay(void)
{
   /* ~4 cycles per loop iteration at -O0, fewer with optimization.
    * Conservative estimate ensures minimum pulse width is met. */
   volatile uint32_t count = (STEPPER_SYSTEM_CLOCK_HZ / 1000000UL) * STEPPER_PULSE_WIDTH_US / 4;
   while (count--)
   {
      /* spin */
   }
}

static void stepperSetEnablePin(uint8_t _01)
{
#if STEPPER_EN_PIN_ENABLE
	#if STEPPER_ENABLE_ACTIVE_LOW
	   PA15 = !_01;
	#else
	   //PF15 = _01;
	#endif
#endif
}

static void stepperSetDirPin(uint8_t _01)
{
	PF15 = _01;
}

static void stepperSetStepPin(uint8_t _01)
{
	PB4 = _01;
}

/* ===== Public Functions =================================================== */

void stepperHalInit(void)
{
	stepperSetEnablePin(1);
    stepperSetDirPin(0);
    stepperSetStepPin(0);
}

void stepperHalPulse(void)
{
   stepperSetStepPin(1);
   pulseDelay();
   stepperSetStepPin(0);
}

void stepperHalSetDirection(uint8_t dir)
{
	stepperSetDirPin(dir ? 1 : 0);
}

void stepperHalEnable(void)
{
	stepperSetEnablePin(1);
}


void stepperHalDisable(void)
{
	stepperSetEnablePin(0);
}


void stepperHalSetTimerPeriod(uint32_t ticks)
{
   TIMER_SET_CMP_VALUE(TIMER1, ticks);
   //TIMER_ResetCounter(TIMER1);
}

void stepperHalStartTimer(void)
{
   //TIMER_ResetCounter(TIMER1);
   TIMER_ClearIntFlag(TIMER1);
   TIMER_EnableInt(TIMER1);
   TIMER_Start(TIMER1);
}

void stepperHalStopTimer(void)
{
   TIMER_Stop(TIMER1);
   TIMER_DisableInt(TIMER1);
}
 
// Only for test , no dependency to library
void stepperHalMotorTest(void)
{
   /* Enable motor */
   stepperSetEnablePin(1);
	
   /* Direction forward */
   stepperSetDirPin(0);
   delayms(2);
   /* 200 steps, ~100 pps (10ms period) */
   for (uint16_t i = 0; i < 200; i++)
   {
      stepperSetStepPin(1);
      delayms(1);
      stepperSetStepPin(0);
      delayms(9);
   }

   stepperSetDirPin(1);
   delayms(2);
   /* 200 steps, ~100 pps (10ms period) */
   for (uint16_t i = 0; i < 200; i++)
   {
      stepperSetStepPin(1);
      delayms(1);
      stepperSetStepPin(0);
      delayms(9);
   }
   
   /* Disable motor */
   stepperSetEnablePin(0);
   delayms(2);
}

/* ===== Timer ISR Callback ================================================= */

/**
 * @brief   timer period elapsed callback
 * @note   
 */
void timerElapsedCallback(void)
{
	stepperTimerCallback();
}
