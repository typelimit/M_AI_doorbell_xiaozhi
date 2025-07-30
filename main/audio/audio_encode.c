#include "audio_encode.h"
#include "com_debug.h"
#include "esp_audio_enc.h"
#include "esp_audio_types.h"
#include "esp_heap_caps.h"
#include "esp_opus_enc.h"
#include "freertos/idf_additions.h"
#include "freertos/ringbuf.h"
#include "portmacro.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct audio_encode {
  RingbufHandle_t input_buffer;
  RingbufHandle_t output_buffer;
  // 编码器句柄会在创建编码器对象的时候被赋到这里
  esp_audio_enc_handle_t encoder;
};

/**
 * @brief 设置编码器的输入缓冲
 *
 * @param input_buffer 输入缓冲
 * @param max_size 输入缓冲上限
 * @param data
 */
static void audio_encode_read_input_buffer(RingbufHandle_t input_buffer,
                                           int max_size, uint8_t *data) {
  size_t read_len = 0;
  void *read_data = NULL;
  while (max_size) {
    // 从缓冲区读取最大不超过 max_size 字节个数据到 read_data 当中
    read_data = xRingbufferReceiveUpTo(input_buffer, &read_len, portMAX_DELAY,
                                       max_size);

    // 把读到的数据拷贝进外部传入的指针data并进行拼包
    memcpy(data, read_data, read_len);
    // 数据缓存偏移
    data += read_len;
    max_size -= read_len;

    // 释放资源
    vRingbufferReturnItem(input_buffer, read_data);
    read_len = 0;
  }
}

void audio_encode_task(void *args) {
  MY_LOGE("编码器任务开始运行");

  audio_encode_t *audio_encode = (audio_encode_t *)args;

  // 获取输出和输出长度
  int in_size = 0, out_size = 0;
  esp_audio_enc_get_frame_size(audio_encode->encoder, &in_size, &out_size);

  // 申请输入输出内存空间
  uint8_t *in_data = (uint8_t *)malloc(in_size);
  // 也就是说，*in_data为起点in_size个字节长度的堆被malloc标记了，其它函数不许使用
  uint8_t *out_data = (uint8_t *)malloc(out_size);

  // 创建输入以及输出数据帧结构体
  esp_audio_enc_in_frame_t in_frame = {
      .buffer = in_data,
      .len = in_size,
  };
  esp_audio_enc_out_frame_t out_frame = {
      .buffer = out_data,
      .len = out_size,
  };

  while (1) {
    // 从输入缓冲区读取数据——读出来的数据放进in_data当中
    audio_encode_read_input_buffer(audio_encode->input_buffer, in_size,
                                   in_data);
    // 编码器处理数据
    esp_audio_enc_process(audio_encode->encoder, &in_frame, &out_frame);

    // 编码器输出数据写入输出缓冲区
    xRingbufferSend(audio_encode->output_buffer, out_frame.buffer,
                    out_frame.encoded_bytes, portMAX_DELAY);
  }
}
/**
 * @brief 初始化opus编码器
 *
 * @return audio_encode_t*
 */
audio_encode_t *audio_encode_init(void) {

  // 申请结构体内存空间
  audio_encode_t *audio_encode =
      (audio_encode_t *)malloc(sizeof(audio_encode_t));

  // 注册OPUS编码器
  esp_opus_enc_register();

  // opus编码信息配置
  esp_opus_enc_config_t opus_cfg = {
      .sample_rate = ESP_AUDIO_SAMPLE_RATE_16K,
      .channel = ESP_AUDIO_MONO,
      .bits_per_sample = ESP_AUDIO_BIT16,
      .bitrate = 32000,
      .frame_duration = ESP_OPUS_ENC_FRAME_DURATION_60_MS,
      .application_mode = ESP_OPUS_ENC_APPLICATION_VOIP,
      .complexity = 5,
      .enable_fec = true,
      .enable_dtx = false,
      .enable_vbr = true,
  };

  // 将opus特有配置设置进公共API配置信息中
  esp_audio_enc_config_t enc_cfg = {
      .type = ESP_AUDIO_TYPE_OPUS,
      .cfg = &opus_cfg,
      .cfg_sz = sizeof(esp_opus_enc_config_t),
  };

  // 开启编码器
  esp_audio_enc_open(&enc_cfg, &audio_encode->encoder);

  // 返回结构体对象
  return audio_encode;
}

/**
 * @brief 开启编码器任务
 *
 * @param audio_encode
 */
void audio_encode_start(audio_encode_t *audio_encode) {

  xTaskCreateWithCaps(audio_encode_task, "encode_task", 32 * 1024, audio_encode,
                      5, NULL, MALLOC_CAP_SPIRAM);
}

/**
 * @brief 设置输出缓冲区
 *
 * @param audio_encode
 * @param ring_buf
 */
void audio_encode_set_input_buffer(audio_encode_t *audio_encode,
                                   RingbufHandle_t ring_buf) {
  audio_encode->input_buffer = ring_buf;
}

/**
 * @brief 设置输出缓冲区
 *
 * @param audio_encode
 * @param ring_buf
 */
void audio_encode_set_output_buffer(audio_encode_t *audio_encode,
                                    RingbufHandle_t ring_buf) {
  audio_encode->output_buffer = ring_buf;
}