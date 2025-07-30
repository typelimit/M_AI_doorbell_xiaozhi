#include "pro_https.h"
#include "bsp.h"
#include "bsp_mutable_buffer.h"
#include "cJSON.h"
#include "com_debug.h"
#include "esp_app_desc.h"
#include "esp_flash.h"
#include "esp_http_client.h"
#include "esp_partition.h"
#include <stdlib.h>
#include <string.h>

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {

  mutable_buffer_t *mutable_buffer = (mutable_buffer_t *)evt->user_data;
  switch (evt->event_id) {
  case HTTP_EVENT_ERROR:
    MY_LOGE("HTTP_EVENT_ERROR");
    break;

  case HTTP_EVENT_ON_CONNECTED:
    MY_LOGE("HTTP_EVENT_ON_CONNECTED");
    break;

  case HTTP_EVENT_HEADER_SENT:
    MY_LOGE("HTTP_EVENT_HEADER_SENT");
    break;

  case HTTP_EVENT_ON_HEADER:
    MY_LOGE("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
            evt->header_value);
    break;

  case HTTP_EVENT_ON_DATA:
    MY_LOGE("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    bsp_mutable_buffer_append_data(mutable_buffer, evt->data, evt->data_len);
    break;

  case HTTP_EVENT_ON_FINISH:
    MY_LOGE("HTTP_EVENT_ON_FINISH");
    char *data = bsp_mutable_buffer_get_data(mutable_buffer);
    MY_LOGE("接收到的完整响应数据：%s", data);
    break;

  case HTTP_EVENT_DISCONNECTED:
    MY_LOGI("HTTP_EVENT_DISCONNECTED");

    break;
  case HTTP_EVENT_REDIRECT:
    MY_LOGE("HTTP_EVENT_REDIRECT");
    break;
  }
  return ESP_OK;
}

/**
 * @brief 封装http请求体
 *
 * @return char*
 */
char *pro_https_get_body_str(void) {

  // 创建第一个JSON对象
  cJSON *root_json = cJSON_CreateObject();

  // 先处理第一层数据
  cJSON_AddStringToObject(root_json, "chip_model_name", "esp32s3");
  cJSON_AddStringToObject(root_json, "mac_address", bsp_get_mac_addr());
  cJSON_AddNumberToObject(root_json, "flash_size", 8 * 1024 * 1024);
  cJSON_AddNumberToObject(root_json, "minimum_free_heap_size",
                          esp_get_minimum_free_heap_size());
  cJSON_AddNumberToObject(root_json, "param_size", esp_psram_get_size());
  cJSON_AddNumberToObject(root_json, "version", 1);

  // 处理application
  cJSON *application = cJSON_CreateObject();
  const esp_app_desc_t *app_desc = esp_app_get_description();
  cJSON_AddStringToObject(application, "compile_time", app_desc->date);
  cJSON_AddStringToObject(application, "version", app_desc->version);
  cJSON_AddStringToObject(application, "name", app_desc->project_name);
  cJSON_AddStringToObject(application, "idf_version", app_desc->idf_ver);
  cJSON_AddStringToObject(application, "elf_sha256",
                          esp_app_get_elf_sha256_str());
  cJSON_AddItemToObject(root_json, "application", application);

  // board
  cJSON *board = cJSON_CreateObject();
  cJSON_AddItemToObject(root_json, "board", board);

  cJSON_AddNumberToObject(board, "channel", 1);
  cJSON_AddStringToObject(board, "ip", "192.168.1.100");
  cJSON_AddStringToObject(board, "ssid", "atguigu");
  cJSON_AddNumberToObject(board, "rssi", -50);
  cJSON_AddStringToObject(board, "mac", bsp_get_mac_addr());
  cJSON_AddStringToObject(board, "name", "bread-compact-wifi");
  cJSON_AddStringToObject(board, "type", "bread-compact-wifi");

  // chip_info
  esp_chip_info_t chip;
  esp_chip_info(&chip);

  cJSON *chip_info = cJSON_CreateObject();
  cJSON_AddItemToObject(root_json, "chip_info", chip_info);
  cJSON_AddNumberToObject(chip_info, "model", chip.model);
  cJSON_AddNumberToObject(chip_info, "features", chip.features);
  cJSON_AddNumberToObject(chip_info, "cores", chip.cores);
  cJSON_AddNumberToObject(chip_info, "revision", chip.revision);

  // partition_table
  cJSON *partition_table = cJSON_CreateArray();
  cJSON_AddItemToObject(root_json, "partition_table", partition_table);

  esp_partition_iterator_t it = esp_partition_find(
      ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  while (it) {
    const esp_partition_t *curr_par = esp_partition_get(it);
    // 处理这个partition信息
    cJSON *partition = cJSON_CreateObject();
    cJSON_AddItemToArray(partition_table, partition);

    cJSON_AddNumberToObject(partition, "address", curr_par->address);
    cJSON_AddNumberToObject(partition, "size", curr_par->size);
    cJSON_AddNumberToObject(partition, "type", curr_par->type);
    cJSON_AddNumberToObject(partition, "subtype", curr_par->subtype);
    cJSON_AddStringToObject(partition, "label", curr_par->label);
    // 迭代
    it = esp_partition_next(it);
  }
  esp_partition_iterator_release(it);

  // ota模块
  const esp_partition_t *running_partition = esp_ota_get_running_partition();
  cJSON *ota = cJSON_CreateObject();
  cJSON_AddItemToObject(root_json, "ota", ota);
  cJSON_AddStringToObject(ota, "label", running_partition->label);

  // 将json转为字符串
  char *json_str = cJSON_PrintUnformatted(root_json);
  cJSON_Delete(root_json);

  return json_str;
}

/**
 * @brief 发送注册请求——获取注册码
 *
 */
void pro_https_send_reg(void) {

  ESP_LOGI("MEM", "Free heap: %d, Largest block: %d", esp_get_free_heap_size(),
           heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));

  // 创建可变缓存用于接收响应数据
  mutable_buffer_t *mutable_buffer = bsp_mutable_buffer_init();

  // 1.http请求参数
  esp_http_client_config_t config = {
      .url = "https://api.tenclass.net/xiaozhi/ota/",
      .event_handler = _http_event_handler,
      .crt_bundle_attach = esp_crt_bundle_attach,
      .method = HTTP_METHOD_POST,
      .user_data = mutable_buffer,
  };

  // 2.初始化http连接
  esp_http_client_handle_t client = esp_http_client_init(&config);

  // 3.设置头信息,设置请求参数为JSON格式
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_header(client, "Accept", "application/json");

  esp_http_client_set_header(client, "Device-Id", bsp_get_mac_addr());
  esp_http_client_set_header(client, "Client-Id", bsp_get_uuid());
  esp_http_client_set_header(client, "Accept-Language", "zh-CN");
  esp_http_client_set_header(client, "User-Agent", "Version-1");

  // 4.设置body信息
  char *json_str = pro_https_get_body_str();
  MY_LOGE("JSON_STR=%s", json_str);
  esp_http_client_set_post_field(client, json_str, strlen(json_str));

  // 5.发送请求
  esp_http_client_perform(client);

  // 6.等待响应码
  int code = esp_http_client_get_status_code(client);
  if (code == 200) {

    MY_LOGE("发送请求成功...");
  } else {

    MY_LOGE("发送请求失败...%d", code);
  }

  // 7.收尾
  free(json_str);
  bsp_mutable_buffer_free(mutable_buffer);
  esp_http_client_cleanup(client);
}
