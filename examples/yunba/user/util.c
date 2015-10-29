/*
 * util.c
 *
 *  Created on: Oct 22, 2015
 *      Author: yunba
 */
#include "esp_common.h"
#include "util.h"


int8_t init_user_parm(USER_PARM_t *user_parm)
{
	uint8_t ret = 0;
	user_parm->appkey = (char *)malloc(54);
	user_parm->deviceid = (char *)malloc(54);
	user_parm->topic = (char *)malloc(54);
	user_parm->alias = (char *)malloc(54);

	if (user_parm->appkey == NULL ||
			user_parm->deviceid == NULL ||
			user_parm->topic == NULL ||
			user_parm->alias == NULL) {
		user_parm_free(user_parm);
		ret = -1;
	}
	return ret;
}

void user_parm_free(USER_PARM_t *user_parm)
{
	if (user_parm->appkey == NULL) free(user_parm->appkey);
	if (user_parm->deviceid == NULL) free(user_parm->deviceid);
	if (user_parm->topic == NULL) free(user_parm->topic);
	if (user_parm->alias == NULL) free(user_parm->alias);
}

int8_t load_user_parm(USER_PARM_t *user_parm)
{
/*  appkey 55fceaa34a481fa955f3955f
 *
 * deviceid							topic			alias				 user
 *
 * ac871c09a69c3d2d9988c9152913fa03	MN826W_34edb547	MN826W_34edb547_led0
 * 682ecfe2106beb4b2cd772f16bc42c68	MN826W_34edb547	MN826W_34edb547_wt
 * 001f40e05c7ec805795c1f09001fc9c0	MN826W_34edb547 MN826W_34edb547_led1
 * 39780fc51b216beb0b7e47694b74ab3e MN826W_34edb547 MN826W_34edb547_led2
 * 8655e6662f24bc87cce198300344f3ee MN826W_34edb547 MN826W_34edb547_plug1
*/
	//TODO:
	strcpy(user_parm->appkey, "55fceaa34a481fa955f3955f");
	strcpy(user_parm->deviceid, "8655e6662f24bc87cce198300344f3ee");
	strcpy(user_parm->topic, "MN826W_34edb547");
	strcpy(user_parm->alias, "MN826W_34edb547_plug1");
	user_parm->aliveinterval = 30;
	return 0;
}
