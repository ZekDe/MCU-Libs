/**
 * @file    stepper_api.h
 * @brief   Stepper motor public API
 * @date    06.02.2026
 *
 * High-level user interface for stepper motor control.
 * This is the only header the application code needs to include.
 */

#ifndef STEPPER_API_H
#define STEPPER_API_H

#include <stdint.h>

/* ===== Callback Type ====================================================== */

typedef void (*stepper_complete_cb_t)(void);

/* ===== Initialization ===================================================== */

/**
 * @brief   Initialize stepper motor system
 * @note    Call once in main() before using any other stepper functions
 */
void stepperInit(void);

/* ===== Motion Control ===================================================== */

/**
 * @brief   Relative move with acceleration (steps/s^2)
 * @param   steps          Steps to move (+ = forward, - = reverse)
 * @param   max_speed_pps  Maximum speed (pulses per second)
 * @param   accel_pps2     Acceleration (steps/s^2), 0 = no ramp
 * @return  0 = success, -1 = invalid params, -2 = motor busy
 *
 * Example:
 *   stepperMove(200, 150, 500);   // 200 steps forward, 150 pps max, 500 steps/s^2
 *   stepperMove(-100, 100, 300);  // 100 steps reverse
 */
int8_t stepperMove(int32_t steps, uint16_t max_speed_pps, uint16_t accel_pps2);

/**
 * @brief   Relative move with ramp time (milliseconds)
 * @param   steps          Steps to move (signed)
 * @param   max_speed_pps  Maximum speed (pulses per second)
 * @param   ramp_time_ms   Time to reach max speed (ms), 0 = no ramp
 * @return  0 = success, -1 = invalid params, -2 = motor busy
 *
 * Example:
 *   stepperMoveTimed(200, 150, 100);  // 200 steps, 150 pps, 100ms ramp
 */
int8_t stepperMoveTimed(int32_t steps, uint16_t max_speed_pps, uint16_t ramp_time_ms);

/**
 * @brief   Smooth stop with deceleration ramp
 */
void stepperStop(void);

/**
 * @brief   Emergency stop (immediate, no ramp)
 * @warning May lose step position accuracy
 */
void stepperEmergencyStop(void);

/* ===== Status ============================================================= */

/**
 * @brief   Check if motor is currently moving
 * @return  1 = moving, 0 = idle
 */
uint8_t stepperIsBusy(void);

/**
 * @brief   Get current position
 * @return  Position in steps (signed)
 */
int32_t stepperGetPosition(void);

/**
 * @brief   Set/override current position (for homing)
 * @param   position  New position value
 */
void stepperSetPosition(int32_t position);

/* ===== Motor Driver Control =============================================== */

/**
 * @brief   Enable motor driver (holding torque active)
 */
void stepperEnable(void);

/**
 * @brief   Disable motor driver (no torque, saves power)
 */
void stepperDisable(void);

void stepperMotorTest(void);
/* ===== Callback =========================================================== */

/**
 * @brief   Register callback for motion complete
 * @param   callback  Function called when move finishes (ISR context!)
 * @note    Keep callback short - set a flag, don't do heavy processing
 *
 * Example:
 *   volatile uint8_t move_done = 0;
 *   void onMoveDone(void) { move_done = 1; }
 *   stepperRegisterCallback(onMoveDone);
 */
void stepperRegisterCallback(stepper_complete_cb_t callback);

#endif /* STEPPER_API_H */
