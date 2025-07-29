#include "audio_decode.h"
#include "audio_encode.h"
#include "audio_sr.h"
#include "bsp_es8311.h"
#include "bsp_wifi.h"
#include "bsp_ws2812.h"
#include "com_debug.h"
#include "esp_avrc_api.h"
#include "esp_heap_caps.h"
#include "esp_vad.h"
#include "freertos/ringbuf.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void app_audio_wakenet_callback(void *args) { MY_LOGE("小智被唤醒..."); }

void app_audio_vad_callback(void *args, vad_state_t vad_status) {
  if (vad_status == VAD_SILENCE) {
    MY_LOGE("声音活动检测——安静");
  } else {
    MY_LOGE("声音活动检测——说话");
  }
}

void app_main(void) {

  // 初始化wifi
  // bsp_wifi_init();

  // 初始化es8311
  bsp_es8311_init();
  bsp_es8311_open();

  // led灯带初始化
  // bsp_ws2812_init();
  // bsp_ws2812_Led_On();

  // 初始化语音识别模块
  audio_sr_t *audio_sr = audio_sr_init();
  // 初始化编码器
  audio_encode_t *audio_encode = audio_encode_init();
  // 初始化解码器
  audio_decode_t *audio_decode = audio_decode_init();

  // 设置SR的环形缓冲
  RingbufHandle_t mic_ringbuf = xRingbufferCreateWithCaps(
      16 * 1024, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM);
  audio_sr_set_ringbuffer(audio_sr, mic_ringbuf);
  audio_encode_set_input_buffer(audio_encode, mic_ringbuf);

  // 设置编码器的环形缓冲
  RingbufHandle_t encode_buffer = xRingbufferCreateWithCaps(
      16 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
  audio_encode_set_output_buffer(audio_encode, encode_buffer);
  audio_decode_set_input_buffer(audio_decode, encode_buffer);

  RingbufHandle_t decode_buffer = xRingbufferCreateWithCaps(
      16 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
  audio_decode_set_output_buffer(audio_decode, decode_buffer);

  // 设置sr模块的回调
  audio_sr_set_wakenet_callback(audio_sr, app_audio_wakenet_callback, NULL);
  audio_sr_set_vad_callback(audio_sr, app_audio_vad_callback, NULL);

  // 启动对应模块
  audio_sr_start(audio_sr);
  audio_encode_start(audio_encode);
  audio_decode_start(audio_decode);

  size_t len = 0;
  while (1) {

    // 从解码器输出缓冲区读取数据
    void *data = xRingbufferReceive(decode_buffer, &len, portMAX_DELAY);

    if (len > 0) {
      // 读出的数据写入扬声器
      bsp_es8311_write_to_speaker((void *)data, len);
      // 把指针块还给解码器的输出环形缓冲
      vRingbufferReturnItem(decode_buffer, data);
    }

    vTaskDelay(10);
  }
}
