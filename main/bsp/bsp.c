#include "bsp.h"
#include "bsp_es8311.h"
#include "bsp_nvs.h"
#include "bsp_wifi.h"
#include "bsp_ws2812.h"
#include "esp_err.h"
#include "esp_system.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

char *mac_addr = NULL;
char *uuid = NULL;
static bsp_nvs_t *bsp_nvs;

/**
 * @brief 硬件设备初始化
 *
 */
void bsp_init(void) {
  bsp_wifi_init();
  bsp_es8311_init();
  bsp_es8311_open();

  bsp_ws2812_init();

  bsp_nvs = bsp_nvs_init();
}
/**
 * @brief 获取板子物理地址
 *
 * @return char*
 */
char *bsp_get_mac_addr(void) {

  // 判断内存中是否存在mac地址
  if (mac_addr != NULL) {
    return mac_addr;
  }

  // 申请内存空间
  mac_addr = (char *)malloc(18);

  // 获取wifi mac地址
  uint8_t eth_mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, eth_mac);

  // 拼接mac地址——按格式
  snprintf(mac_addr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", eth_mac[0],
           eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);

  mac_addr[17] = '\0';
  return mac_addr;
}
/**
 * @brief 获取uuid
 *
 * @return char*
 */
char *bsp_get_uuid(void) {
  // 判断内存中是否存在UUID
  if (uuid != NULL) {
    return uuid;
  }
  // 分配空间
  uuid = (char *)malloc(37);
  // 查询nvs当中是否由uuid
  esp_err_t err = bsp_nvs_is_key_exists(bsp_nvs, "UUID");
  if (err == ESP_OK) {
    // 读UUID并返回
    size_t len = 37;
    bsp_nvs_read_data(bsp_nvs, "UUID", uuid, &len);
    return uuid;

  } else {

    // 第一次随机生成一个UUID
    int i;
    unsigned char raw[16];
    // 推荐：srand只在main里全局初始化一次，不要每次都srand
    for (i = 0; i < 16; ++i) {
      raw[i] = rand() & 0xFF;
    }
    // UUID v4 版本号和variant位处理
    raw[6] = (raw[6] & 0x0F) | 0x40;
    raw[8] = (raw[8] & 0x3F) | 0x80;
    sprintf(
        uuid,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7], raw[8],
        raw[9], raw[10], raw[11], raw[12], raw[13], raw[14], raw[15]);

    // 将生成的UUID保存到NVS
    bsp_nvs_write_data(bsp_nvs, "UUID", uuid);
    // 返回UUID
    return uuid;
  }
}
