/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/12/1, v1.0 create this file.
*******************************************************************************/
#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "user_smartconfig.h"
#include "mqtt_client.h"
#include "util.h"

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
#if defined(LIGHT_DEVICE)
	user_light_init();
#elif defined(PLUG_DEVICE)
	user_plug_init();
#endif
//	sem_yunba = xSemaphoreCreateMutex();
//	xSemaphoreTake(sem_yunba, portMAX_DELAY);
    setup_wifi();
}

