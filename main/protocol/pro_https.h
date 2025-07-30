#ifndef __PRO_HTTPS_H__
#define __PRO_HTTPS_H__

#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "nvs_flash.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_http_client.h"
#include "esp_psram.h"

#include "cJSON.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#include "esp_chip_info.h"

void pro_https_send_reg(void);

#endif /* __PRO_HTTPS_H__ */