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

#include "pro_mqtt.h"

#define LOG_WS_IN(fmt, ...) MY_LOGI("[WS  IN ] " fmt, ##__VA_ARGS__)
#define LOG_WS_OUT(fmt, ...) MY_LOGI("[WS  OUT] " fmt, ##__VA_ARGS__)
#define LOG_MQTT_OUT(fmt, ...) MY_LOGI("[MQTT OUT] " fmt, ##__VA_ARGS__)
#define LOG_MQTT_IN(fmt, ...) MY_LOGI("[MQTT IN ] " fmt, ##__VA_ARGS__)
#define LOG_SYS(fmt, ...) MY_LOGI("[SYS     ] " fmt, ##__VA_ARGS__)
#define LOG_USER(fmt, ...) MY_LOGI("[USER    ] " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) MY_LOGW("[WARN    ] " fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) MY_LOGE("[ERROR   ] " fmt, ##__VA_ARGS__)

extern void wait_for_mqtt_connected(void);

// 全局音频处理器实例，用于处理音频数据的编码、解码和语音识别
audio_processor_t *audio_processor;

static int last_targetSpeed = -1;
static int last_motorStatus = -1;

/**
 * @brief ws接收到文本数据回调函数
 *
 * @param args
 */
void app_ws_text_callback(const char *data, int len) {
  if (!data || len <= 0) {
    LOG_WARN("收到空数据，丢弃");
    return;
  }
  cJSON *root_json = cJSON_Parse(data);
  if (!root_json) {
    LOG_ERR("JSON解析失败: %.*s", len, data);
    return;
  }

  cJSON *type_json = cJSON_GetObjectItem(root_json, "type");
  if (!cJSON_IsString(type_json) || !type_json->valuestring) {
    LOG_ERR("JSON无type字段或type不是字符串: %.*s", len, data);
    cJSON_Delete(root_json);
    return;
  }
  const char *type_str = cJSON_GetStringValue(type_json);

  LOG_WS_IN("<%s> 收到WS消息: %.*s", type_str, len, data);

  if (strcmp("hello", type_str) == 0) {
    cJSON *session_json = cJSON_GetObjectItem(root_json, "session_id");
    char *session_str = cJSON_GetStringValue(session_json);
    LOG_WS_IN("<hello> 握手成功，session_id=%s",
              session_str ? session_str : "(null)");
    if (session_str) {
      strncpy(session_id, session_str, sizeof(session_id) - 1);
      session_id[sizeof(session_id) - 1] = '\0';
    }
    xEventGroupSetBits(event_group, 2);

  } else if (strcmp("stt", type_str) == 0) {
    cJSON *text_json = cJSON_GetObjectItem(root_json, "text");
    const char *text_str = cJSON_GetStringValue(text_json);
    LOG_WS_IN("<stt> 语音识别结果: %s", text_str ? text_str : "(null)");
    if (!text_str || strlen(text_str) == 0) {
      LOG_WARN("STT识别为空，切回IDLE。原始: %.*s", len, data);
      com_status_switch_status(IDLE);
    }

  } else if (strcmp("llm", type_str) == 0) {
    LOG_WS_IN("<llm> LLM消息内容: %.*s", len, data);

  } else if (strcmp("iot", type_str) == 0) {
    cJSON *commands = cJSON_GetObjectItem(root_json, "commands");
    int size_cmd = cJSON_GetArraySize(commands);
    LOG_WS_IN("<iot> 收到IoT命令包，共%d条", size_cmd);

    for (int i = 0; i < size_cmd; i++) {
      cJSON *item = cJSON_GetArrayItem(commands, i);
      cJSON *name_json = cJSON_GetObjectItem(item, "name");
      char *name_str = cJSON_GetStringValue(name_json);
      if (!name_str)
        continue;
      cJSON *method_json = cJSON_GetObjectItem(item, "method");
      char *method_str = cJSON_GetStringValue(method_json);
      cJSON *parameters = cJSON_GetObjectItem(item, "parameters");

      // Speaker
      if (strcmp(name_str, "Speaker") == 0) {
        if (strcmp(method_str, "SetVolume") == 0) {
          int volume =
              cJSON_GetNumberValue(cJSON_GetObjectItem(parameters, "volume"));
          LOG_WS_IN("<iot> Speaker-SetVolume: volume=%d", volume);
          bsp_es8311_set_vol(volume);
        } else if (strcmp(method_str, "SetMute") == 0) {
          bool mute = cJSON_IsTrue(cJSON_GetObjectItem(parameters, "mute"));
          LOG_WS_IN("<iot> Speaker-SetMute: mute=%s", mute ? "true" : "false");
          bsp_es8311_set_mute(mute);
        }
      }
      // LED/WS2812
      else if (strcmp(name_str, "LED") == 0 ||
               strcmp(name_str, "WS2812") == 0) {
        if (strcmp(method_str, "SetPower") == 0) {
          bool power = cJSON_IsTrue(cJSON_GetObjectItem(parameters, "power"));
          LOG_WS_IN("<iot> %s-SetPower: power=%s", name_str,
                    power ? "on" : "off");
          if (power) {
            bsp_ws2812_Led_On();
          } else {
            bsp_ws2812_Led_Off();
          }
        }
      }
      // Display
      else if (strcmp(name_str, "Display") == 0) {
        if (strcmp(method_str, "SetBrightness") == 0) {
          int brightness = cJSON_GetNumberValue(
              cJSON_GetObjectItem(parameters, "brightness"));
          LOG_WS_IN("<iot> Display-SetBrightness: brightness=%d", brightness);
          bsp_lcd_set_brightness(brightness);
        }
      }
      // Motor
      else if (strcmp(name_str, "Motor") == 0) {
        int id = 1;
        char *connectType = "can";
        int targetSpeed = last_targetSpeed;
        int motorStatus = last_motorStatus;
        bool updated = false;

        if (strcmp(method_str, "SetSpeed") == 0 && parameters) {
          if (cJSON_HasObjectItem(parameters, "speed")) {
            targetSpeed =
                cJSON_GetNumberValue(cJSON_GetObjectItem(parameters, "speed"));
            updated = true;
            LOG_WS_IN("<iot> Motor-SetSpeed: speed=%d", targetSpeed);
          }
        }
        if (strcmp(method_str, "SetStatus") == 0 && parameters) {
          if (cJSON_HasObjectItem(parameters, "status")) {
            motorStatus =
                cJSON_IsTrue(cJSON_GetObjectItem(parameters, "status")) ? 1 : 0;
            updated = true;
            LOG_WS_IN("<iot> Motor-SetStatus: status=%d", motorStatus);
          }
        }

        if (updated && (targetSpeed != last_targetSpeed ||
                        motorStatus != last_motorStatus)) {
          cJSON *mqtt_json = cJSON_CreateObject();
          cJSON_AddNumberToObject(mqtt_json, "id", id);
          cJSON_AddStringToObject(mqtt_json, "connectType", connectType);
          cJSON_AddNumberToObject(mqtt_json, "targetSpeed", targetSpeed);
          cJSON_AddNumberToObject(mqtt_json, "motorStatus", motorStatus);

          char *mqtt_json_str = cJSON_PrintUnformatted(mqtt_json);
          LOG_MQTT_OUT("推送电机指令到MQTT: %s", mqtt_json_str);
          pro_mqtt_send_json(mqtt_json_str);

          cJSON_free(mqtt_json_str);
          cJSON_Delete(mqtt_json);

          last_targetSpeed = targetSpeed;
          last_motorStatus = motorStatus;
        } else {
          LOG_WS_IN("<iot> Motor命令未变化，不下发MQTT");
        }
      }
    }

  } else if (strcmp("tts", type_str) == 0) {
    cJSON *state_json = cJSON_GetObjectItem(root_json, "state");
    char *state_str = cJSON_GetStringValue(state_json);
    LOG_WS_IN("<tts> 状态: %s", state_str ? state_str : "(null)");
    if (strcmp("start", state_str) == 0) {
      LOG_SYS("小智开始说话（SPEAKING）");
      com_status_switch_status(SPEAKING);
    } else if (strcmp("stop", state_str) == 0) {
      LOG_SYS("小智停止说话（IDLE）");
      com_status_switch_status(IDLE);
    } else if (strcmp("sentence_start", state_str) == 0) {
      cJSON *textJson = cJSON_GetObjectItem(root_json, "text");
      LOG_SYS("句子开始，屏幕展示文本: %s", cJSON_GetStringValue(textJson));
      bsp_display_show_text(cJSON_GetStringValue(textJson));
    }
  }

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
    // 才将数据写入解码输入缓冲，避免处理非说话状态下的音频数据
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

  // 发送唤醒词信息到服务器
  pro_ws_send_wakenet_word();

  // 处理完唤醒事件后，将状态切换回空闲状态
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

  // 初始化小智状态为START状态
  com_status_switch_status(START);

  // 板级初始化，包括外设、引脚等硬件初始化
  bsp_init();

  // 创建客户端并直接发送注册请求，向服务器注册设备
  pro_https_send_reg();

  // 初始化WebSocket连接模块
  pro_ws_init();

  // 设置WebSocket回调函数，处理接收到的文本和二进制数据
  pro_ws_set_text_callback(app_ws_text_callback);
  pro_ws_set_bin_callback(app_ws_bin_callback);

  // 初始化音频处理模块
  audio_processor = audio_process_init();

  // 设置音频相关回调函数，处理唤醒和语音活动检测事件
  audio_process_set_wakenet_callback(audio_processor,
                                     app_audio_wakenet_callback);
  audio_process_set_vad_callback(audio_processor, app_audio_vad_callback);

  // 启动音频处理模块，开始进行音频采集和处理
  audio_process_start(audio_processor);

  // 初始化MQTT客户端，用于与其他设备通信
  pro_mqtt_init();

  wait_for_mqtt_connected();

  // 小智状态转为为空闲，准备接收用户交互
  com_status_switch_status(IDLE);

  size_t len = 0;
  while (1) {

    // 从音频处理模块中读取编码后的音频数据
    void *data = audio_process_read_data(audio_processor, &len);

    if (len > 0) {

      // 把音频数据通过WebSocket发送给小智服务器进行语音识别
      pro_ws_send_opus(data, len);
      // 释放资源
      free(data);
    }

    // 延时5个ticks，避免过度占用CPU资源
    vTaskDelay(5);
  }
}