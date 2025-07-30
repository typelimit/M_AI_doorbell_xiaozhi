#include "audio_process.h"
#include "audio_decode.h"
#include "audio_encode.h"
#include "audio_sr.h"
#include "bsp_es8311.h"
#include "com_debug.h"
#include "esp_heap_caps.h"
#include "esp_vad.h"
#include "freertos/idf_additions.h"
#include "freertos/ringbuf.h"
#include "portmacro.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct audio_process {
  audio_sr_t *audio_sr;
  audio_encode_t *audio_encode;
  audio_decode_t *audio_decode;

  RingbufHandle_t encode_input_buffer;
  RingbufHandle_t encode_output_buffer;

  RingbufHandle_t decode_input_buffer;
  RingbufHandle_t decode_output_buffer;
};

void aduio_process_task(void *args) {
  MY_LOGE("处理模块————扬声器启动");
  audio_processor_t *audio_processor = (audio_processor_t *)args;
  size_t len = 0;
  while (1) {

    void *read_data = xRingbufferReceive(audio_processor->decode_output_buffer,
                                         &len, portMAX_DELAY);

    if (len > 0) {
      bsp_es8311_write_to_speaker(read_data, len);

      vRingbufferReturnItem(audio_processor->decode_output_buffer, read_data);
      len = 0;
    }

    vTaskDelay(5);
  }
}

/**
 * @brief 音频处理模块初始化
 *
 * @return audio_processor_t*
 */
audio_processor_t *audio_process_init(void) {

  // 申请内存空间
  audio_processor_t *audio_processor =
      (audio_processor_t *)malloc(sizeof(audio_processor_t));

  audio_processor->audio_sr = audio_sr_init();
  audio_processor->audio_decode = audio_decode_init();
  audio_processor->audio_encode = audio_encode_init();

  audio_processor->encode_input_buffer = xRingbufferCreateWithCaps(
      16 * 1024, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM);
  audio_processor->encode_output_buffer = xRingbufferCreateWithCaps(
      16 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
  audio_processor->decode_input_buffer = xRingbufferCreateWithCaps(
      16 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
  audio_processor->decode_output_buffer = xRingbufferCreateWithCaps(
      16 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);

  audio_sr_set_ringbuffer(audio_processor->audio_sr,
                          audio_processor->encode_input_buffer);

  audio_encode_set_input_buffer(audio_processor->audio_encode,
                                audio_processor->encode_input_buffer);

  audio_encode_set_output_buffer(audio_processor->audio_encode,
                                 audio_processor->encode_output_buffer);

  audio_decode_set_input_buffer(audio_processor->audio_decode,
                                audio_processor->decode_input_buffer);

  audio_decode_set_output_buffer(audio_processor->audio_decode,
                                 audio_processor->decode_output_buffer);

  return audio_processor;
}

/**
 * @brief 启动音频处理模块
 *
 * @param audio_processor
 */
void audio_process_start(audio_processor_t *audio_processor) {

  xTaskCreateWithCaps(aduio_process_task, "process_task", 32 * 1024,
                      audio_processor, 5, NULL, MALLOC_CAP_SPIRAM);

  audio_decode_start(audio_processor->audio_decode);
  audio_encode_start(audio_processor->audio_encode);
  audio_sr_start(audio_processor->audio_sr);
}

void *audio_process_read_data(audio_processor_t *audio_processor, size_t *len) {
  size_t read_len = 0;
  void *read_data = xRingbufferReceive(audio_processor->encode_output_buffer,
                                       len, portMAX_DELAY);
  // 赋值
  void *data = malloc(read_len);
  memcpy(data, read_data, read_len);
  *len = read_len;

  vRingbufferReturnItem(audio_processor->encode_output_buffer, read_data);

  return data;
}

void audio_process_write_data(audio_processor_t *audio_processor, uint8_t *data,
                              size_t len) {
  xRingbufferSend(audio_processor->decode_input_buffer, data, len,
                  portMAX_DELAY);
}

void audio_process_set_wakenet_callback(audio_processor_t *audio_processor,
                                        void (*callback)(void *)) {

  audio_sr_set_wakenet_callback(audio_processor->audio_sr, callback,
                                audio_processor->audio_sr);
}

void audio_process_set_vad_callback(audio_processor_t *audio_processor,
                                    void (*callback)(void *, vad_state_t)) {

  audio_sr_set_vad_callback(audio_processor->audio_sr, callback,
                            audio_processor->audio_sr);
}

void audio_process_reset_wakenet(audio_processor_t *audio_processor) {

  audio_sr_reset_wakenet(audio_processor->audio_sr);
}