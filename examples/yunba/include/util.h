/*
 * util.h
 *
 *  Created on: Oct 22, 2015
 *      Author: yunba
 */

#ifndef UTIL_H_
#define UTIL_H_


typedef struct {
	char *appkey;
	char *deviceid;
	char *alias;
	char *topic;
	unsigned short aliveinterval;
} USER_PARM_t;


int8_t init_user_parm(USER_PARM_t *user_parm);
int8_t load_user_parm(USER_PARM_t *user_parm);
void user_parm_free(USER_PARM_t *user_parm);


#endif /* UTIL_H_ */
