#ifndef __BSP_LCD_H__
#define __BSP_LCD_H__

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

#include "com_debug.h"
#include "esp_lcd_types.h"

#define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL 1
#define LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define PIN_NUM_SCLK 47
#define PIN_NUM_MOSI 48
#define PIN_NUM_MISO 18
#define PIN_NUM_LCD_DC 45
#define PIN_NUM_LCD_RST 16
#define PIN_NUM_LCD_CS 21
#define PIN_NUM_BK_LIGHT 40

#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8

#define LCD_HOST SPI2_HOST

extern esp_lcd_panel_io_handle_t io_handle;
extern esp_lcd_panel_handle_t panel_handle;

void bsp_lcd_init(void);

#endif /* __BSP_LCD_H__ */