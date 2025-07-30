#ifndef __BSP_NVS_H__
#define __BSP_NVS_H__

#include "esp_flash.h"
#include "esp_log.h"
#include <stddef.h>
#include <sys/_intsup.h>

typedef struct bsp_nvs bsp_nvs_t;

bsp_nvs_t *bsp_nvs_init(void);

esp_err_t bsp_nvs_is_key_exists(bsp_nvs_t *bsp_nvs, char *key);

esp_err_t bsp_nvs_write_data(bsp_nvs_t *bsp_nvs, char *key, char *value);

esp_err_t bsp_nvs_read_data(bsp_nvs_t *bsp_nvs, char *key, char *value,size_t *len);

#endif /* __BSP_NVS_H__ */