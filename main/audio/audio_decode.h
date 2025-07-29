#ifndef __AUDIO_DECODE_H__
#define __AUDIO_DECODE_H__

#include "esp_audio_dec.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_types.h"

#include "com_debug.h"

#include "freertos/ringbuf.h"

typedef struct audio_decode audio_decode_t;

audio_decode_t *audio_decode_init(void);

void audio_decode_start(audio_decode_t *audio_decode);
// 以结构体存储全局变量可以以“对象”为界限对不同对象的配置信息做出包装和区分

void audio_decode_stop(audio_decode_t *audio_decode);

void audio_decode_set_input_buffer(audio_decode_t *audio_decode,
                                   RingbufHandle_t ring_buf);

void audio_decode_set_output_buffer(audio_decode_t *audio_decode,
                                    RingbufHandle_t ring_buf);

#endif /* __AUDIO_DECODE_H__ */
