/**
 * @file    stepper_hal.h
 * @brief   Stepper motor hardware abstraction layer
 * @date    06.02.2026
 *
 * Platform-specific implementation in stepper_hal.c.
 * Port this file for different MCU platforms.
 */

#ifndef STEPPER_HAL_H
#define STEPPER_HAL_H

#include <stdint.h>

/**
 * @brief   Initialize GPIO pins (enable, direction, step)
 * @note    CubeMX handles pin init, this sets default states
 */
void stepperHalInit(void);

/**
 * @brief   Generate a single step pulse on the STEP pin
 * @note    Pulse width defined by STEPPER_PULSE_WIDTH_US in config
 */
void stepperHalPulse(void);

/**
 * @brief   Set motor direction
 * @param   dir  0 = forward (CW), 1 = reverse (CCW)
 */
void stepperHalSetDirection(uint8_t dir);

/**
 * @brief   Enable motor driver (apply current to coils)
 */
void stepperHalEnable(void);

/**
 * @brief   Disable motor driver (no holding torque, saves power)
 */
void stepperHalDisable(void);

/**
 * @brief   Set timer auto-reload value (controls step rate)
 * @param   ticks  Timer ticks between interrupts
 */
void stepperHalSetTimerPeriod(uint32_t ticks);

/**
 * @brief   Start timer interrupt generation
 */
void stepperHalStartTimer(void);

/**
 * @brief   Stop timer interrupt generation
 */
void stepperHalStopTimer(void);

void stepperHalMotorTest(void);

#endif /* STEPPER_HAL_H */
