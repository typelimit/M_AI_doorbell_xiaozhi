/*
 * @Author: WHAlex
 * @Date: 2025-07-21 11:37:17
 *
 * Copyright (c) 2025 by atguigu, All Rights Reserved.
 */
#ifndef __BSP_ES8311_H__
#define __BSP_ES8311_H__

#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "soc/soc_caps.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "unity.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

#define ES8311_I2C_SDA GPIO_NUM_0
#define ES8311_I2C_SCL GPIO_NUM_1

#define ES8311_I2S_BCK GPIO_NUM_2
#define ES8311_I2S_MCK GPIO_NUM_3
#define ES8311_I2S_DIN GPIO_NUM_4
#define ES8311_I2S_WS GPIO_NUM_5
#define ES8311_I2S_DOUT GPIO_NUM_6

#define ES8311_PA_EN GPIO_NUM_7

void bsp_es8311_init(void);

void bsp_es8311_open(void);

void bsp_es8311_close(void);

void bsp_es8311_set_vol(int vol);

void bsp_es8311_set_mute(bool mute);

void bsp_es8311_read_from_mic(void *data, int len);

void bsp_es8311_write_to_speaker(void *data, int len);

#endif
