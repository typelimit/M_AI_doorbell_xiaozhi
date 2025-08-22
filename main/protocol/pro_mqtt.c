#include "pro_mqtt.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <string.h>

// 默认MQTT服务器URI
#define DEFAULT_MQTT_URI "ws://broker.emqx.io:8083/mqtt"
// 默认MQTT服务器主机地址
#define DEFAULT_MQTT_HOST "broker.emqx.io"
// 默认MQTT服务器端口
#define DEFAULT_MQTT_PORT 8083
// 默认MQTT路径
#define DEFAULT_MQTT_PATH "/mqtt"
// 默认MQTT主题
#define DEFAULT_MQTT_TOPIC "zhangziheng"

// 日志标签
static const char *TAG = "PRO_MQTT";
// MQTT客户端句柄
static esp_mqtt_client_handle_t client = NULL;
// MQTT主题字符串，最大长度64字节
static char mqtt_topic[64] = DEFAULT_MQTT_TOPIC;
// MQTT连接状态标志，volatile确保多线程环境下的可见性
static volatile bool is_connected = false;

void wait_for_mqtt_connected(void) {
  int retry = 0;
  while (!is_connected) {
    ESP_LOGW(TAG, "MQTT还没连上，等待重试 #%d ...", ++retry);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  ESP_LOGI(TAG, "MQTT连接成功！");
}

/**
 * @brief MQTT事件回调函数
 *
 * @param handler_args 用户注册的回调函数参数
 * @param base 事件基础类型
 * @param event_id 事件ID
 * @param event_data 事件数据
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  // 将事件数据转换为MQTT事件句柄
  esp_mqtt_event_handle_t event = event_data;
  // 根据事件ID处理不同的MQTT事件
  switch (event->event_id) {
  case MQTT_EVENT_CONNECTED:
    // MQTT连接成功事件
    ESP_LOGI(TAG, "MQTT connected!");
    // 设置连接状态为已连接
    is_connected = true;
    send_hello_to_mqtt();
    break;
  case MQTT_EVENT_DISCONNECTED:
    // MQTT断开连接事件
    ESP_LOGW(TAG, "MQTT disconnected!");
    // 设置连接状态为未连接
    is_connected = false;
    break;
  case MQTT_EVENT_ERROR:
    // MQTT发生错误事件
    ESP_LOGE(TAG, "MQTT event error");
    break;
  default:
    // 其他事件不处理
    break;
  }
}

void send_hello_to_mqtt(void) {
  // 构造JSON
  cJSON *hello_json = cJSON_CreateObject();
  cJSON_AddStringToObject(hello_json, "msg", "hello, this is XiaoZhi!!!!!");
  // 你可以添加更多字段，比如设备ID、时间戳等
  char *hello_str = cJSON_PrintUnformatted(hello_json);

  pro_mqtt_send_json(hello_str); // 直接发到当前默认topic

  cJSON_free(hello_str);
  cJSON_Delete(hello_json);
}

/**
 * @brief 初始化MQTT客户端
 *
 * 配置并启动MQTT客户端，连接到默认的MQTT服务器
 */
void pro_mqtt_init(void) {
  esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = DEFAULT_MQTT_URI,
      .network.disable_auto_reconnect = false,  // 自动重连
      .network.reconnect_timeout_ms = 5000,     // 断线后每5s自动重连
      .network.timeout_ms = 8000,               // 网络操作超时8秒
      .network.refresh_connection_after_ms = 0, // 0=不强制刷新
      .task.priority = 5,                       // 任务优先级可调整
      .task.stack_size = 8192,                  // 足够大防止栈溢出
      .buffer.size = 4096,                      // 收发缓冲区建议>=2k
      .buffer.out_size = 4096,
      .outbox.limit = 0,       // 不限制outbox（默认）
      .session.keepalive = 30, // 协议层保活心跳(秒)
      // 如果需要，可以加用户名、密码、client_id等
      //.credentials.username = "...",
      //.credentials.password = "...",
      //.credentials.client_id = "...",
  };

  client = esp_mqtt_client_init(&mqtt_cfg);
  if (client == NULL) {
    ESP_LOGE(TAG, "MQTT client init failed, check config!");
    return;
  }
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler,
                                 NULL);
  esp_mqtt_client_start(client);
}

/**
 * @brief 检查MQTT是否已连接
 *
 * @return true 已连接
 * @return false 未连接
 */
bool pro_mqtt_is_connected(void) { return is_connected; }

/**
 * @brief 发送JSON数据到MQTT服务器
 *
 * @param json 要发送的JSON字符串
 * @return true 发送成功
 * @return false 发送失败
 */
bool pro_mqtt_send_json(const char *json) {
  // 检查MQTT客户端是否已初始化且已连接
  if (client == NULL || !is_connected) {
    ESP_LOGE(TAG, "MQTT client not connected!");
    return false;
  }
  // 发布消息到指定主题
  int msg_id = esp_mqtt_client_publish(client, mqtt_topic, json, 0, 1, 0);
  // 检查发布是否成功
  if (msg_id < 0) {
    ESP_LOGE(TAG, "MQTT publish failed");
    return false;
  }
  // 记录发布成功的日志
  ESP_LOGI(TAG, "MQTT published to [%s]: %s", mqtt_topic, json);
  return true;
}

/**
 * @brief 设置MQTT主题
 *
 * @param topic 要设置的主题字符串
 */
void pro_mqtt_set_topic(const char *topic) {
  // 检查主题字符串是否有效且长度合适
  if (topic && strlen(topic) < sizeof(mqtt_topic)) {
    // 复制新主题到全局变量
    strcpy(mqtt_topic, topic);
    // 记录主题切换的日志
    ESP_LOGI(TAG, "MQTT topic switched to [%s]", mqtt_topic);
  } else {
    // 记录主题无效或过长的警告日志
    ESP_LOGW(TAG, "Topic is NULL or too long!");
  }
}