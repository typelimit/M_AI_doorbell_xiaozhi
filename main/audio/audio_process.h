#ifndef __AUDIO_PROCESS_H__
#define __AUDIO_PROCESS_H__

#include "audio_decode.h"
#include "audio_encode.h"
#include "audio_sr.h"
#include "esp_vad.h"
#include <stddef.h>
#include <stdint.h>

typedef struct audio_process audio_processor_t;

audio_processor_t *audio_process_init(void);

void audio_process_start(audio_processor_t *audio_processor);

void *audio_process_read_data(audio_processor_t *audio_processor, size_t *len);

void audio_process_write_data(audio_processor_t *audio_processor, uint8_t *data,
                              size_t len);

void audio_process_set_wakenet_callback(audio_processor_t *audio_processor,
                                        void (*call_back)(void *));

void audio_process_set_vad_callback(audio_processor_t *audio_processor,
                                    void (*callback)(void *, vad_state_t));

void audio_process_reset_wakenet(audio_processor_t *audio_processor);

#endif /* __AUDIO_PROCESS_H__ */