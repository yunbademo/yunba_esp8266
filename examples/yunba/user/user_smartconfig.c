#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//#include "lwip/sockets.h"
//#include "lwip/dns.h"
//#include "lwip/netdb.h"
#include "smartconfig.h"
#include "user_config.h"
#include "util.h"
#include "mqtt_client.h"

void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
    printf("smartconfig_done:%d\n", status);
    switch(status) {
        case SC_STATUS_WAIT:
            printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            printf("SC_STATUS_GETTING_SSID_PSWD\n");
            sc_type *type = pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:
            printf("SC_STATUS_LINK\n");
            struct station_config *sta_conf = pdata;

	        wifi_station_set_config(sta_conf);
	        wifi_station_disconnect();
	        wifi_station_connect();
	        //change led. green
            break;
        case SC_STATUS_LINK_OVER:
            printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL) {
                uint8 phone_ip[4] = {0};

                memcpy(phone_ip, (uint8*)pdata, 4);
                printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
            }
            //change led blue
            smartconfig_stop();
            xTaskCreate(yunba_mqtt_client_task,
            		"yunba_mqtt_client_task",
            		384,//configMINIMAL_STACK_SIZE * 7,
            		NULL,
            		2,
            		NULL);
        //    xSemaphoreGive(sem_yunba);
            break;
    }
}



void
smartconfig_task(void *pvParameters)
{
    printf("smartconfig_task start\n");
    smartconfig_start(smartconfig_done);

    vTaskDelete(NULL);

    printf("------------------->\n");
}


void smartconfig() {
	//get_station_config
 //   struct station_config *sta_config5 = (struct station_config *)zalloc(sizeof(struct station_config)*5);
//    int ret = wifi_station_get_ap_info(sta_config5);
 //   free(sta_config5);
  //  printf("wifi_station_get_ap num %d\n",ret);
 //   if(0 == ret) {
        /*should be true here, Zero just for debug usage*/
 //       if(1){
            /*AP_num == 0, no ap cached,start smartcfg*/
            wifi_set_opmode(STATION_MODE);
            xTaskCreate(smartconfig_task, "smartconfig_task", 256, NULL, 10, NULL);

//            while(device_status != 0) {
//                printf("configing...\n");
//                vTaskDelay(2000 / portTICK_RATE_MS);
        //    }
 //       }
 //   }
}

void setup_wifi(void)
{
#if 0
	char *ssid = "yunba.io";
	char *pass = "123456789";
//	char *ssid = "OpenWrt";
//	char *pass = "qwertyuiop";

	struct station_config config;
	wifi_station_get_config(&config);
	bzero(config.ssid, 32);
	bzero(config.password, 64);
	memcpy(config.ssid,ssid, strlen(ssid));
	memcpy(config.password, pass, strlen(pass));
	printf("ssid=%s,password=%s\n", config.ssid, config.password);
	wifi_station_set_config(&config);

	wifi_set_opmode(STATION_MODE);
	wifi_station_connect();
#else
	smartconfig();
#endif
}

