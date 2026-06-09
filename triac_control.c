/**
 * @file    triac_control.c
 * @brief   Non-blocking triac phase control library with virtual zero-crossing
 * @date    04.03.2026
 */

/* Includes */
#include "triac_control.h"

/* Private defines */

/* Private function prototypes */
static uint32_t calcElapsed(uint32_t current_us, uint32_t reference_us);
static uint16_t calcPhaseDelay(triac_t *handle);
static void updateRamp(triac_t *handle, uint32_t current_ms);

/* Public functions */

void initTriac(triac_t *handle, pin_write_fn_t write_pin,
               uint16_t base_offset_us, uint32_t half_fire_offset_us, uint16_t pulse_width_us,
               uint8_t min_power, uint16_t max_phase_delay_us, uint16_t rejected_period_us)
{
   handle->write_pin          = write_pin;
   handle->base_offset_us     = base_offset_us;
   handle->pulse_width_us     = pulse_width_us;
   handle->min_power          = min_power;
   handle->max_phase_delay_us = max_phase_delay_us;
   handle->half_fire_offset   = half_fire_offset_us;
   handle->rejected_period    = rejected_period_us;

   handle->power              = 0;
   handle->state              = TRIAC_STATE_IDLE;
   handle->last_zc_us         = 0;
   handle->prev_zc_us         = 0;
   handle->measured_period_us = 0;
   handle->fire_time_us       = 0;
   handle->pulse_start_us     = 0;
   handle->zc_flag            = 0;

   handle->ramp_active        = 0;
   handle->ramp_start_power   = 0;
   handle->ramp_target_power  = 0;
   handle->ramp_duration_ms   = 0;
   handle->ramp_start_ms      = 0;
}


void handleZeroCrossing(triac_t *handle, uint32_t current_us)
{
   uint32_t period;

   period = calcElapsed(current_us, handle->last_zc_us);

   /* Reject spurious interrupts (EMI noise) */
   if (handle->last_zc_us != 0 && period < handle->rejected_period)
   {
      return;
   }

   /* Store previous for period measurement */
   handle->prev_zc_us = handle->last_zc_us;
   handle->last_zc_us = current_us;

   /* Calculate period from consecutive interrupts */
   if (handle->prev_zc_us != 0)
   {
      handle->measured_period_us = calcElapsed(current_us, handle->prev_zc_us);
   }

   handle->zc_flag = 1;
}

void updateTriac(triac_t *handle, uint32_t current_us, uint32_t current_ms)
{
   uint32_t elapsed;

   /* Update ramp before state machine */
   if (handle->ramp_active)
   {
      updateRamp(handle, current_ms);
   }

   switch (handle->state)
   {
      case TRIAC_STATE_IDLE:
         if (handle->zc_flag)
         {
            handle->zc_flag = 0;

            /* Power 0 or below minimum threshold: do nothing */
            if (handle->power == 0 || handle->power < handle->min_power)
            {
               break;
            }

            /* Calculate total firing delay: base_offset + phase_delay */
            handle->fire_time_us = (uint32_t)handle->base_offset_us + calcPhaseDelay(handle);
            handle->state = TRIAC_STATE_WAIT_FIRST;
         }
         break;

      case TRIAC_STATE_WAIT_FIRST:
         elapsed = calcElapsed(current_us, handle->last_zc_us);
         if (elapsed >= handle->fire_time_us)
         {
            handle->write_pin(1);
            handle->pulse_start_us = current_us;
            handle->state = TRIAC_STATE_PULSE_FIRST;
         }
         break;

      case TRIAC_STATE_PULSE_FIRST:
         elapsed = calcElapsed(current_us, handle->pulse_start_us);
         if (elapsed >= handle->pulse_width_us)
         {
            handle->write_pin(0);
#if TRIAC_HALF_PERIOD_MODE
            /* Half-period: her IRQ tek yari periyot, 2. yari yok */
            handle->state = TRIAC_STATE_IDLE;
#else
            /* Full-period: 2. yariyi sanal olarak uret */
            handle->state = TRIAC_STATE_WAIT_SECOND;
#endif
         }
         break;

      case TRIAC_STATE_WAIT_SECOND:
         elapsed = calcElapsed(current_us, handle->last_zc_us);
         if (elapsed >= (handle->measured_period_us / 2 + handle->half_fire_offset + calcPhaseDelay(handle)))
         {
            handle->write_pin(1);
            handle->pulse_start_us = current_us;
            handle->state = TRIAC_STATE_PULSE_SECOND;
         }
         break;

      case TRIAC_STATE_PULSE_SECOND:
         elapsed = calcElapsed(current_us, handle->pulse_start_us);
         if (elapsed >= handle->pulse_width_us)
         {
            handle->write_pin(0);
            handle->state = TRIAC_STATE_IDLE;
         }
         break;

      default:
         handle->write_pin(0);
         handle->state = TRIAC_STATE_IDLE;
         break;
   }
}

void setTriacPower(triac_t *handle, uint8_t power)
{
   if (power > 100)
   {
      power = 100;
   }
   handle->power = power;
}

void setTriacPowerByRamp(triac_t *handle, uint8_t power, uint16_t ramp_ms)
{
   if (power > 100)
   {
      power = 100;
   }
   
    /* Ramp already running toward same target: do nothing */
   if (handle->ramp_active && handle->ramp_target_power == power)
   {
      return;
   }

   /* No ramp requested: apply immediately */
   if (ramp_ms == 0)
   {
      handle->ramp_active = 0;
      setTriacPower(handle, power);
      return;
   }

   /* Already at target: nothing to do */
   if (handle->power == power)
   {
      handle->ramp_active = 0;
      return;
   }

   /* Start new ramp from current power */
   handle->ramp_start_power  = handle->power;
   handle->ramp_target_power = power;
   handle->ramp_duration_ms  = ramp_ms;
   handle->ramp_start_ms     = 0;   /* Will be captured on first updateRamp call */
   handle->ramp_active       = 1;
}

uint32_t getTriacPeriod(triac_t *handle)
{
   return handle->measured_period_us;
}

/* Private functions */

/**
 * @brief   Calculate elapsed time with 24-bit timer rollover handling
 * @param   current_us    Current timestamp
 * @param   reference_us  Reference timestamp
 * @return  Elapsed time in microseconds
 */
static uint32_t calcElapsed(uint32_t current_us, uint32_t reference_us)
{
   return (current_us - reference_us) & TIMER_MASK_24BIT;
}

/**
 * @brief   Calculate phase delay based on power setting
 * @param   handle  Pointer to triac handle
 * @return  Phase delay in microseconds
 *
 * @note    power=100 -> delay=0 (full power, fires at zero-cross)
 *          power=min -> delay=max_phase_delay (minimum power)
 */
static uint16_t calcPhaseDelay(triac_t *handle)
{
   /* Full power: no additional delay */
   if (handle->power >= 100)
   {
      return 0;
   }

   return (uint16_t)((uint32_t)handle->max_phase_delay_us * (100 - handle->power) / 100);
}

/**
 * @brief   Update ramp interpolation and apply power
 * @param   handle      Pointer to triac handle
 * @param   current_ms  Current millisecond timestamp from systick
 *
 * @note    Uses signed arithmetic internally to handle both
 *          increasing and decreasing ramps correctly.
 */
static void updateRamp(triac_t *handle, uint32_t current_ms)
{
   uint32_t elapsed_ms;
   int32_t  delta;
   int32_t  new_power;

   /* Capture start timestamp on first call */
   if (handle->ramp_start_ms == 0)
   {
      handle->ramp_start_ms = current_ms;
      return;
   }

   elapsed_ms = current_ms - handle->ramp_start_ms;

   /* Ramp complete */
   if (elapsed_ms >= handle->ramp_duration_ms)
   {
      setTriacPower(handle, handle->ramp_target_power);
      handle->ramp_active = 0;
      return;
   }

   /* Linear interpolation: start + (target - start) * elapsed / duration */
   delta = (int32_t)handle->ramp_target_power - (int32_t)handle->ramp_start_power;
   new_power = (int32_t)handle->ramp_start_power + (delta * (int32_t)elapsed_ms / (int32_t)handle->ramp_duration_ms);

   setTriacPower(handle, (uint8_t)new_power);
}
