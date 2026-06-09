/**
 * @file    stepper_motion.h
 * @brief   Stepper motor motion control engine
 * @date    06.02.2026
 *
 * Platform-independent motion control with trapezoidal velocity profile.
 * Uses David Austin's real-time acceleration algorithm (integer math in ISR).
 */

#ifndef STEPPER_MOTION_H
#define STEPPER_MOTION_H

#include <stdint.h>

/* ===== Types ============================================================== */

typedef enum
{
   STEPPER_IDLE = 0,
   STEPPER_ACCEL,
   STEPPER_RUN,
   STEPPER_DECEL
} stepper_state_t;

typedef void (*stepper_callback_t)(void);

/* ===== Public Functions =================================================== */

/**
 * @brief   Initialize motion engine and HAL
 */
void stepperMotionInit(void);

/**
 * @brief   Start a move with acceleration specified in steps/s^2
 * @param   steps          Relative steps to move (signed, + = forward)
 * @param   max_speed_pps  Maximum speed in pulses per second
 * @param   accel_pps2     Acceleration in steps/s^2 (0 = no ramp)
 * @return  0 = success, -1 = invalid params, -2 = motor busy
 */
int8_t stepperMotionStart(int32_t steps, uint16_t max_speed_pps, uint16_t accel_pps2);

/**
 * @brief   Start a move with ramp time instead of acceleration
 * @param   steps          Relative steps to move (signed)
 * @param   max_speed_pps  Maximum speed in pulses per second
 * @param   ramp_time_ms   Time to reach max speed (ms), 0 = no ramp
 * @return  0 = success, -1 = invalid params, -2 = motor busy
 */
int8_t stepperMotionStartTimed(int32_t steps, uint16_t max_speed_pps, uint16_t ramp_time_ms);

/**
 * @brief   Smooth stop (decelerate to zero from current speed)
 */
void stepperMotionStop(void);

/**
 * @brief   Emergency stop (immediate, no deceleration)
 * @warning May cause mechanical stress and lost steps
 */
void stepperMotionEmergencyStop(void);

/**
 * @brief   Get current state
 * @return  Current stepper_state_t
 */
stepper_state_t stepperMotionGetState(void);

/**
 * @brief   Check if motor is moving
 * @return  1 = busy, 0 = idle
 */
uint8_t stepperMotionIsBusy(void);

/**
 * @brief   Get current position in steps
 * @return  Position (signed)
 */
int32_t stepperMotionGetPosition(void);

/**
 * @brief   Override current position (for homing or reset)
 * @param   position  New position value
 */
void stepperMotionSetPosition(int32_t position);

/**
 * @brief   Register callback for motion complete event
 * @param   callback  Function pointer (called from ISR context!)
 */
void stepperMotionRegisterCallback(stepper_callback_t callback);

/**
 * @brief   Timer interrupt handler - called from HAL callback
 * @note    Do NOT call this directly, it is called by stepper_hal.c
 */
void stepperTimerCallback(void);

#endif /* STEPPER_MOTION_H */
