/*
 * @Author: WHAlex
 * @Date: 2025-07-21 16:24:31
 *
 * Copyright (c) 2025 by atguigu, All Rights Reserved.
 */
#ifndef __BSP_WIFI_H__
#define __BSP_WIFI_H__




#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include "qrcode.h"
#include "esp_system.h"

#include "../com/com_debug.h"

#define PROV_QR_VERSION "v1"
#define PROV_TRANSPORT_BLE "ble"
#define QRCODE_BASE_URL "https://espressif.github.io/esp-jumpstart/qrcode.html"

void bsp_wifi_init(void);

#endif /* __BSP_WIFI_H__ */


