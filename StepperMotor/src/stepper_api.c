/**
 * @file    stepper_api.c
 * @brief   Stepper motor public API implementation
 * @date    06.02.2026
 *
 * Thin wrapper over stepper_motion module.
 * Add application-specific logic here without modifying motion engine.
 */

#include "stepper_api.h"
#include "stepper_motion.h"
#include "stepper_hal.h"

/* ===== Initialization ===================================================== */

void stepperInit(void)
{
   stepperMotionInit();
}

/* ===== Motion Control ===================================================== */

int8_t stepperMove(int32_t steps, uint16_t max_speed_pps, uint16_t accel_pps2)
{
   return stepperMotionStart(steps, max_speed_pps, accel_pps2);
}

int8_t stepperMoveTimed(int32_t steps, uint16_t max_speed_pps, uint16_t ramp_time_ms)
{
   return stepperMotionStartTimed(steps, max_speed_pps, ramp_time_ms);
}

void stepperStop(void)
{
   stepperMotionStop();
}

void stepperEmergencyStop(void)
{
   stepperMotionEmergencyStop();
}

/* ===== Status ============================================================= */

uint8_t stepperIsBusy(void)
{
   return stepperMotionIsBusy();
}

int32_t stepperGetPosition(void)
{
   return stepperMotionGetPosition();
}

void stepperSetPosition(int32_t position)
{
   stepperMotionSetPosition(position);
}

/* ===== Motor Driver Control =============================================== */

void stepperEnable(void)
{
   stepperHalEnable();
}

void stepperDisable(void)
{
   stepperHalDisable();
}

void stepperMotorTest(void)
{
	stepperHalMotorTest();
}

/* ===== Callback =========================================================== */

void stepperRegisterCallback(stepper_complete_cb_t callback)
{
   stepperMotionRegisterCallback((stepper_callback_t)callback);
}
