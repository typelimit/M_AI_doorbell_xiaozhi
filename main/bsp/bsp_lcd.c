#include "bsp_lcd.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_types.h"

esp_lcd_panel_io_handle_t io_handle;
esp_lcd_panel_handle_t panel_handle;

void bsp_lcd_init(void) {
  // 配置背光引脚
  MY_LOGI("Turn off LCD backlight");
  gpio_config_t bk_gpio_config = {.mode = GPIO_MODE_OUTPUT,
                                  .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT};
  ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

  // 安装SPI驱动
  MY_LOGI("Initialize SPI bus");
  spi_bus_config_t buscfg = {
      .sclk_io_num = PIN_NUM_SCLK,
      .mosi_io_num = PIN_NUM_MOSI,
      .miso_io_num = PIN_NUM_MISO,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  // 在spi总线上安装LCD
  MY_LOGI("Install panel IO");

  esp_lcd_panel_io_spi_config_t io_config = {
      .dc_gpio_num = PIN_NUM_LCD_DC,
      .cs_gpio_num = PIN_NUM_LCD_CS,
      .pclk_hz = LCD_PIXEL_CLOCK_HZ,
      .lcd_cmd_bits = LCD_CMD_BITS,
      .lcd_param_bits = LCD_PARAM_BITS,
      .spi_mode = 0,
      .trans_queue_depth = 10,
  };
  // Attach the LCD to the SPI bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST,
                                           &io_config, &io_handle));

  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = PIN_NUM_LCD_RST,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = 16,
  };

  esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);
  // 重置lcd
  esp_lcd_panel_reset(panel_handle);
  // 初始化lcd
  esp_lcd_panel_init(panel_handle);
  // 翻转颜色
  esp_lcd_panel_invert_color(panel_handle, true);
  // 镜像
  esp_lcd_panel_mirror(panel_handle, true, true);
  // 打开lcd
  esp_lcd_panel_disp_on_off(panel_handle, true);

  MY_LOGI("Turn on LCD backlight");
  gpio_set_level(PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);
}
