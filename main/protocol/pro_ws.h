#ifndef __PRO_WS_H__
#define __PRO_WS_H__

#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <stddef.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include <cJSON.h>

extern char *session_id;

typedef void (*text_callback)(const char *, int);
typedef void (*bin_callback)(const char *, int);

void pro_ws_set_text_callback(text_callback cb);

void pro_ws_set_bin_callback(bin_callback cb);

void pro_ws_init(void);

void pro_ws_start(void);

void pro_ws_close(void);

void pro_ws_send_hello(void);

void pro_ws_send_wakenet_word(void);

void pro_ws_send_listen(void);

void pro_ws_send_stop(void);

void pro_ws_send_abort(void);

void pro_ws_send_opus(void *data, size_t len);

#endif /* __PRO_WS_H__ */