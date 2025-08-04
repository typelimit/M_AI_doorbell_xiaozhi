#ifndef __AUDIO_SR_H__
#define __AUDIO_SR_H__
#include "bsp_es8311.h"
#include "com_debug.h"
#include "com_status.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_heap_caps.h"
#include "esp_task.h"
#include "esp_vad.h"
#include "freertos/ringbuf.h"

// 这里被声明的结构体，它所承载的信息是什么？
typedef struct audio_sr audio_sr_t;

audio_sr_t *audio_sr_init(void);

void audio_sr_start(audio_sr_t *audio_sr);

void audio_sr_set_ringbuffer(audio_sr_t *audio_sr, RingbufHandle_t ringbuf);

void audio_sr_set_wakenet_callback(audio_sr_t *audio_sr,
                                   void (*callback)(void *), void *args);

void audio_sr_set_vad_callback(audio_sr_t *audio_sr,
                               void (*callback)(void *, vad_state_t),
                               void *args);

void audio_sr_reset_wakenet(audio_sr_t *audio_sr);

#endif /* __AUDIO_SR_H__ */