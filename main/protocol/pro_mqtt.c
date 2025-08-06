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

/**
 * @brief 初始化MQTT客户端
 *
 * 配置并启动MQTT客户端，连接到默认的MQTT服务器
 */
void pro_mqtt_init(void) {
  // 明确声明并初始化所有子结构体，防止未初始化字段导致连接失败
  esp_mqtt_client_config_t mqtt_cfg = {0};

  // ---- broker.address 子结构体 ----
  mqtt_cfg.broker.address.uri = DEFAULT_MQTT_URI; // 推荐直接用uri
  mqtt_cfg.broker.address.hostname =
      DEFAULT_MQTT_HOST; // 可选，如果没uri可以用host+port
  mqtt_cfg.broker.address.transport =
      MQTT_TRANSPORT_OVER_WS;                       // 新版transport类型
  mqtt_cfg.broker.address.path = DEFAULT_MQTT_PATH; // ws用/mqtt
  mqtt_cfg.broker.address.port = DEFAULT_MQTT_PORT;

  // ---- 可选项（比如加认证/客户端ID等）----
  // mqtt_cfg.credentials.username = "your_username";
  // mqtt_cfg.credentials.password = "your_password";
  // mqtt_cfg.credentials.client_id = "your_clientid";

  // ---- 其他（根据需要补充） ----

  // 初始化MQTT客户端
  client = esp_mqtt_client_init(&mqtt_cfg);
  // 注册MQTT事件回调函数
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler,
                                 NULL);
  // 启动MQTT客户端
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