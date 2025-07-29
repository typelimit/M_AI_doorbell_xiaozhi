/*
 * @Author: WHAlex
 * @Date: 2025-07-25 13:56:35
 *
 * Copyright (c) 2025 by atguigu, All Rights Reserved.
 */
#ifndef __BSP_WS2812_H__
#define __BSP_WS2812_H__

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"

// 不启用DMA
#define LED_STRIP_USE_DMA 0

// 灯带的个数
#define LED_STRIP_LED_COUNT 2
#define LED_STRIP_MEMORY_BLOCK_WORDS 0

// 引脚
#define LED_STRIP_GPIO_PIN GPIO_NUM_46
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

void bsp_ws2812_init(void);

void bsp_ws2812_Led_On(void);

void bsp_ws2812_Led_Off(void);

#endif
