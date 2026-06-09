/**
 * @file    triac_control.h
 * @brief   Non-blocking triac phase control library with virtual zero-crossing
 * @date    04.03.2026
 */
 
// 	
// Example 
//#if TRIAC_HALF_PERIOD_MODE
//	/* Half-period: 2 IRQ arasi ~10ms -> rejected_period < 10ms (8000),
//	   half_fire_offset kullanilmaz (0) */
//	initTriac(&dev_data.fan1, fan1Pin, 0, 0, 300, 20, 8000, 8000);
//	initTriac(&dev_data.fan2, fan2Pin, 0, 0, 300, 20, 8000, 8000);
//	initTriac(&dev_data.fan3, fan3Pin, 0, 0, 300, 20, 8000, 8000);
//	initTriac(&dev_data.exh, exhPin, 0, 0, 300, 20, 8000, 8000);
//	initTriac(&dev_data.aug, augPin, 0, 0, 300, 20, 8000, 8000);
//#else
//	/* Full-period (mevcut): periyotta bir IRQ ~20ms, rejected_period 18000 */
//	initTriac(&dev_data.fan1, fan1Pin, 0, 0, 500, 20, 9000, 18000);
//	initTriac(&dev_data.fan2, fan2Pin, 0, 0, 500, 20, 9000, 18000);
//	initTriac(&dev_data.fan3, fan3Pin, 0, 0, 500, 20, 9000, 18000);
//	initTriac(&dev_data.exh, exhPin, 0, 0, 500, 20, 9000, 18000);
//	initTriac(&dev_data.aug, augPin, 0, 0, 500, 20, 9000, 18000);
//#endif


#ifndef TRIAC_CONTROL_H
#define TRIAC_CONTROL_H

/* Includes */
#include <stdint.h>

/* Public defines */
#define TIMER_MASK_24BIT          0xFFFFFF

/* Triac calisma modu:
 *   0 = full-period (sanal 2. yari periyot, GPIO_INT_RISING, periyotta bir IRQ ~20ms)
 *   1 = half-period (her zero-cross HW interrupt, GPIO_INT_BOTH_EDGE, her yari ~10ms)
 */
#ifndef TRIAC_HALF_PERIOD_MODE
#define TRIAC_HALF_PERIOD_MODE    1
#endif

/* Public typedefs */

/**
 * @brief   Function pointer type for pin write operations
 * @param   state  1 = high, 0 = low
 */
typedef void (*pin_write_fn_t)(uint8_t state);

/**
 * @brief   Triac state machine states
 */
typedef enum
{
   TRIAC_STATE_IDLE = 0,
   TRIAC_STATE_WAIT_FIRST,
   TRIAC_STATE_PULSE_FIRST,
   TRIAC_STATE_WAIT_SECOND,
   TRIAC_STATE_PULSE_SECOND
} triac_state_t;

/**
 * @brief   Triac control handle
 */
typedef struct
{
   /* Configuration */
   uint32_t half_fire_offset;       /* half period fire offset */
   pin_write_fn_t write_pin;        /* Platform-independent pin write function  */
   uint16_t base_offset_us;         /* Delay from interrupt to true zero-cross  */
   uint16_t max_phase_delay_us;     /* Max additional delay at min power        */
   uint16_t pulse_width_us;         /* Gate pulse duration                      */
   uint8_t  min_power;              /* Minimum meaningful power (below = off)   */
   uint16_t rejected_period;        /* Interrupts occurring below this duration are rejected */

   /* Runtime - user settable */
   uint8_t  power;                  /* Desired power level 0-100                */

   /* Runtime - internal */
   triac_state_t state;             /* Current state machine state              */
   uint32_t last_zc_us;             /* Timestamp of last zero-crossing interrupt */
   uint32_t prev_zc_us;             /* Previous ZC timestamp for period calc     */
   uint32_t measured_period_us;     /* Measured full cycle period                */
   uint32_t fire_time_us;           /* Calculated firing delay from last_zc_us   */
   uint32_t pulse_start_us;         /* Timestamp when pulse started              */
   uint8_t  zc_flag;                /* Zero-crossing detected flag               */

   /* Ramp control */
   uint8_t  ramp_active;            /* Ramp in progress flag                     */
   uint8_t  ramp_start_power;       /* Power level when ramp started             */
   uint8_t  ramp_target_power;      /* Target power level                        */
   uint16_t ramp_duration_ms;       /* Total ramp duration in milliseconds       */
   uint32_t ramp_start_ms;          /* Systick timestamp when ramp started       */
} triac_t;

/* Public function prototypes */

/**
 * @brief   Initialize triac control handle
 * @param   handle             Pointer to triac handle
 * @param   write_pin          Function pointer to set gate pin
 * @param   base_offset_us     Delay from interrupt to true zero-cross (us)
 * @param   half_fire_offset_us Half period fire offset (us)
 * @param   pulse_width_us     Gate pulse duration (us)
 * @param   min_power          Minimum power threshold (0-100)
 * @param   max_phase_delay_us Maximum phase delay at minimum power (us)
 * @param   rejected_period    Minimum valid period between zero-crossings (us)
 */
void initTriac(triac_t *handle, pin_write_fn_t write_pin,
               uint16_t base_offset_us, uint32_t half_fire_offset_us, uint16_t pulse_width_us,
               uint8_t min_power, uint16_t max_phase_delay_us, uint16_t rejected_period);

/**
 * @brief   Call from zero-crossing interrupt handler
 * @param   handle      Pointer to triac handle
 * @param   current_us  Current microsecond timestamp from getus()
 */
void handleZeroCrossing(triac_t *handle, uint32_t current_us);

/**
 * @brief   Call from main loop as fast as possible
 * @param   handle      Pointer to triac handle
 * @param   current_us  Current microsecond timestamp from getus()
 * @param   current_ms  Current millisecond timestamp from systick
 */
void updateTriac(triac_t *handle, uint32_t current_us, uint32_t current_ms);

/**
 * @brief   Set triac output power
 * @param   handle  Pointer to triac handle
 * @param   power   Power level 0-100 (0 = off, 100 = full)
 */
void setTriacPower(triac_t *handle, uint8_t power);

/**
 * @brief   Set triac output power with linear ramp
 * @param   handle   Pointer to triac handle
 * @param   power    Target power level 0-100
 * @param   ramp_ms  Ramp duration in milliseconds (0 = immediate, no ramp)
 *
 * @note    If called while a ramp is active, the current interpolated power
 *          becomes the new start point and a fresh ramp begins toward the
 *          new target.
 */
void setTriacPowerByRamp(triac_t *handle, uint8_t power, uint16_t ramp_ms);

/**
 * @brief   Get measured full cycle period
 * @param   handle  Pointer to triac handle
 * @return  Measured period in microseconds (0 if not yet measured)
 */
uint32_t getTriacPeriod(triac_t *handle);

#endif /* TRIAC_CONTROL_H */
