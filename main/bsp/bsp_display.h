#ifndef __BSP_DISPLAY_H__
#define __BSP_DISPLAY_H__

#include "bsp_lcd.h"
#include "esp_lvgl_port.h"

#define LCD_H_RES 240
#define LCD_V_RES 320

void bsp_display_init(void);

void bsp_display_show_text(char *text);

void bsp_display_show_emoji(char *emoji);

#endif /* __BSP_DISPLAY_H__ */