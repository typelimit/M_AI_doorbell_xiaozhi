#ifndef __COM_STATUS_H__
#define __COM_STATUS_H__
#include "com_debug.h"

typedef enum { START, CONNECTED, IDLE, LISTENING, SPEAKING } Status;

extern Status xiaozhi_status;

void com_status_switch_status(Status status);

#endif /* __COM_STATUS_H__ */