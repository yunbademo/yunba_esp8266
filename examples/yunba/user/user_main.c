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

//#include "lwip/sockets.h"
//#include "lwip/dns.h"
//#include "lwip/netdb.h"
#include "smartconfig.h"

#include "MQTTClient.h"
#include "json/cJSON.h"

const char *DEV_ALIAS = "MN826W_34edb547_morlinks_mn826w";

typedef enum {
	ST_INIT, ST_CONNECT, ST_REG, ST_SUB, ST_PUBLISH, ST_RUNNING, ST_DIS
} MQTT_STATE_t;

MQTT_STATE_t MQTT_State = ST_INIT;

LOCAL uint8 device_status = -1;


void
smartconfig_done(sc_status status, void *pdata)
{
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
            wifi_set_opmode(STATION_MODE); //.....
            wifi_station_connect();
            break;
        case SC_STATUS_LINK_OVER:
            printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL) {
                uint8 phone_ip[4] = {0};
                memcpy(phone_ip, (uint8*)pdata, 4);
                printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
            }
            smartconfig_stop();

            device_status = 0;
            break;
    }

}


void
smartconfig_task(void *pvParameters)
{
    printf("smartconfig_task start\n");
    smartconfig_start(smartconfig_done);

    vTaskDelete(NULL);
}


void smartconfig() {
 //   struct station_config *sta_config5 = (struct station_config *)zalloc(sizeof(struct station_config)*5);
//    int ret = wifi_station_get_ap_info(sta_config5);
 //   free(sta_config5);
  //  printf("wifi_station_get_ap num %d\n",ret);
 //   if(0 == ret) {
        /*should be true here, Zero just for debug usage*/
        if(1){
            /*AP_num == 0, no ap cached,start smartcfg*/
            wifi_set_opmode(STATION_MODE);
            xTaskCreate(smartconfig_task, "smartconfig_task", 256, NULL, 2, NULL);

            while(device_status != 0){
                printf("configing...\n");
                vTaskDelay(2000 / portTICK_RATE_MS);
            }
        }
 //   }
}


void messageArrived(MessageData* data)
{
	if (data->message->payloadlen > 256 || data->topicName->lenstring.len > 100)
		return;

	uint8_t *buf = (uint8_t *)zalloc(256);

	if (buf) {
		int8_t *topic = (int8_t *)zalloc(100);

		if (!topic) printf("can't get mem\n");

		memset(buf, 0, 256);
		memcpy(buf, data->message->payload, data->message->payloadlen);
		printf("message arrive,msg: %s\n", buf);
		memset(topic, 0, 100);
		memcpy(topic, data->topicName->lenstring.data, data->topicName->lenstring.len);
		printf("message arrive,topic: %s\n", topic);
		if (strcmp(topic, DEV_ALIAS) == 0) {
			cJSON *root = cJSON_Parse(buf);
			//{p:period, r:red, g:green, b:blue}
			if (root) {
				int ret_size = cJSON_GetArraySize(root);
				if (ret_size >= 4) {
					uint16_t period = cJSON_GetObjectItem(root,"p")->valueint;
					uint16_t red = cJSON_GetObjectItem(root,"r")->valueint;
					uint16_t green = cJSON_GetObjectItem(root,"g")->valueint;
					uint16_t blue = cJSON_GetObjectItem(root,"b")->valueint;
					light_set_aim(red, green, blue, 0, 0, period);
					printf("light parm:%d,%d, %d, %d\n", period, red, green, blue);
				}
				cJSON_Delete(root);
			}
		}
		free(topic);
		free(buf);
	}
}

static void extMessageArrive(EXTED_CMD cmd, int status, int ret_string_len, char *ret_string)
{
	uint8_t *buf = (uint8_t *)malloc(200);
	if (buf) {
			memset(buf, 0, 200);
			if (ret_string_len <= 200) {
				memcpy(buf, ret_string, ret_string_len);
				printf("%s, cmd:%d, status:%d, payload: %s\n", __func__, cmd, status, buf);
			}
			free(buf);
	}
}


void setup_wifi(void)
{
#if 1
	char *ssid = "yunba.io 2";
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


const portTickType xDelay = 1000 / portTICK_RATE_MS;

static int get_ip_pair(const char *url, char *addr, uint16_t *port)
{
	char *p = strstr(url, "tcp://");
	if (p) {
		p += 6;
		char *q = strstr(p, ":");
		if (q) {
			int len = strlen(p) - strlen(q);
			if (len > 0) {
				memcpy(addr, p, len);
				//sprintf(addr, "%.*s", len, p);
				*port = atoi(q + 1);
				return SUCCESS;
			}
		}
	}
	return FAILURE;
}


void yunba_get_mqtt_broker(char *appkey, char *deviceid, char *broker_addr, uint16_t *port, REG_info *reg)
{
    char *url = (char *)malloc(128);

    if (url) {
        memset(url, 0, 128);
        do {
        	if (MQTTClient_get_host_v2(appkey, url) == SUCCESS)
        		break;
        	vTaskDelay(30 / portTICK_RATE_MS);
        } while (1);
        printf("get url: %s\n", url);
        get_ip_pair(url, broker_addr, port);
        free(url);
    }

    do {
    	if (MQTTClient_setup_with_appkey_v2(appkey, deviceid, reg) == SUCCESS)
    		break;
    	vTaskDelay(30 / portTICK_RATE_MS);
    } while (1);
}


void ICACHE_FLASH_ATTR
yunba_mqtt_client_task(void *pvParameters)
{
    MQTTClient client;
    Network network;
//    unsigned char sendbuf[200], readbuf[200];
    int rc = 0, count = 0;

    uint8_t *sendbuf = (uint8_t *)malloc(200);
    uint8_t *readbuf = (uint8_t *)malloc(200);

    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    vTaskDelay(1000 / portTICK_RATE_MS);
    pvParameters = 0;
    NetworkInit(&network);
    MQTTClientInit(&client, &network, 30000, sendbuf, 200 * sizeof(uint8_t), readbuf, 200 * sizeof(uint8_t));

    REG_info reg;
    reg.client_id = (char *)malloc(56);
    reg.device_id = (char *)malloc(56);
    reg.password = (char *)malloc(56);
    reg.username = (char *)malloc(56);
    char *addr = (char *)malloc(28);
    uint16_t port;
    memset(addr, 0, 28);
    yunba_get_mqtt_broker("55fceaa34a481fa955f3955f", "ac871c09a69c3d2d9988c9152913fa03", addr, &port, &reg);

    printf("get mqtt broker->%s:%d\n", addr, port);
    printf("get reg info: cid:%s, username:%d, password:%s, devid:%s\n",
    		reg.client_id, reg.username, reg.password, reg.device_id);

#if defined(MQTT_TASK)
    if ((rc = MQTTStartTask(&client)) != pdPASS)
    	os_printf("Return code from start tasks is %d\n", rc);
#endif

    MQTTSetCallBack(&client, messageArrived, extMessageArrive);

    for(;;) {
    	printf("yunba_mqtt_client_task %d word left\n",uxTaskGetStackHighWaterMark(NULL));
    	switch (MQTT_State) {
    	case ST_INIT:
    	{
			if ((rc = NetworkConnect(&network, addr, port)) != 0)
//    		if ((rc = NetworkConnect(&network, "123.56.225.70", 1883)) != 0)
				os_printf("Return code from network connect is %d\n", rc);
    		else
    			printf("net connect: %d\n", rc);
    		MQTT_State = ST_CONNECT;
    	}
    		break;

    	case ST_CONNECT:
    	{
       	    connectData.MQTTVersion = 19;//3;
			connectData.clientID.cstring = reg.client_id;
			connectData.username.cstring = reg.username;
			connectData.password.cstring = reg.password;
			connectData.keepAliveInterval = 30;
//       	    connectData.clientID.cstring = "0000002557-000000000011";
//       	    connectData.username.cstring = "2531052302704246144";
//       	    connectData.password.cstring = "0d0bb10fd99e3";

			if ((rc = MQTTConnect(&client, &connectData)) != 0)
				os_printf("Return code from MQTT connect is %d\n", rc);
			else
				os_printf("MQTT Connected\n");
			MQTT_State = ST_REG;
			free(reg.client_id);
			free(reg.device_id);
			free(reg.username);
			free(reg.password);
    	}
     		break;
    	case ST_REG:
    	{
    		if ((rc = MQTTSubscribe(&client, "MN826W_34edb547", QOS1, messageArrived)) != 0)
    			os_printf("Return code from MQTT subscribe is %d\n", rc);
    		else
    			os_printf("subscribe: %d\n", rc);
    		MQTT_State = ST_SUB;
    	}
    		break;

    	case ST_SUB:
    	{
//    	    MQTTMessage message;
//    	    char payload[30];
//
//    	    message.qos = 1;
//    	    message.retained = 0;
//    	    message.payload = payload;
//    	    sprintf(payload, "message number %d", count);
//    	    message.payloadlen = strlen(payload);
//
//        	os_printf("------>publish\n");
//        	 if ((rc = MQTTPublish(&client, "MN826W_34edb547", &message)) != 0)
//        	            printf("Return code from MQTT publish is %d\n", rc);

        	 MQTTSetAlias(&client, DEV_ALIAS);
//        	 MQTTGetAlias(&client, "MN826W_34edb547");

        	// rc = MQTTPublishToAlias(&client, "MN826W_34edb547_morlinks_mn826w", payload, message.payloadlen);
        	 MQTT_State = ST_RUNNING;
    	}
    		break;
    	case ST_RUNNING:
    	{
//    	    MQTTMessage message;
//        	    char payload[30];
//
//        	    message.qos = 1;
//        	    message.retained = 0;
//        	    message.payload = payload;
//                sprintf(payload, "message number %d", count++);
//                message.payloadlen = strlen(payload);
//          	 if ((rc = MQTTPublish(&client, "MN826W_34edb547", &message)) != 0)
//          	            printf("Return code from MQTT publish is %d\n", rc);
 //         	os_printf("------>running, publish\n");
    	}

    		break;

    	default:

    		break;
    	}

    	vTaskDelay(xDelay);

#if !defined(MQTT_TASK)
            if ((rc = MQTTYield(&client, 1000)) != 0)
                    printf("Return code from yield is %d\n", rc);
#endif
    }

//    free(reg.client_id);
//    free(reg.device_id);
//    free(reg.password);
//    free(reg.username);
    free(sendbuf);
    free(readbuf);

    free(addr);

	vTaskDelete(NULL);
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
	setup_wifi();
	user_light_init();
    xTaskCreate(yunba_mqtt_client_task,
    		"smartconfig_task",
    		392,//configMINIMAL_STACK_SIZE * 7,
    		NULL,
    		2,
    		NULL);
}

