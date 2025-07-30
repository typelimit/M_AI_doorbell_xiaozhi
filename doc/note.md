<!-- title: 小智AI项目笔记 -->
- [Bug List](#bug-list)
  - [Bug-App-ws](#bug-app-ws)
- [NOTE List](#note-list)
  - [NOTE-audio-encode 对环形缓冲区数据的使用](#note-audio-encode-对环形缓冲区数据的使用)
  - [NOTE-pro-http 没看懂的free](#note-pro-http-没看懂的free)
  - [NOTE-bsp-mutable-buffer 没看懂的可边长数组内存机制](#note-bsp-mutable-buffer-没看懂的可边长数组内存机制)

# Bug List

## Bug-App-ws 


# NOTE List

## NOTE-audio-encode 对环形缓冲区数据的使用
```c
static void audio_encode_read_input_buffer(RingbufHandle_t input_buffer,
                                           int max_size, uint8_t *data) {
  size_t read_len = 0;
  void *read_data = NULL;
  while (max_size) {
    // 从缓冲区读取最多max_size字节个数据到read_data当中
    read_data = xRingbufferReceiveUpTo(input_buffer, &read_len, portMAX_DELAY,
                                       max_size);
    
    // 把读到的数据拷贝到外部传入的指针data上，话说这一步到底是给*data这个地址上赋值，还是把*data指向了read_data?
    memcpy(data, read_data, read_len);
    data += read_len;
    max_size -= read_len;

    // 释放资源
    vRingbufferReturnItem(input_buffer, read_data);
    read_len = 0;
  }
}
```
关于xRingbufferReceiveUpTo这个函数，它读取的对象是input_buffer即之前声明出的环形缓冲，
它返回一个存储着具体数据的指针作为结果，max_size应该就是它预期读取的数据长度
read_len应该就是它本次读取到的数据个数，但是这是为什么呢？
————max_size是它能读取的上限，不是它的预期读取长度。

upto的意思不是直到吗？
————不是，是“最大到……”的意思。
那么它应该是直到读到max_size个数据之前都不会输出的啊，后面延时部分不是写了portMAX_DELAY吗？
翻译过来就是说，如果没有从环形缓冲中读到足够的数据，它根本就不会把read_data这个指针吐出来。
那后面的这个对*data进行移动操作、对max_size操作到底有什么用？

read_len应该是一直等于max_size的，除非这里的超时判断用的不是portMAX_DELAY才有可能让它提前吐值。
————错误，超时

还有这个释放资源，在死等的情况下还好，毕竟一定会等到数据足够才会从xRingbufferReceiveUpTo这行代码走出来，所以之后把读数据的指针*read_data释放掉也没关系，但是如果真的给它加个超时时间，那不是会变成把读指针归还给环形缓冲input_buffer吗？下一次再用xRingbufferReceiveUpTo申请会把指针申请到哪里？

“最多max_size字节”的数据块的指针——这句话是什么意思？我能理解指针指向内存地址上的具体某个字节，但是为什么指针还会包含“块”这个概念？

“块”的边界是写入时确定，读取时才有实际意义。写的人定义了原子单位，读的人必须以写端原子为粒度操作，不能随便掰开。

nvs和flash的区别是什么？



## NOTE-pro-http 没看懂的free

```c
void bsp_mutable_buffer_free(mutable_buffer_t *mutable_buffer) {
  // 这一步是什么意思？这个判断有什么意义？
  if (mutable_buffer->buffer) {

    free(mutable_buffer->buffer);
  }
  free(mutable_buffer);
}
```

## NOTE-bsp-mutable-buffer 没看懂的可边长数组内存机制
```c
void bsp_mutable_buffer_append_data(mutable_buffer_t *mutable_buffer,
                                    char *data, size_t len) {

  char *buf = (char *)heap_caps_realloc(
      mutable_buffer->buffer, mutable_buffer->size + len, MALLOC_CAP_SPIRAM);
  // 数据追加至缓存
  memcpy(buf + mutable_buffer->size, data, len);

  // 修改可变缓存属性
  mutable_buffer->size = mutable_buffer->size + len + 1;
  buf[mutable_buffer->size] = '\0';
  mutable_buffer->buffer = buf;
}
```