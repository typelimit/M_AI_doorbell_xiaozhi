#ifndef __BSP_H__
#define __BSP_H__

#include "esp_system.h"
#include "bsp_es8311.h"
#include "bsp_nvs.h"
#include "bsp_wifi.h"
#include "bsp_ws2812.h"


// typedef struct bsp_info bsp_info_t;

void bsp_init(void);

char *bsp_get_mac_addr(void);

char *bsp_get_uuid(void);

#endif /* __BSP_H__ */