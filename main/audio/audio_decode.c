#include "audio_decode.h"
#include "com_debug.h"
#include "esp_audio_dec.h"
#include "esp_audio_types.h"
#include "esp_heap_caps.h"
#include "esp_opus_dec.h"
#include "esp_pcm_dec.h"
#include "freertos/idf_additions.h"
#include "freertos/ringbuf.h"
#include "portmacro.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

struct audio_decode {
  RingbufHandle_t input_buffer;
  RingbufHandle_t output_buffer;
  esp_audio_dec_handle_t decoder;
  uint8_t is_running;
};

void audio_decode_task(void *args) {
  MY_LOGE("解码器任务开始执行...");
  // 从任意指针转为自定义的音频解码指针
  audio_decode_t *audio_decode = (audio_decode_t *)args;

  esp_audio_dec_in_raw_t in_raw = {0};

  uint32_t out_size = 1920;                        // 计算输出缓冲大小
  uint8_t *out_data = (uint8_t *)malloc(out_size); // 按计算结果生成缓冲
  esp_audio_dec_out_frame_t out_frame = {
      .buffer = out_data,
      .len = out_size,
  };

  while (audio_decode->is_running) {

    // 从输入缓冲读数据
    size_t len = 0;
    void *read_data =
        xRingbufferReceive(audio_decode->input_buffer, &len, portMAX_DELAY);

    // 将数据赋值进入输入缓冲
    in_raw.buffer = read_data;
    in_raw.len = len;

    while (in_raw.len > 0) {

      // 开始解码处理
      esp_audio_dec_process(audio_decode->decoder, &in_raw, &out_frame);
      // 解码器输出结果写入输出缓冲
      xRingbufferSend(audio_decode->output_buffer, out_frame.buffer,
                      out_frame.decoded_size, portMAX_DELAY);
      // MY_LOGE("解码器输出字节数 = %" PRIu32, out_frame.decoded_size);

      // 将已经解码的数据指针后移=>拼包
      in_raw.buffer += in_raw.consumed;
      in_raw.len -= in_raw.consumed;
    }

    // 将资源归还给环形缓冲
    vRingbufferReturnItem(audio_decode->input_buffer, read_data);
  }
  // 释放资源
  free(out_data);
  // 任务自杀
  vTaskDelete(NULL);
}

/**
 * @brief 初始化opus解码器
 *
 * @return audio_decode_t*
 */
audio_decode_t *audio_decode_init(void) {

  // 给结构体对象分配内存
  audio_decode_t *audio_decode =
      (audio_decode_t *)malloc(sizeof(audio_decode_t));

  audio_decode->is_running = 0; // 初始化的时候给它置零

  // 注册opus解码器
  esp_opus_dec_register(); // 刚才把解码器注册成pcm了！

  // 配置信息
  esp_opus_dec_cfg_t opus_cfg = {
      .sample_rate = ESP_AUDIO_SAMPLE_RATE_16K,
      .channel = ESP_AUDIO_MONO,
      .frame_duration = ESP_OPUS_DEC_FRAME_DURATION_60_MS,
      .self_delimited = false,
  };
  esp_audio_dec_cfg_t dec_cfg = {
      .type = ESP_AUDIO_TYPE_OPUS,
      .cfg = &opus_cfg,
      .cfg_sz = sizeof(esp_opus_dec_cfg_t),
  };

  // 启动解码器
  esp_audio_dec_open(
      &dec_cfg,
      &audio_decode->decoder); // 这里用的audio_decode->decoder应该是二级指针

  // 返回结构体
  return audio_decode;
}

/**
 * @brief 启动解码器
 *
 * @param audio_decode
 * @return audio_decode_t*
 */
void audio_decode_start(audio_decode_t *audio_decode) {
  audio_decode->is_running = 1; // 执行开始编码给它置1
  xTaskCreateWithCaps(audio_decode_task, "decode_task", 32 * 1024, audio_decode,
                      5, NULL, MALLOC_CAP_SPIRAM);
}

void audio_decode_stop(audio_decode_t *audio_decode) {

  audio_decode->is_running = 0;
}

void audio_decode_set_input_buffer(audio_decode_t *audio_decode,
                                   RingbufHandle_t ring_buf) {

  audio_decode->input_buffer = ring_buf;
}

void audio_decode_set_output_buffer(audio_decode_t *audio_decode,
                                    RingbufHandle_t ring_buf) {

  audio_decode->output_buffer = ring_buf;
}