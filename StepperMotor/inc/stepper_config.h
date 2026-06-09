/**
 * @file    stepper_config.h
 * @brief   Stepper motor configuration - edit for your hardware
 * @date    06.02.2026
 */

#ifndef STEPPER_CONFIG_H
#define STEPPER_CONFIG_H

/* ===== Timer Configuration ================================================ */

/** Timer handle */
//#define STEPPER_TIMER_HANDLE        TIMER1

/** Timer clock frequency after prescaler (Hz)
 *  Example: 72 MHz APB / PSC=71 => 1 MHz = 1 us/tick */
#define STEPPER_TIMER_FREQ_HZ       1000000UL

/* ===== Speed Limits ======================================================= */

/** Minimum speed in pulses per second (startup speed) */
#define STEPPER_MIN_SPEED_PPS       10

/** Maximum allowable speed in pulses per second */
#define STEPPER_MAX_SPEED_PPS       1000


/* ===== Hardware Timing ==================================================== */

/** Step pulse HIGH duration in microseconds
 *  A4988 requires minimum 1 us, DRV8825 requires 1.9 us */
#define STEPPER_PULSE_WIDTH_US      2

/** System core clock in Hz (for pulse delay calculation) */
#define STEPPER_SYSTEM_CLOCK_HZ     48000000UL

/* ===== Driver IC Configuration ============================================ */

/** Enable pin active level: 0 = active LOW (A4988), 1 = active HIGH */
#define STEPPER_ENABLE_ACTIVE_LOW   1
#define STEPPER_EN_PIN_ENABLE 		1

#endif /* STEPPER_CONFIG_H */
