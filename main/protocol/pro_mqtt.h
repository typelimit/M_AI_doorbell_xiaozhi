#ifndef __PRO_MQTT_H__
#define __PRO_MQTT_H__

#include <stdbool.h>

void pro_mqtt_init(void);

bool pro_mqtt_send_json(const char *json);

#endif /* __PRO_MQTT_H__ */