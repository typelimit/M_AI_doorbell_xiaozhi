#ifndef __BSP_MUTABLE_BUFFER_H__
#define __BSP_MUTABLE_BUFFER_H__

#include <stddef.h>

typedef struct mutable_buffer mutable_buffer_t;

mutable_buffer_t *bsp_mutable_buffer_init(void);

void bsp_mutable_buffer_append_data(mutable_buffer_t *mutable_buffer,
                                    char *data, size_t len);

char *bsp_mutable_buffer_get_data(mutable_buffer_t *mutable_buffer);

void bsp_mutable_buffer_free(mutable_buffer_t *mutable_buffer);

#endif /* __BSP_MUTABLE_BUFFER_H__ */