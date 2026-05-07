#ifndef DEVICE_FLASH_OPERATIONS_H
#define DEVICE_FLASH_OPERATIONS_H

#include "flash_manager.h"
extern flash_config_t config;

//USER DATA
extern param_descriptor_t temp_setpoint;

void flashCheckFirstBoot(void);
void flashRegisterParams(void);
flash_result_t flashSaveDefaults(param_category_t category);



#endif

