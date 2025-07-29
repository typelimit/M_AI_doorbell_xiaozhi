#ifndef __AUDIO_ENCODE_H__
#define __AUDIO_ENCODE_H__

#include "esp_audio_enc.h"
#include "esp_audio_enc_default.h"
#include "esp_audio_types.h"

#include "com_debug.h"

#include "freertos/ringbuf.h"
#include "esp_heap_caps.h"
#include "esp_task.h"



typedef struct audio_encode audio_encode_t;

audio_encode_t *audio_encode_init(void);

void audio_encode_start(audio_encode_t *audio_encode);

void audio_encode_set_input_buffer(audio_encode_t *audio_encode,RingbufHandle_t ring_buf);

void audio_encode_set_output_buffer(audio_encode_t *audio_encode,RingbufHandle_t ring_buf);


#endif /* __AUDIO_ENCODE_H__ */