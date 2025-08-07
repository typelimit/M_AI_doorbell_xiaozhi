/*
 * @Author: WHAlex
 * @Date: 2025-07-25 13:56:33
 *
 * Copyright (c) 2025 by atguigu, All Rights Reserved.
 */
#include "bsp_ws2812.h"

static led_strip_handle_t led_strip;

/**
 * @description: 照明模块初始化
 * @return {*}
 */
void bsp_ws2812_init(void)
{
    // 灯带配置
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN,                        // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_COUNT,                             // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,                               // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        }};

    // RMT模块初始化
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,                    // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ,             // RMT counter clock frequency
        .mem_block_symbols = LED_STRIP_MEMORY_BLOCK_WORDS, // the memory block size used by the RMT channel
        .flags = {
            .with_dma = LED_STRIP_USE_DMA, // Using DMA can improve performance when driving more LEDs
        }};

    // 创建灯带操作句柄
    led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
}

/**
 * @description: 开灯
 * @return {*}
 */
void bsp_ws2812_Led_On(void)
{
    led_strip_set_pixel(led_strip, 0, 50, 50, 50);
    led_strip_set_pixel(led_strip, 1, 50, 50, 50);

    led_strip_refresh(led_strip);
}

/**
 * @description: 关灯
 * @return {*}
 */
void bsp_ws2812_Led_Off(void)
{
    led_strip_clear(led_strip);
    led_strip_refresh(led_strip);
}