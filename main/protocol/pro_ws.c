#include "pro_ws.h"
#include "audio_process.h"
#include "bsp.h"
#include "cJSON.h"
#include "com_debug.h"
#include "com_status.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_transport_ssl.h"
#include "esp_websocket_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// 这里还需要这个extern吗？——————误会了，这个没有暴露在头文件，需要extern
extern audio_processor_t *audio_processor;

#define CONN_SUCCESS (1 << 0)

static esp_websocket_client_handle_t client; // 作为对象的客户端结构体
EventGroupHandle_t
    event_group; // 事件标志组————给ws_start发送通知，保证连接成功以后才继续

text_callback ws_text_callback; // 处理文本的回调函数指针
bin_callback ws_bin_callback;   // 处理音频的回调函数指针

char session_id[9]; // 被存储的会话id

/**
 * @brief 打印错误ws错误日志
 *
 * @param message
 * @param error_code
 */
static void log_error_if_nonzero(const char *message, int error_code) {

  if (error_code != 0) {
    MY_LOGE("Last error %s: 0x%x", message, error_code);
  }
}

/**
 * @brief 用于处理接收到的文本数据的回调函数
 *
 * @param cb
 */
void pro_ws_set_text_callback(text_callback cb) { ws_text_callback = cb; }

/**
 * @brief 用于处理接收到的音频数据(opus by websocket)的回调函数
 *
 * @param cb
 */
void pro_ws_set_bin_callback(bin_callback cb) { ws_bin_callback = cb; }

/**
 * @brief 处理各类接收到的ws数据的回调函数
 *
 * @param handler_args
 * @param base
 * @param event_id
 * @param event_data
 */
static void websocket_event_handler(void *handler_args, esp_event_base_t base,
                                    int32_t event_id, void *event_data) {

  esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
  switch (event_id) {

  case WEBSOCKET_EVENT_BEGIN:
    MY_LOGI("WEBSOCKET_EVENT_BEGIN");
    break;

  case WEBSOCKET_EVENT_CONNECTED:
    MY_LOGI("WEBSOCKET_EVENT_CONNECTED");
    xEventGroupSetBits(event_group, CONN_SUCCESS);
    break;

  case WEBSOCKET_EVENT_DISCONNECTED:
    MY_LOGI("WEBSOCKET_EVENT_DISCONNECTED");
    log_error_if_nonzero("HTTP status code",
                         data->error_handle.esp_ws_handshake_status_code);
    if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT) {
      log_error_if_nonzero("reported from esp-tls",
                           data->error_handle.esp_tls_last_esp_err);
      log_error_if_nonzero("reported from tls stack",
                           data->error_handle.esp_tls_stack_err);
      log_error_if_nonzero("captured as transport's socket errno",
                           data->error_handle.esp_transport_sock_errno);
    }
    break;

  case WEBSOCKET_EVENT_DATA:
    // MY_LOGI("WEBSOCKET_EVENT_DATA");
    // MY_LOGI("Received opcode=%d", data->op_code);
    if (data->op_code == 0x2) {

      if (ws_bin_callback) {

        ws_bin_callback(data->data_ptr, data->data_len);

      } else {

        MY_LOGE("未设置音频处理回调函数...");
      }

    } else if (data->op_code == 0x1) {

      if (ws_text_callback) {

        ws_text_callback(data->data_ptr, data->data_len);

      } else {

        MY_LOGE("未设置文本处理回调函数...");
      }
    }
    break;

  case WEBSOCKET_EVENT_ERROR:
    MY_LOGI("WEBSOCKET_EVENT_ERROR");
    log_error_if_nonzero("HTTP status code",
                         data->error_handle.esp_ws_handshake_status_code);
    if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT) {
      log_error_if_nonzero("reported from esp-tls",
                           data->error_handle.esp_tls_last_esp_err);
      log_error_if_nonzero("reported from tls stack",
                           data->error_handle.esp_tls_stack_err);
      log_error_if_nonzero("captured as transport's socket errno",
                           data->error_handle.esp_transport_sock_errno);
    }
    break;

  case WEBSOCKET_EVENT_FINISH:
    MY_LOGI("WEBSOCKET_EVENT_FINISH");
    audio_process_reset_wakenet(audio_processor);
    // WS结束事件时小智变为空闲————也就是说，这个时候小智进入待机状态，直到接收唤醒词才会再次监听？
    com_status_switch_status(IDLE);
    break;
  }
}

/**
 * @brief ws连接初始化
 *
 */
void pro_ws_init(void) {

  esp_websocket_client_config_t websocket_cfg = {
      .uri = "wss://api.tenclass.net/xiaozhi/v1/",
      .buffer_size = 2048,
      .transport = WEBSOCKET_TRANSPORT_OVER_SSL,
      .crt_bundle_attach = esp_crt_bundle_attach,
      .network_timeout_ms = 5000,
      .reconnect_timeout_ms = 5000,
  };

  // 创建ws连接
  client = esp_websocket_client_init(&websocket_cfg);
  // 注册回调
  esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY,
                                websocket_event_handler, (void *)client);

  // WSS协议需要配置头信息
  esp_websocket_client_append_header(client, "Authorization",
                                     "Bearer test-token");
  esp_websocket_client_append_header(client, "Protocol-Version", "1");
  esp_websocket_client_append_header(client, "Device-Id", bsp_get_mac_addr());
  esp_websocket_client_append_header(client, "Client-Id", bsp_get_uuid());

  event_group = xEventGroupCreate();
}

/**
 * @brief 开始连接服务器
 *
 */
void pro_ws_start(void) {

  if (client != NULL && !esp_websocket_client_is_connected(client)) {
    // 发起连接请求
    esp_websocket_client_start(client);
    // 等待连接成功————由事件标志组进行通知
    xEventGroupWaitBits(event_group, CONN_SUCCESS, pdTRUE, pdTRUE,
                        portMAX_DELAY);
  }
}

void pro_ws_close(void) {

  if (client != NULL && esp_websocket_client_is_connected(client)) {

    esp_websocket_client_close(client, portMAX_DELAY);
  }
}

/**
 * @brief 发送建立信道消息
 *
 */
void pro_ws_send_hello(void) {

  if (client != NULL && esp_websocket_client_is_connected(client)) {

    // 创建json对象
    cJSON *root_json = cJSON_CreateObject();
    cJSON_AddStringToObject(root_json, "transport", "websocket");
    cJSON_AddStringToObject(root_json, "type", "hello");
    cJSON_AddNumberToObject(root_json, "version", 1);

    cJSON *params = cJSON_CreateObject();
    cJSON_AddItemToObject(root_json, "audio_params", params);
    cJSON_AddStringToObject(params, "format", "opus");
    cJSON_AddNumberToObject(params, "channels", 1);
    cJSON_AddNumberToObject(params, "frame_duration", 60);
    cJSON_AddNumberToObject(params, "sample_rate", 16000);

    // 发送hello消息
    char *json_str = cJSON_PrintUnformatted(root_json);
    esp_websocket_client_send_text(client, json_str, strlen(json_str),
                                   portMAX_DELAY);

    // 释放资源
    free(json_str);
    cJSON_Delete(root_json);

  } else {

    MY_LOGE("客户端未初始化或未连接...");
  }
}

/**
 * @brief 发送唤醒词
 *
 */
void pro_ws_send_wakenet_word(void) {

  if (client != NULL && esp_websocket_client_is_connected(client)) {

    // 创建json对象
    cJSON *root_json = cJSON_CreateObject();
    cJSON_AddStringToObject(root_json, "session_id", session_id);
    cJSON_AddStringToObject(root_json, "state", "detect");
    cJSON_AddStringToObject(root_json, "text", "你好小智");
    cJSON_AddStringToObject(root_json, "type", "listen");

    // 发送消息
    char *json_str = cJSON_PrintUnformatted(root_json);
    esp_websocket_client_send_text(client, json_str, strlen(json_str),
                                   portMAX_DELAY);
    // 释放资源
    free(json_str);
    cJSON_Delete(root_json);
  } else {
    MY_LOGE("客户端未初始化或未连接...");
  }
}

/**
 * @brief 发送监听请求
 *
 */
void pro_ws_send_listen(void) {

  if (client != NULL && esp_websocket_client_is_connected(client)) {

    // 创建json对象
    cJSON *root_json = cJSON_CreateObject();
    cJSON_AddStringToObject(root_json, "session_id", session_id);
    cJSON_AddStringToObject(root_json, "mode", "auto");
    cJSON_AddStringToObject(root_json, "state", "start");
    cJSON_AddStringToObject(root_json, "type", "listen");

    // 将json转为字符串并发送
    char *json_str = cJSON_PrintUnformatted(root_json);
    esp_websocket_client_send_text(client, json_str, strlen(json_str),
                                   portMAX_DELAY);

    // 释放json资源
    free(json_str);
    cJSON_Delete(root_json);
  } else {
    MY_LOGE("客户端未初始化或未连接...");
  }
}

/**
 * @brief 发送断开连接请求
 *
 */
void pro_ws_send_stop(void) {

  if (client != NULL && esp_websocket_client_is_connected(client)) {

    // 创建json对象
    cJSON *root_json = cJSON_CreateObject();
    cJSON_AddStringToObject(root_json, "session_id", session_id);
    cJSON_AddStringToObject(root_json, "state", "stop");
    cJSON_AddStringToObject(root_json, "type", "listen");

    // 将json转为字符串并发送
    char *json_str = cJSON_PrintUnformatted(root_json);
    esp_websocket_client_send_text(client, json_str, strlen(json_str),
                                   portMAX_DELAY);

    // 释放json资源
    free(json_str);
    cJSON_Delete(root_json);
  } else {

    MY_LOGE("客户端未初始化或未连接...");
  }
}

/**
 * @brief 发送中断对话请求
 *
 */
void pro_ws_send_abort(void) {

  if (client != NULL && esp_websocket_client_is_connected(client)) {

    // 创建json对象
    cJSON *root_json = cJSON_CreateObject();
    cJSON_AddStringToObject(root_json, "session_id", session_id);
    cJSON_AddStringToObject(root_json, "reason", "wake_word_detected");
    cJSON_AddStringToObject(root_json, "type", "abort");

    // 将json转为字符串并发送
    char *json_str = cJSON_PrintUnformatted(root_json);
    esp_websocket_client_send_text(client, json_str, strlen(json_str),
                                   portMAX_DELAY);
    // 释放json资源
    free(json_str);
    cJSON_Delete(root_json);

  } else {

    MY_LOGE("客户端未初始化或者未连接.....");
  }
}

/**
 * @brief 发送opus语音数据
 *
 * @param data
 * @param len
 */
void pro_ws_send_opus(void *data, size_t len) {

  if (client != NULL && esp_websocket_client_is_connected(client)) {

    esp_websocket_client_send_bin(client, (char *)data, (int)len,
                                  portMAX_DELAY);
  } else {

    MY_LOGE("客户端未连接");
  }
}

void pro_ws_send_device_info(void) {
  if (client != NULL && esp_websocket_client_is_connected(client)) {

    const char *device_info =
        "{\"descriptors\":[{\"description\":\"Speaker\",\"methods\":{"
        "\"SetMute\":"
        "{\"description\":\"Set mute "
        "status\",\"parameters\":{\"mute\":{\"description\":\"Mute "
        "status\",\"type\":\"boolean\"}}},\"SetVolume\":{\"description\":\"Set "
        "volume level\",\"parameters\":{\"volume\":{\"description\":\"Volume "
        "level[0-100]\",\"type\":\"number\"}}}},\"name\":\"Speaker\","
        "\"properties\":{\"mute\":{\"description\":\"Mute "
        "status\",\"type\":\"boolean\"},\"volume\":{\"description\":\"Volume "
        "level[0-100]\",\"type\":\"number\"}}}],\"session_id\":\"%s\","
        "\"type\":\"iot\",\"update\":true}";

    size_t str_len = strlen(device_info) + strlen(session_id) + 16;
    char *json_str = heap_caps_malloc(str_len, MALLOC_CAP_SPIRAM);

    if (json_str == NULL) {
      MY_LOGE("json_str内存分配失败！！！");
      return;
    }

    snprintf(json_str, str_len, device_info, session_id);

    esp_websocket_client_send_text(client, json_str, strlen(json_str),
                                   portMAX_DELAY);
    heap_caps_free(json_str);
  } else {

    MY_LOGE("客户端未初始化或者未连接.....");
  }
}

void pro_ws_send_device_state(void) {

  if (client != NULL && esp_websocket_client_is_connected(client)) {

    const char *device_state =
        "{\"session_id\":\"%s\",\"states\":[{\"name\":\"Speaker\","
        "\"state\":{\"mute\":false,\"volume\":60}}],\"type\":\"iot\","
        "\"update\":"
        "true}";

    size_t str_len = strlen(device_state) + strlen(session_id) + 16;
    char *json_str = heap_caps_malloc(str_len, MALLOC_CAP_SPIRAM);

    if (json_str == NULL) {
      MY_LOGE("json_str内存分配失败！！！");
      return;
    }

    snprintf(json_str, str_len, device_state, session_id);

    esp_websocket_client_send_text(client, json_str, strlen(json_str),
                                   portMAX_DELAY);
    heap_caps_free(json_str);
  } else {
    MY_LOGE("客户端未初始化或者未连接.....");
  }
}