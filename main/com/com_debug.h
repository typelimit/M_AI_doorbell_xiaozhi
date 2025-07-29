/*
 * @Author: WHAlex
 * @Date: 2025-07-21 10:55:11
 *
 * Copyright (c) 2025 by atguigu, All Rights Reserved.
 */
#ifndef __COM_DEBUG_H__
#define __COM_DEBUG_H__

#include "esp_log.h"
#include "string.h"
#include "esp_task.h"

#define DEBUG 1

extern char tag[100];
#if (DEBUG == 1)

#define MY_LOGE(format, ...)                                \
    do                                                      \
    {                                                       \
        sprintf(tag, "[%10s:%4d]", __FILENAME__, __LINE__); \
        ESP_LOGE(tag, format, ##__VA_ARGS__);               \
    } while (0)
#define MY_LOGW(format, ...)                                \
    do                                                      \
    {                                                       \
        sprintf(tag, "[%10s:%4d]", __FILENAME__, __LINE__); \
        ESP_LOGW(tag, format, ##__VA_ARGS__);               \
    } while (0)

#define MY_LOGI(format, ...)                                \
    do                                                      \
    {                                                       \
        sprintf(tag, "[%10s:%4d]", __FILENAME__, __LINE__); \
        ESP_LOGI(tag, format, ##__VA_ARGS__);               \
    } while (0)

#define MY_LOGD(format, ...)                                \
    do                                                      \
    {                                                       \
        sprintf(tag, "[%10s:%4d]", __FILENAME__, __LINE__); \
        ESP_LOGD(tag, format, ##__VA_ARGS__);               \
    } while (0)

#define MY_LOGV(format, ...)                                \
    do                                                      \
    {                                                       \
        sprintf(tag, "[%10s:%4d]", __FILENAME__, __LINE__); \
        ESP_LOGV(tag, format, ##__VA_ARGS__);               \
    } while (0)
#else
#define MY_LOGE(format, ...)
#define MY_LOGW(format, ...)
#define MY_LOGI(format, ...)
#define MY_LOGD(format, ...)
#define MY_LOGV(format, ...)
#endif

#endif
