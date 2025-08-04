#include "audio_decode.h"
#include "audio_encode.h"
#include "audio_process.h"
#include "audio_sr.h"

#include "bsp.h"
#include "bsp_display.h"
#include "bsp_es8311.h"
#include "bsp_wifi.h"
#include "bsp_ws2812.h"

#include "cJSON.h"
#include "com_debug.h"
#include "com_status.h"

#include "esp_avrc_api.h"
#include "esp_heap_caps.h"
#include "esp_vad.h"
#include "freertos/ringbuf.h"
#include "lwip/tcpbase.h"
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

  // 字符串转json
  cJSON *root_json = cJSON_Parse(data);

  // 获取类型
  cJSON *type_json = cJSON_GetObjectItem(root_json, "type");
  char *type_str = cJSON_GetStringValue(type_json);

  // 判断文本数据类型
  if (strcmp("hello", type_str) == 0) {

    MY_LOGE("接收到hello消息...%.*s", len, data);
    cJSON *session_json = cJSON_GetObjectItem(root_json, "session_id");
    char *session_str = cJSON_GetStringValue(session_json);

    if (session_str) {
      strncpy(session_id, session_str, sizeof(session_id) - 1);
      session_id[sizeof(session_id) - 1] = '\0';
    }

    xEventGroupSetBits(event_group, 2);

  } else if (strcmp("stt", type_str) == 0) {

    MY_LOGE("接收到stt消息%.*s", len, data);

  } else if (strcmp("llm", type_str) == 0) {

    MY_LOGE("接收到llm消息%.*s", len, data);

  } else if (strcmp("iot", type_str) == 0) {

    MY_LOGE("接收到iot消息%.*s", len, data);
    // 解析json——取出commonds
    cJSON *commands = cJSON_GetObjectItem(root_json, "commands");
    int size_cmd = cJSON_GetArraySize(commands);
    for (int i = 0; i < size_cmd; i++) {

      // 取出数组中的每单个项
      cJSON *item = cJSON_GetArrayItem(commands, i);
      cJSON *method = cJSON_GetObjectItem(item, "method");
      char *method_val = cJSON_GetStringValue(method);
      // 判断操作类型
      if (strcmp(method_val, "SetVolume") == 0) {
        // 取出第二层的项
        cJSON *parameters = cJSON_GetObjectItem(item, "parameters");
        // 取出第三层的项
        cJSON *volume = cJSON_GetObjectItem(parameters, "volume");
        // 取出项内的值
        int volume_val = (int)cJSON_GetNumberValue(volume);
        // 把取出来的值设置进去
        bsp_es8311_set_vol(volume_val);
        // 判断操作类型
      } else if (strcmp(method_val, "SetMute") == 0) {

        cJSON *parameters = cJSON_GetObjectItem(item, "parameters");
        cJSON *mute = cJSON_GetObjectItem(parameters, "mute");

        if (cJSON_IsTrue(mute)) {

          bsp_es8311_set_mute(true);
        } else {
          bsp_es8311_set_mute(false);
        }
      }
    }

  } else if (strcmp("tts", type_str) == 0) {

    // 取出状态数据
    cJSON *state_json = cJSON_GetObjectItem(root_json, "state");
    char *state_str = cJSON_GetStringValue(state_json);
    if (strcmp("start", state_str) == 0) {

      MY_LOGE("接收到tts_start消息%.*s", len, data);
      com_status_switch_status(SPEAKING); // 转状态标识小智开始说话
    } else if (strcmp("stop", state_str) == 0) {

      MY_LOGE("接收到tts_stop消息%.*s", len, data);
      com_status_switch_status(IDLE); // 转状态标识小智停止说话

    } else if (strcmp("sentence_start", state_str) == 0) {

      MY_LOGE("接收到tts_sentence_start消息%.*s", len, data);
      // 取出应答当中的文本数据
      cJSON *textJson = cJSON_GetObjectItem(root_json, "text");
      // 把文本数据呈现在屏幕上面
      bsp_display_show_text(cJSON_GetStringValue(textJson));
    }
  }
  // 释放资源
  cJSON_Delete(root_json);
}

/**
 * @brief ws接收到音频数据回调
 *
 * @param args
 * @param len
 */
void app_ws_bin_callback(const char *args, int len) {
  // 添加判断————只有小智为说话状态时
  if (xiaozhi_status == SPEAKING) {
    // 才将数据写入解码输入缓冲
    audio_process_write_data(audio_processor, (uint8_t *)args, (size_t)len);
  }
}

/**
 * @brief SR唤醒词回调
 *
 * @param args
 */
void app_audio_wakenet_callback(void *args) {

  MY_LOGE("小智被唤醒...");
  // 若小智当前为空闲状态
  if (xiaozhi_status == IDLE) {
    // 则状态切换为连接
    com_status_switch_status(CONNECTED);
    // 进行ws连接并发送唤醒词
    pro_ws_start();
    pro_ws_send_hello();

    // 发送设备信息以及当前状态
    pro_ws_send_device_info();
    pro_ws_send_device_state();

    // 若小智为说话状态
  } else if (xiaozhi_status == SPEAKING) {
    // 则给服务器发送打断请求
    pro_ws_send_abort();
  }

  pro_ws_send_wakenet_word();
  com_status_switch_status(IDLE);
}

/**
 * @brief SR语音识别回调————
 *
 * @param args
 * @param vad_status
 */
void app_audio_vad_callback(void *args, vad_state_t vad_status) {
  if (vad_status == VAD_SILENCE) {
    MY_LOGE("声音活动检测——VAD_SILENCE");
    // 当用户没有说话而且小智为监听时
    if (xiaozhi_status == LISTENING) {
      // 给小智服务器发送断开连接请求
      pro_ws_send_stop();
      com_status_switch_status(IDLE);
    }
  } else if (vad_status == VAD_SPEECH) {
    MY_LOGE("声音活动检测——VAD_SPEECH");
    // 当用户说话而且小智为空闲的时候
    if (xiaozhi_status == IDLE) {
      // 给小智服务器发送监听用户请求
      pro_ws_send_listen();
      com_status_switch_status(LISTENING);
      // 小智必须为空闲————保证了小智不会把自己说话时的内容接收进来
    }
  }
}

void app_main(void) {

  com_status_switch_status(START);

  // 板级初始化
  bsp_init();

  // 创建客户端并直接发送注册请求
  pro_https_send_reg();

  // 初始化ws
  pro_ws_init();

  // 设置ws回调函数
  pro_ws_set_text_callback(app_ws_text_callback);
  pro_ws_set_bin_callback(app_ws_bin_callback);

  // 初始化音频处理模块
  audio_processor = audio_process_init();

  // 设置音频相关回调函数
  audio_process_set_wakenet_callback(audio_processor,
                                     app_audio_wakenet_callback);
  audio_process_set_vad_callback(audio_processor, app_audio_vad_callback);

  // 启动音频处理模块
  audio_process_start(audio_processor);

  // 小智状态转为为空闲
  com_status_switch_status(IDLE);

  size_t len = 0;
  while (1) {

    // 从音频处理模块中读取数据
    void *data = audio_process_read_data(audio_processor, &len);

    if (len > 0) {

      // 把音频数据发给小智服务器
      pro_ws_send_opus(data, len);
      // 释放资源
      free(data);
    }

    vTaskDelay(5);
  }
}
