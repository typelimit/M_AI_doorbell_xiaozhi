#include "audio_sr.h"
#include "bsp_es8311.h"
#include "com_debug.h"
#include "esp_afe_config.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_vad.h"
#include "esp_wn_iface.h"
#include "freertos/ringbuf.h"
#include "portmacro.h"
#include "protocomm_console.h"
#include <stdint.h>
#include <stdlib.h>

struct audio_sr {
  esp_afe_sr_iface_t *afe_handle;
  esp_afe_sr_data_t *afe_data;
  void (*wakenet_callback)(void *);
  void *wakenet_args;

  void (*vad_callback)(void *, vad_state_t);
  void *vad_args;

  uint8_t is_waked;
  vad_state_t last_vad_state;

  RingbufHandle_t ringbuffer;
};
// 喂数据任务
void audio_sr_feed_task(void *args) {

  MY_LOGE("audio feed task 开始运行...");

  audio_sr_t *audio_sr = (audio_sr_t *)args;
  esp_afe_sr_iface_t *afe_handle = audio_sr->afe_handle;
  esp_afe_sr_data_t *afe_data = audio_sr->afe_data;

  // 获取单通道输入数据大小
  int feed_chunksize = afe_handle->get_feed_chunksize(afe_data);
  // 获取通道数
  int feed_nch = afe_handle->get_feed_channel_num(afe_data);
  // 计算单次输入数据大小(最大值)
  size_t len = feed_chunksize * feed_nch * sizeof(int16_t);

  // 根据计算出的大小对声明的指针进行内存分配
  int16_t *feed_buff = (int16_t *)malloc(len);

  while (1) {

    // 从mic读数据
    bsp_es8311_read_from_mic((void *)feed_buff, len);
    //  将数据写入AFE框架
    afe_handle->feed(afe_data, feed_buff);

    vTaskDelay(10);
  }
}
// 取数据任务
void audio_sr_fetch_task(void *args) {

  MY_LOGE("audio fetch task 开始运行...");

  audio_sr_t *audio_sr = (audio_sr_t *)args;
  esp_afe_sr_iface_t *afe_handle = audio_sr->afe_handle;
  esp_afe_sr_data_t *afe_data = audio_sr->afe_data;

  // 获取单通道输入数据大小
  int feed_chunksize = afe_handle->get_feed_chunksize(afe_data);
  // 获取通道数
  int feed_nch = afe_handle->get_feed_channel_num(afe_data);
  // 计算单次输入数据大小(最大值)
  size_t len = feed_chunksize * feed_nch * sizeof(int16_t);
  while (1) {
    // 获取语音活动检测结果
    afe_fetch_result_t *result = afe_handle->fetch(afe_data);
    // 获取音频结果
    int16_t *processed_audio = result->raw_data;
    // 获取语音活动检测状态
    vad_state_t vad_state = result->vad_state;
    // 获取唤醒状态
    wakenet_state_t wakeup_state = result->wakeup_state;
    
    // 判断当前是否为唤醒词
    if (wakeup_state == WAKENET_DETECTED) {
      // 修改唤醒状态
      audio_sr->is_waked = 1;
      if (audio_sr->wakenet_callback) {
        audio_sr->wakenet_callback(audio_sr->wakenet_args);
      }
    }
    
    // 若当前为唤醒状态则处理语音数据
    if (audio_sr->is_waked) {
      
      // 当语音识别状态变化时需要调用回调函数
      if (vad_state != audio_sr->last_vad_state) {
        
        // 当前语音检测状态赋给上一次
        audio_sr->last_vad_state = vad_state;
        
        if (audio_sr->vad_callback) {
          audio_sr->vad_callback(audio_sr->vad_args, vad_state);
        }
      }
      // 若当前有人说话则保留语音
      if (vad_state == VAD_SPEECH) {
        MY_LOGE("开始循环抓取语音...");
        xRingbufferSend(audio_sr->ringbuffer, processed_audio, len,
                        portMAX_DELAY);
      }
    }
    vTaskDelay(10); // 刚才把taskdelay放在while外面了，导致任务没拉起来
  }
}

audio_sr_t *audio_sr_init(void) {
  // 1.申请资源
  audio_sr_t *audio_sr = malloc(sizeof(audio_sr_t));
  audio_sr->is_waked = 0;
  audio_sr->last_vad_state = VAD_SILENCE;
  // 2.初始化AFE，以及创建实例
  // 2.1加载模型以及配置AFE
  srmodel_list_t *models = esp_srmodel_init("model");
  afe_config_t *afe_config =
      afe_config_init("M", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
  afe_config->aec_init = false;
  afe_config->se_init = false;
  afe_config->ns_init = false;
  afe_config->agc_init = false;

  afe_config->vad_init = true;
  afe_config->vad_mode = VAD_MODE_1;
  afe_config->vad_min_speech_ms = 128;
  afe_config->wakenet_init = true;
  afe_config->wakenet_mode = DET_MODE_95;
  afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;

  // 2.2创建AFE实例
  // 获取句柄
  audio_sr->afe_handle = esp_afe_handle_from_config(afe_config);
  // 创建实例
  audio_sr->afe_data = audio_sr->afe_handle->create_from_config(afe_config);

  // 3.返回对象
  return audio_sr;
}

void audio_sr_start(audio_sr_t *audio_sr) {
  xTaskCreate(audio_sr_feed_task, "feed task", 32 * 1024, audio_sr, 5, NULL);
  xTaskCreate(audio_sr_fetch_task, "fetch task", 32 * 1024, audio_sr, 6, NULL);
}
/**
 * @brief 设置输出环形缓冲区
 *
 * @param audio_sr
 * @param ringbuf
 */
void audio_sr_set_ringbuffer(audio_sr_t *audio_sr, RingbufHandle_t ringbuf) {
  audio_sr->ringbuffer = ringbuf;
}

/**
 * @brief 设置唤醒回调函数
 *
 * @param audio_sr
 * @param callback
 * @param args
 */
void audio_sr_set_wakenet_callback(audio_sr_t *audio_sr,
                                   void (*callback)(void *), void *args) {
  audio_sr->wakenet_callback = callback;
  audio_sr->wakenet_args = args;
}

/**
 * @brief 设置语音活动检测回调函数
 *
 * @param audio_sr
 * @param callback
 * @param args
 */
void audio_sr_set_vad_callback(audio_sr_t *audio_sr,
                               void (*callback)(void *, vad_state_t),
                               void *args) {
  audio_sr->vad_callback = callback;
  audio_sr->vad_args = args;
}
/**
 * @brief 重置为未唤醒状态
 *
 * @param audio_sr
 */
void audio_sr_reset_wakenet(audio_sr_t *audio_sr) { audio_sr->is_waked = 0; }
