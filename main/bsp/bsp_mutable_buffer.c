#include "bsp_mutable_buffer.h"
#include "esp_heap_caps.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct mutable_buffer {
  char *buffer;
  size_t size;
};

mutable_buffer_t *bsp_mutable_buffer_init(void) {
  // 给结构体对象分配内存
  mutable_buffer_t *mutable_buffer = (mutable_buffer_t *)heap_caps_malloc(
      sizeof(mutable_buffer_t), MALLOC_CAP_SPIRAM);
  memset(mutable_buffer, 0, sizeof(mutable_buffer_t));
  return mutable_buffer;
}

void bsp_mutable_buffer_append_data(mutable_buffer_t *mutable_buffer,
                                    char *data, size_t len) {

  size_t size = mutable_buffer->size + len + 1;
  printf("申请内存空间大小 = %d\r\n", size);

  char *buf = (char *)heap_caps_realloc(mutable_buffer->buffer, size,
                                        MALLOC_CAP_SPIRAM);

  // 数据追加至缓存
  memcpy(buf + mutable_buffer->size, data, len);

  // 修改可变缓存属性
  mutable_buffer->size = mutable_buffer->size + len;
  buf[mutable_buffer->size] = '\0';
  mutable_buffer->buffer = buf;
}

char *bsp_mutable_buffer_get_data(mutable_buffer_t *mutable_buffer) {
  return mutable_buffer->buffer;
}

void bsp_mutable_buffer_free(mutable_buffer_t *mutable_buffer) {
  // 这一步是什么意思？这个判断有什么意义？
  if (mutable_buffer->buffer) {

    heap_caps_free(mutable_buffer->buffer);
  }
  heap_caps_free(mutable_buffer);
}