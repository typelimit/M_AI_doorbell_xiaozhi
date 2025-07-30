#include "bsp_nvs.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stddef.h>
#include <stdlib.h>

struct bsp_nvs {
  nvs_handle_t nvs_handler;
};

bsp_nvs_t *bsp_nvs_init(void) {

  bsp_nvs_t *bsp_nvs = (bsp_nvs_t *)malloc(sizeof(bsp_nvs_t));
  // 初始化整个nvs空间
  nvs_flash_init();

  // 开辟一个小的存储空间
  nvs_open("BSP", NVS_READWRITE, &bsp_nvs->nvs_handler);

  return bsp_nvs;
}

esp_err_t bsp_nvs_is_key_exists(bsp_nvs_t *bsp_nvs, char *key) {
  return nvs_find_key(bsp_nvs->nvs_handler, key, NULL);
}

esp_err_t bsp_nvs_write_data(bsp_nvs_t *bsp_nvs, char *key, char *value) {

  esp_err_t err = nvs_set_str(bsp_nvs->nvs_handler, key, value);

  nvs_commit(bsp_nvs->nvs_handler);

  return err;
}

esp_err_t bsp_nvs_read_data(bsp_nvs_t *bsp_nvs, char *key, char *value,
                            size_t *len) {

  return nvs_get_str(bsp_nvs->nvs_handler, key, value, len);
}