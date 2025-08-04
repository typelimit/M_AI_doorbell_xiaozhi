#include "bsp_display.h"
#include "bsp_lcd.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style_gen.h"
#include "display/lv_display.h"
#include "esp_lvgl_port.h"
#include "font/lv_font.h"
#include "font_emoji.h"
#include "misc/lv_area.h"
#include "misc/lv_types.h"
#include "widgets/label/lv_label.h"
#include <string.h>

LV_FONT_DECLARE(font_puhui_14_1)

typedef struct {
  const char *icon;
  const char *text;
} emotion_t;
// 用于存储emoji的结构体数组
static const emotion_t emotions[] = {
    {"😶", "neutral"},   {"🙂", "happy"},   {"😆", "laughing"},
    {"😂", "funny"},     {"😔", "sad"},     {"😠", "angry"},
    {"😭", "crying"},    {"😍", "loving"},  {"😳", "embarrassed"},
    {"😯", "surprised"}, {"😱", "shocked"}, {"🤔", "thinking"},
    {"😉", "winking"},   {"😎", "cool"},    {"😌", "relaxed"},
    {"🤤", "delicious"}, {"😘", "kissy"},   {"😏", "confident"},
    {"😴", "sleepy"},    {"😜", "silly"},   {"🙄", "confused"},
};

lv_obj_t *text_label;
lv_obj_t *emoji_label;

void bsp_display_init(void) {

  /* Initialize LVGL */
  const lvgl_port_cfg_t lvgl_cfg = {
      .task_priority = 4,  /* LVGL task priority */
      .task_stack = 4096,  /* LVGL task stack size */
      .task_affinity = -1, /* LVGL task pinned to core (-1 is no affinity) */
      .task_max_sleep_ms = 500, /* Maximum sleep in LVGL task */
      .timer_period_ms = 5      /* LVGL timer tick period in ms */
  };
  lvgl_port_init(&lvgl_cfg);

  /* Add LCD screen */
  MY_LOGD("Add LCD screen");
  const lvgl_port_display_cfg_t disp_cfg = {.io_handle = io_handle,
                                            .panel_handle = panel_handle,
                                            .buffer_size = LCD_H_RES * 10,
                                            .double_buffer = true,
                                            .hres = LCD_H_RES,
                                            .vres = LCD_V_RES,
                                            .monochrome = false, // 单色模式
                                            .color_format =
                                                LV_COLOR_FORMAT_RGB565,
                                            .rotation =
                                                {
                                                    .swap_xy = false,
                                                    .mirror_x = true,
                                                    .mirror_y = true,
                                                },
                                            .flags = {
                                                .buff_dma = true,
                                                .swap_bytes = false,
                                            }};
  lvgl_port_add_disp(&disp_cfg);

  // 创建显示屏
  lv_obj_t *screen = lv_screen_active();

  // 默认显示内容
  lvgl_port_lock(0);

  text_label = lv_label_create(screen);
  lv_label_set_long_mode(text_label,
                         LV_LABEL_LONG_WRAP); /*Break the long lines*/
  lv_label_set_text(text_label, "尚硅谷小智AI聊天机器人");
  lv_obj_set_width(text_label,
                   LCD_H_RES); /*Set smaller width to make the lines wrap*/
  lv_obj_set_style_text_font(text_label, &font_puhui_14_1, 0);
  lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(text_label, LV_ALIGN_CENTER, 0, 0);

  emoji_label = lv_label_create(screen);
  lv_label_set_long_mode(emoji_label, LV_LABEL_LONG_MODE_WRAP);
  lv_label_set_text(emoji_label, "😆");
  lv_obj_set_width(emoji_label, LCD_H_RES);
  lv_obj_set_style_text_align(emoji_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(emoji_label, font_emoji_64_init(), 0);
  lv_obj_align(emoji_label, LV_ALIGN_CENTER, 0, -50);

  lvgl_port_unlock();
}

void bsp_display_show_text(char *text) {

  lvgl_port_lock(0);
  lv_label_set_text(text_label, text);
  lvgl_port_unlock();
}

/**
 * @brief 在显示屏上显示emoji
 *
 * @param emotion WS传过来的字符串，其内容为emotion
 */
void bsp_display_show_emoji(char *emotion) {
  // 计算结构体数组当中有多少个结构体
  int arr_size = sizeof(emotions) / sizeof(emotion_t);
  // 对该数组进行遍历
  for (int i = 0; i < arr_size; i++) {

    emotion_t emotion_item = emotions[i];
    // 把元素取出来与字符串进行比对
    if (strcmp(emotion, emotion_item.text) == 0) {
      lvgl_port_lock(0);
      lv_label_set_text(emoji_label, emotion_item.icon);
      lvgl_port_unlock();

      return;
    }
  }
  // 给定默认表情
  char *emoji = "😳";
  lvgl_port_lock(0);
  lv_label_set_text(emoji_label, emoji);
  lvgl_port_unlock();
}