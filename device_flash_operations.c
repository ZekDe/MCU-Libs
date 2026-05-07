/**
* @Author: Emrah Duatepe
*/
#include "device_flash_operations.h"
#include "device_context.h"
#include "stdio.h"
#include "systemtick.h"
#include <string.h>

//#define DEBUG_DEVICE_FLASH
//todo: 30000 yapilacak
#define DEVICE_WRITE_FLASH_TIMEOUT 	30000 //  dirty 1 oldugunda flash'a yazmak için bekleyecegi süre(ms)


flash_config_t config = {
	.write_delay_ms = DEVICE_WRITE_FLASH_TIMEOUT,        // DEVICE_WRITE_FLASH_TIMEOUT ms gecikme
	.verification_enabled = 1       					 // Dogrulama aktif
};

/******************************* USER DATA BEGIN *****************************************/
param_descriptor_t temp_setpoint = 
{
	.id = 1,
	.category = PARAM_CAT_DYNAMIC,
	.size = sizeof(float),
	.ram_ptr = &dev_data.temp_setpoint,
};





/******************************* USER DATA END *****************************************/
void flashRegisterParams(void)
{
	flashManagerRegisterParam(&temp_setpoint);
}


void flashCheckFirstBoot(void)
{
	uint8_t valid_sectors[PARAM_CAT_COUNT] = {0};
	flashManagerGetValidSectors(valid_sectors);
	
	config.write_delay_ms = DEVICE_WRITE_FLASH_TIMEOUT;
	
	// Her kategori için ayri kontrol
    for (uint8_t cat = 0; cat < PARAM_CAT_COUNT; ++cat) 
	{
        param_category_t category = (param_category_t)cat;
        
        if (!valid_sectors[cat]) 
		{
            // Geçersiz - default yaz
			#ifdef DEBUG_DEVICE_FLASH
            printf("Category %d invalid, writing defaults...\n", cat);
			#endif
            (void)flashSaveDefaults(category);
        } 
		else 
		{
            // Geçerli - yükle
			#ifdef DEBUG_DEVICE_FLASH
            printf("Category %d valid, loading...\n", cat);
			#endif
            flashManagerLoadCategory(category);
        }
    }
}


/**
 * @brief Factory default degerleri RAM'e yükle
 */
void flashSetFactoryDefaultsToRam(void)
{
	dev_data.temp_setpoint	   = 	40;
}

/**
 * @brief Belirtilen kategoriye default degerleri yükle ve flash'a yaz
 */
flash_result_t flashSaveDefaults(param_category_t category)
{
	flash_result_t result = FLASH_OK;
	
	switch(category)
	{
		case PARAM_CAT_DYNAMIC:
			#ifdef DEBUG_DEVICE_FLASH
            printf("PARAM_CAT_DYNAMIC...\n");
			#endif
			flashSetFactoryDefaultsToRam();		
			flashManagerForceWrite(temp_setpoint.id); // DYNAMIC olanlardan bir tanesi yeterli
			
		break;
		
		case PARAM_CAT_STATIC:
			#ifdef DEBUG_DEVICE_FLASH
            printf("PARAM_CAT_STATIC...\n");
			#endif
		break;
		
		case PARAM_CAT_SYSTEM:
			#ifdef DEBUG_DEVICE_FLASH
            printf("PARAM_CAT_SYSTEM...\n");
			#endif
		break;
		
		default:
			result = FLASH_ERROR_INVALID_PARAM;
		break;
	}
	return result;
}

