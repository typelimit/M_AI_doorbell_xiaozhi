#include "audio_decode.h"
#include "audio_encode.h"
#include "audio_process.h"
#include "audio_sr.h"
#include "bsp.h"
#include "bsp_es8311.h"
#include "bsp_wifi.h"
#include "bsp_ws2812.h"
#include "com_debug.h"
#include "esp_avrc_api.h"
#include "esp_heap_caps.h"
#include "esp_vad.h"
#include "freertos/ringbuf.h"
#include "pro_https.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void app_audio_wakenet_callback(void *args) { MY_LOGE("小智被唤醒..."); }

void app_audio_vad_callback(void *args, vad_state_t vad_status) {
  if (vad_status == VAD_SILENCE) {
    MY_LOGE("声音活动检测——安静");
  } else {
    MY_LOGE("声音活动检测——说话");
  }
}

void app_main(void) {

  // 板级初始化
  bsp_init();

  // 创建客户端并直接发送注册请求
  pro_https_send_reg();

  // 初始化音频处理模块
  audio_processor_t *audio_processor = audio_process_init();

  // 设置相关回调函数
  audio_process_set_wakenet_callback(audio_processor,
                                     app_audio_wakenet_callback);
  audio_process_set_vad_callback(audio_processor, app_audio_vad_callback);

  // 启动音频处理模块
  audio_process_start(audio_processor);

  size_t len = 0;
  while (1) {
    // 从音频处理模块中读取数据
    void *data = audio_process_read_data(audio_processor, &len);

    if (len > 0) {
      // MY_LOGE("从音频处理模块中读取数据成功...");

      // 将数据再写回音频处理模块
      audio_process_write_data(audio_processor, data, len);

      // 释放资源
      free(data);
    }

    vTaskDelay(10);
  }
}
