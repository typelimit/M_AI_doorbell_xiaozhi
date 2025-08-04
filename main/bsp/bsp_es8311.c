/*
 * @Author: WHAlex
 * @Date: 2025-07-21 11:37:14
 *
 * Copyright (c) 2025 by atguigu, All Rights Reserved.
 */
#include "bsp_es8311.h"

static esp_codec_dev_sample_info_t fs = {
    .sample_rate = 16000,
    .channel = 1,
    .bits_per_sample = 16,
    .channel_mask = ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0),
};

// Mic输入通道句柄
static i2s_chan_handle_t mic_handler;

// 扬声器输出通道句柄
static i2s_chan_handle_t speaker_handler;

// ES8311设备句柄
static esp_codec_dev_handle_t codec_dev;

/**
 * @description: I2C初始化
 */
static void bsp_es8311_i2c_init(void) {
  // 1.构建I2C配置信息
  i2c_config_t i2c_cfg = {
      .mode = I2C_MODE_MASTER,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = 100000,
      .sda_io_num = ES8311_I2C_SDA,
      .scl_io_num = ES8311_I2C_SCL,
  };

  // 2.使配置信息生效
  i2c_param_config(I2C_NUM_0, &i2c_cfg);

  // 3.安装驱动并启用
  i2c_driver_install(I2C_NUM_0, i2c_cfg.mode, 0, 0, 0);
}

/**
 * @description: I2S初始化
 */
static void bsp_es8311_i2s_init(void) {
  // 1.通道基础配置
  i2s_chan_config_t chan_cfg =
      I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);

  // 2.标准格式的配置信息
  i2s_std_config_t std_cfg = {
      // 采样率做倍频
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
      // 位深16,单声道
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(16, I2S_SLOT_MODE_MONO),
      .gpio_cfg =
          {
              .mclk = ES8311_I2S_MCK,
              .bclk = ES8311_I2S_BCK,
              .ws = ES8311_I2S_WS,
              .dout = ES8311_I2S_DOUT,
              .din = ES8311_I2S_DIN,
          },
  };

  // 3.创建输入输出通道
  i2s_new_channel(&chan_cfg, &speaker_handler, &mic_handler);

  // 4.初始化并使能通道
  i2s_channel_init_std_mode(mic_handler, &std_cfg);
  i2s_channel_init_std_mode(speaker_handler, &std_cfg);

  i2s_channel_enable(mic_handler);
  i2s_channel_enable(speaker_handler);
}

/**
 * @description: 设备初始化
 */
static void bsp_es8311_dev_init(void) {
  // 1.使用I2S配置创建数据接口
  audio_codec_i2s_cfg_t i2s_cfg = {
      .rx_handle = mic_handler,
      .tx_handle = speaker_handler,
      .port = I2S_NUM_0,
  };
  const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);

  // 2.使用I2C配置创建控制接口
  audio_codec_i2c_cfg_t i2c_cfg = {
      .addr = ES8311_CODEC_DEFAULT_ADDR, .port = I2C_NUM_0,
      // .bus_handle = 如果使用新版I2C驱动需要配置
  };
  const audio_codec_ctrl_if_t *out_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

  // 3.创建GPIO接口
  const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();

  // 4.构建编解码器配置信息并使用该配置创建编解码器接口
  es8311_codec_cfg_t es8311_cfg = {
      .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,
      .ctrl_if = out_ctrl_if,
      .gpio_if = gpio_if,
      .pa_pin = ES8311_PA_EN,
      .use_mclk = true,
  };
  const audio_codec_if_t *out_codec_if = es8311_codec_new(&es8311_cfg);

  // 5.构建设备配置信息
  esp_codec_dev_cfg_t dev_cfg = {
      .codec_if = out_codec_if,
      .data_if = data_if,
      .dev_type = ESP_CODEC_DEV_TYPE_IN_OUT,
  };

  // 6.创建设备对象
  codec_dev = esp_codec_dev_new(&dev_cfg);

  // 7.设置输入增益以及输出音量
  esp_codec_dev_set_in_gain(codec_dev, 10.0);
  esp_codec_dev_set_out_vol(codec_dev, 50);
}

/**
 * @description: ES8311设备初始化
 */
void bsp_es8311_init(void) {
  // 1.初始化I2C
  bsp_es8311_i2c_init();

  // 2.初始化I2S
  bsp_es8311_i2s_init();

  // 3.初始化ES8311设备
  bsp_es8311_dev_init();
}

/**
 * @description: 从Mic读取数据
 * @param {uint8_t} *data  读取数据存放的容器
 * @param {size_t} *len    读取到的数据长度
 */
void bsp_es8311_read_from_mic(void *data, int len) {
  if (codec_dev != NULL && len > 0) {
    esp_codec_dev_read(codec_dev, data, len);
  }
}

/**
 * @description: 将数据发送至扬声器播放
 * @param {uint8_t} *data  待播放的数据
 * @param {size_t} len     待播放的数据长度
 */
void bsp_es8311_write_to_speaker(void *data, int len) {
  if (codec_dev != NULL && len > 0) {
    esp_codec_dev_write(codec_dev, data, len);
  }
}

/**
 * @description: 开启ES8311设备
 * @return {*}
 */
void bsp_es8311_open(void) {
  // 开启设备
  printf("开启设备...\r\n");
  esp_codec_dev_open(codec_dev, &fs);
}

/**
 * @description: 关闭ES8311设备
 * @return {*}
 */
void bsp_es8311_close(void) {
  printf("关闭设备...\r\n");
  esp_codec_dev_close(codec_dev);
}

/**
 * @description: 修改音量
 * @param {int} vol
 * @return {*}
 */
void bsp_es8311_set_vol(int vol) { esp_codec_dev_set_out_vol(codec_dev, vol); }

/**
 * @description: 设置静音
 * @param {bool} mute
 * @return {*}
 */
void bsp_es8311_set_mute(bool mute) {
  esp_codec_dev_set_out_mute(codec_dev, mute);
}