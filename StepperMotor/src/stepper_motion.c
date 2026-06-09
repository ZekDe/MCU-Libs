/**
 * @file    stepper_motion.c
 * @brief   Stepper motor motion control engine
 * @date    06.02.2026
 *
 * Implements trapezoidal/triangular velocity profiles using David Austin's
 * real-time acceleration algorithm. All ISR math is integer-only.
 *
 * Reference: "Generate stepper-motor speed profiles in real time"
 *            by David Austin, Embedded Systems Programming, 2005
 *
 * Algorithm summary:
 *   c0 = 0.676 * f * sqrt(2 / accel)        [initial timer period]
 *   cn = c_{n-1} - 2*c_{n-1} / (4*n + 1)    [acceleration, integer]
 *   cn = c_{n-1} + 2*c_{n-1} / (4*n + 1)    [deceleration, integer]
 */

#include "stepper_motion.h"
#include "stepper_hal.h"
#include "stepper_config.h"
#include <math.h>   /* sqrt() - used once at motion start, not in ISR */
#include <stdlib.h> /* abs() */

/* ===== Private Types ====================================================== */

typedef struct
{
   volatile stepper_state_t state;

   /* Position tracking */
   volatile int32_t current_position;
   int32_t target_position;
   uint8_t direction;          /* 0 = forward, 1 = reverse */

   /* Step counters */
   volatile uint32_t step_count;     /* steps completed in this move */
   uint32_t total_steps;             /* total steps for this move */

   /* Ramp profile */
   uint32_t accel_steps;       /* steps needed for full acceleration */
   uint32_t decel_start;       /* step number where decel begins */
   uint32_t ramp_n;            /* current step within accel/decel phase */

   /* Timer periods (in timer ticks) */
   uint32_t current_delay;     /* current timer period */
   uint32_t min_delay;         /* minimum period = maximum speed */
   uint32_t max_delay;         /* maximum period = minimum speed */
   uint32_t c0;                /* initial period (first step of accel) */

   /* Acceleration (stored for stop calculation) */
   uint16_t accel_pps2;

} stepper_motion_t;

/* ===== Private Variables ================================================== */

stepper_motion_t mot;
static stepper_callback_t motion_complete_cb = NULL;

/* ===== Private Function Prototypes ======================================== */

static void calculateProfile(uint16_t max_speed_pps, uint16_t accel_pps2);
static void completeMotion(void);

/* ===== Public Functions =================================================== */

void stepperMotionInit(void)
{
   mot.state = STEPPER_IDLE;
   mot.current_position = 0;
   mot.target_position = 0;
   mot.step_count = 0;
   mot.total_steps = 0;
   mot.current_delay = 0;
   mot.accel_pps2 = 0;

   mot.min_delay = STEPPER_TIMER_FREQ_HZ / STEPPER_MAX_SPEED_PPS;
   mot.max_delay = STEPPER_TIMER_FREQ_HZ / STEPPER_MIN_SPEED_PPS;

   stepperHalInit();
}

int8_t stepperMotionStart(int32_t steps, uint16_t max_speed_pps, uint16_t accel_pps2)
{
   /* Validate parameters */
   if (steps == 0)
   {
      return -1;
   }
   if (max_speed_pps < STEPPER_MIN_SPEED_PPS || max_speed_pps > STEPPER_MAX_SPEED_PPS)
   {
      return -1;
   }
   if (mot.state != STEPPER_IDLE)
   {
      return -2;
   }

   /* Set direction */
   mot.direction = (steps > 0) ? 0 : 1;
   stepperHalSetDirection(mot.direction);

   /* Set target */
   mot.total_steps = (uint32_t)abs(steps);
   mot.target_position = mot.current_position + steps;
   mot.step_count = 0;

   /* Store accel for potential stop calculation */
   mot.accel_pps2 = accel_pps2;

   /* Calculate velocity profile */
   calculateProfile(max_speed_pps, accel_pps2);

   /* Enable motor */
   stepperHalEnable();

   /* Set initial timer period and start */
   if (accel_pps2 == 0)
   {
      /* No ramp: constant speed */
      mot.current_delay = STEPPER_TIMER_FREQ_HZ / max_speed_pps;
      mot.state = STEPPER_RUN;
   }
   else
   {
      /* Start from c0 (first step delay) */
      mot.current_delay = mot.c0;

      /* Clamp to valid range */
      if (mot.current_delay > mot.max_delay)
      {
         mot.current_delay = mot.max_delay;
      }
      if (mot.current_delay < mot.min_delay)
      {
         mot.current_delay = mot.min_delay;
      }

      mot.state = STEPPER_ACCEL;
   }

   mot.ramp_n = 0;
   stepperHalSetTimerPeriod(mot.current_delay);
   stepperHalStartTimer();

   return 0;
}

int8_t stepperMotionStartTimed(int32_t steps, uint16_t max_speed_pps, uint16_t ramp_time_ms)
{
   uint16_t accel_pps2 = 0;

   if (ramp_time_ms > 0)
   {
      /* accel = max_speed / ramp_time_seconds */
      accel_pps2 = (uint16_t)((uint32_t)max_speed_pps * 1000UL / ramp_time_ms);
      if (accel_pps2 == 0)
      {
         accel_pps2 = 1;
      }
   }

   return stepperMotionStart(steps, max_speed_pps, accel_pps2);
}

void stepperMotionStop(void)
{
   if (mot.state == STEPPER_IDLE)
   {
      return;
   }

   if (mot.state == STEPPER_DECEL)
   {
      /* Already decelerating, do nothing */
      return;
   }

   if (mot.accel_pps2 == 0)
   {
      /* No ramp configured, stop immediately */
      completeMotion();
      return;
   }

   /* Calculate decel steps from current speed:
    * current_speed = timer_freq / current_delay
    * decel_steps = speed^2 / (2 * accel)
    * = timer_freq^2 / (current_delay^2 * 2 * accel) */
   uint32_t current_speed = STEPPER_TIMER_FREQ_HZ / mot.current_delay;
   uint32_t decel_steps = (current_speed * current_speed) / (2UL * mot.accel_pps2);
   if (decel_steps == 0)
   {
      decel_steps = 1;
   }

   /* Adjust total_steps so motion ends after decel */
   mot.total_steps = mot.step_count + decel_steps;
   mot.decel_start = mot.step_count;
   mot.state = STEPPER_DECEL;
   mot.ramp_n = 0;
}

void stepperMotionEmergencyStop(void)
{
   stepperHalStopTimer();
   mot.state = STEPPER_IDLE;
   mot.current_delay = 0;
   /* Position may be inaccurate after emergency stop */
}

stepper_state_t stepperMotionGetState(void)
{
   return mot.state;
}

uint8_t stepperMotionIsBusy(void)
{
   return (mot.state != STEPPER_IDLE) ? 1 : 0;
}

int32_t stepperMotionGetPosition(void)
{
   return mot.current_position;
}

void stepperMotionSetPosition(int32_t position)
{
   mot.current_position = position;
}

void stepperMotionRegisterCallback(stepper_callback_t callback)
{
   motion_complete_cb = callback;
}

/* ===== Timer ISR Handler ================================================== */

void stepperTimerCallback(void)
{
   /* Generate one step pulse */
   stepperHalPulse();

   /* Update counters */
   mot.step_count++;

   if (mot.direction == 0)
   {
      mot.current_position++;
   }
   else
   {
      mot.current_position--;
   }

   /* Check if move is complete */
   if (mot.step_count >= mot.total_steps)
   {
      completeMotion();
      return;
   }

   /* State machine - update speed for next step */
   switch (mot.state)
   {
      case STEPPER_ACCEL:
      {
         mot.ramp_n++;

         /* Austin's formula: cn = c_{n-1} - 2*c_{n-1} / (4*n + 1) */
         uint32_t denominator = 4 * mot.ramp_n + 1;
         mot.current_delay -= (2 * mot.current_delay) / denominator;

         /* Clamp to max speed */
         if (mot.current_delay <= mot.min_delay)
         {
            mot.current_delay = mot.min_delay;
            mot.state = STEPPER_RUN;
         }

         /* Check if we should start decelerating (triangular profile) */
         if (mot.step_count >= mot.decel_start)
         {
            mot.state = STEPPER_DECEL;
            mot.ramp_n = 0;
         }

         stepperHalSetTimerPeriod(mot.current_delay);
         break;
      }

      case STEPPER_RUN:
      {
         /* Constant speed - no timer update needed */
         if (mot.step_count >= mot.decel_start)
         {
            mot.state = STEPPER_DECEL;
            mot.ramp_n = 0;
         }
         break;
      }

      case STEPPER_DECEL:
      {
         mot.ramp_n++;

         /* Reverse Austin's: cn = c_{n-1} + 2*c_{n-1} / (4*n + 1) */
         uint32_t denominator = 4 * mot.ramp_n + 1;
         mot.current_delay += (2 * mot.current_delay) / denominator;

         /* Clamp to min speed */
         if (mot.current_delay >= mot.max_delay)
         {
            mot.current_delay = mot.max_delay;
         }

         stepperHalSetTimerPeriod(mot.current_delay);
         break;
      }

      case STEPPER_IDLE:
      default:
      {
         /* Should not reach here */
         stepperHalStopTimer();
         break;
      }
   }
}

/* ===== Private Functions ================================================== */

/**
 * @brief   Calculate trapezoidal/triangular motion profile
 * @param   max_speed_pps  Target maximum speed
 * @param   accel_pps2     Acceleration in steps/s^2
 */
static void calculateProfile(uint16_t max_speed_pps, uint16_t accel_pps2)
{
   if (accel_pps2 == 0)
   {
      /* No ramp */
      mot.accel_steps = 0;
      mot.decel_start = mot.total_steps;
      mot.c0 = STEPPER_TIMER_FREQ_HZ / max_speed_pps;
      return;
   }

   /* Calculate initial delay using Austin's formula:
    * c0 = 0.676 * f * sqrt(2 / accel)
    * 0.676 is a correction factor for the first step approximation */
   float f = (float)STEPPER_TIMER_FREQ_HZ;
   float a = (float)accel_pps2;
   mot.c0 = (uint32_t)(0.676f * f * sqrtf(2.0f / a));

   /* Minimum delay = maximum speed */
   mot.min_delay = STEPPER_TIMER_FREQ_HZ / max_speed_pps;

   /* Accel steps: s = v^2 / (2*a) */
   uint32_t speed_sq = (uint32_t)max_speed_pps * (uint32_t)max_speed_pps;
   mot.accel_steps = speed_sq / (2UL * accel_pps2);

   /* Check for triangular profile (can't reach max speed) */
   uint32_t total_ramp_steps = mot.accel_steps * 2;

   if (total_ramp_steps >= mot.total_steps)
   {
      /* Triangular profile */
      mot.accel_steps = mot.total_steps / 2;
      mot.decel_start = mot.accel_steps;
   }
   else
   {
      /* Trapezoidal profile */
      mot.decel_start = mot.total_steps - mot.accel_steps;
   }
}

/**
 * @brief   Complete motion - stop timer, reset state, fire callback
 */
static void completeMotion(void)
{
   stepperHalStopTimer();
   mot.state = STEPPER_IDLE;
   mot.current_delay = 0;

   if (motion_complete_cb != NULL)
   {
      motion_complete_cb();
   }
}
