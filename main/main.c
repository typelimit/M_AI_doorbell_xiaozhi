#include "audio_decode.h"
#include "audio_encode.h"
#include "audio_process.h"
#include "audio_sr.h"
#include "bsp.h"
#include "bsp_es8311.h"
#include "bsp_wifi.h"
#include "bsp_ws2812.h"
#include "cJSON.h"
#include "com_debug.h"
#include "esp_avrc_api.h"
#include "esp_heap_caps.h"
#include "esp_vad.h"
#include "freertos/ringbuf.h"
#include "mbedtls/ssl.h"
#include "pro_https.h"
#include "pro_ws.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

audio_processor_t *audio_processor;

/**
 * @brief ws接收到文本数据回调函数
 *
 * @param args
 */
void app_ws_text_callback(const char *data, int len) {

  // 转json
  cJSON *root_json = cJSON_Parse(data);

  // 获取类型
  cJSON *type_json = cJSON_GetObjectItem(root_json, "type");
  char *type_str = cJSON_GetStringValue(type_json);

  // 判断文本数据类型
  if (strcmp("hello", type_str) == 0) {

    MY_LOGE("接收到hello消息...%.*s", len, data);
    cJSON *session_json = cJSON_GetObjectItem(root_json, "session_id");
    char *session_str = cJSON_GetStringValue(session_json);
    session_id = session_str;

  } else if (strcmp("stt", type_str) == 0) {

    MY_LOGE("接收到stt消息%.*s", len, data);

  } else if (strcmp("llm", type_str) == 0) {

    MY_LOGE("接收到llm消息%.*s", len, data);

  } else if (strcmp("tts", type_str) == 0) {

    // 取出状态数据
    cJSON *state_json = cJSON_GetObjectItem(root_json, "state");
    char *state_str = cJSON_GetStringValue(state_json);
    if (strcmp("start", state_str) == 0) {

      MY_LOGE("接收到tts_start消息%.*s", len, data);

    } else if (strcmp("stop", state_str) == 0) {

      MY_LOGE("接收到tts_stop消息%.*s", len, data);

    } else if (strcmp("sentence_start", state_str) == 0) {

      MY_LOGE("接收到tts_sentence_start消息%.*s", len, data);
    }
  }
}

/**
 * @brief ws接收到音频数据回调
 *
 * @param args
 * @param len
 */
void app_ws_bin_callback(const char *args, int len) {

  audio_process_write_data(audio_processor, (uint8_t *)args, (size_t)len);
}

/**
 * @brief SR唤醒词回调
 *
 * @param args
 */
void app_audio_wakenet_callback(void *args) { MY_LOGE("小智被唤醒..."); }

/**
 * @brief SR语音识别回调
 *
 * @param args
 * @param vad_status
 */
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

  // 初始化ws
  pro_ws_init();

  // 设置ws回调函数
  pro_ws_set_text_callback(app_ws_text_callback);
  pro_ws_set_bin_callback(app_ws_bin_callback);

  // 启动ws
  pro_ws_start();

  // 发送消息测试
  pro_ws_send_hello();
  pro_ws_send_wakenet_word();
  pro_ws_send_listen();

  // 初始化音频处理模块
  audio_processor = audio_process_init();

  // 设置音频相关回调函数
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
