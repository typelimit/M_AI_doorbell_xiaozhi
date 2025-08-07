#ifndef __PRO_MQTT_H__
#define __PRO_MQTT_H__

#include "cJSON.h"
#include <stdbool.h>

void pro_mqtt_init(void);

bool pro_mqtt_send_json(const char *json);

bool pro_mqtt_is_connected(void);

void send_hello_to_mqtt(void);

#endif /* __PRO_MQTT_H__ */