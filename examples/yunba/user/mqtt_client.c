/*
 * mqtt_client.c
 *
 *  Created on: Oct 22, 2015
 *      Author: yunba
 */


#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "smartconfig.h"

#include "MQTTClient.h"
#include "json/cJSON.h"

#include "util.h"

#define YUNBA_MQTT_VER   19

//const char *DEV_ALIAS = "MN826W_34edb547_led0";
//const char *DEV_ALIAS = "MN826W_34edb547_wirelesstag";

 USER_PARM_t parm;

const portTickType xDelay = 1000 / portTICK_RATE_MS;


typedef enum {
	ST_INIT, ST_CONNECT, ST_REG, ST_SUB, ST_PUBLISH, ST_RUNNING, ST_DIS, ST_RECONN
} MQTT_STATE_t;

MQTT_STATE_t MQTT_State = ST_INIT;

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
		printf("message arrive:\nmsg: %s\n", buf);
		memset(topic, 0, 100);
		memcpy(topic, data->topicName->lenstring.data, data->topicName->lenstring.len);
		printf("topic: %s\n", topic);
		if (strcmp(topic, parm.alias) == 0) {
			cJSON *root = cJSON_Parse(buf);
			//{p:period, r:red, g:green, b:blue}
			if (root != NULL) {
				int ret_size = cJSON_GetArraySize(root);
#if defined(LIGHT_DEVICE)
				if (ret_size >= 4) {
					uint16_t period = cJSON_GetObjectItem(root,"p")->valueint;
					uint16_t red = cJSON_GetObjectItem(root,"r")->valueint;
					uint16_t green = cJSON_GetObjectItem(root,"g")->valueint;
					uint16_t blue = cJSON_GetObjectItem(root,"b")->valueint;
					printf("light parm:%d,%d, %d, %d\n", period, red, green, blue);
					light_set_aim(red, green, blue, 0, 0, period);
				}
#elif defined(PLUG_DEVICE)
				if (ret_size >= 2) {
					//uint16_t device_type = cJSON_GetObjectItem(root,"t")->valuestring;
					cJSON * pJstatus = cJSON_GetObjectItem(root,"switch");
					if (pJstatus != NULL) {
						if(pJstatus->type == cJSON_Number){
							printf("switch set\n");
							user_plug_set_status(pJstatus->valueint);
						}
					}
				}
#endif
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
    int rc = 0, count = 0;
    uint8_t *sendbuf = (uint8_t *)malloc(200);
    uint8_t *readbuf = (uint8_t *)malloc(200);

    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

    if (init_user_parm(&parm) == 0)
    	load_user_parm(&parm);

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
    yunba_get_mqtt_broker(parm.appkey, parm.deviceid, addr, &port, &reg);

    printf("get mqtt broker->%s:%d\n", addr, port);
    printf("get reg info: cid:%s, username:%d, password:%s, devid:%s\n",
    		reg.client_id, reg.username, reg.password, reg.device_id);

    MQTTSetCallBack(&client, messageArrived, extMessageArrive);

    for(;;) {
    	switch (MQTT_State) {
    	case ST_INIT:
    	{
			if ((rc = NetworkConnect(&network, addr, port)) != 0)
				os_printf("Return code from network connect is %d\n", rc);
    		else
    			printf("net connect: %d\n", rc);
    		MQTT_State = ST_CONNECT;
    	}
    		break;

    	case ST_CONNECT:
    	{
    		connectData.MQTTVersion = YUNBA_MQTT_VER;
			connectData.clientID.cstring = reg.client_id;
			connectData.username.cstring = reg.username;
			connectData.password.cstring = reg.password;
			connectData.keepAliveInterval = parm.aliveinterval;

			if ((rc = MQTTConnect(&client, &connectData)) != 0) {
				os_printf("Return code from MQTT connect is %d\n", rc);
			}
			else {
				os_printf("MQTT Connected\n");
			#if defined(MQTT_TASK)
				if ((rc = MQTTStartTask(&client)) != pdPASS)
					os_printf("Return code from start tasks is %d\n", rc);
			#endif
				MQTT_State = ST_REG;
			}
    	}
     		break;
    	case ST_REG:
    	{
    		if ((rc = MQTTSubscribe(&client, parm.topic, QOS1, messageArrived)) != 0)
    			os_printf("Return code from MQTT subscribe is %d\n", rc);
    		else
    			os_printf("subscribe: %d\n", rc);
    		MQTT_State = ST_SUB;
    	}
    		break;

    	case ST_SUB:
    		MQTTClient_presence(&client, parm.topic);
    		MQTTSetAlias(&client, parm.alias);
        	MQTT_State = ST_RUNNING;
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

		case ST_RECONN:
		{
			if (MQTTConnect(&client, &connectData) == 0) {
				MQTT_State = ST_RUNNING;
			}
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

    free(reg.client_id);
    free(reg.device_id);
    free(reg.password);
    free(reg.username);
    free(sendbuf);
    free(readbuf);

    free(addr);
    user_parm_free(&parm);

	vTaskDelete(NULL);
}




