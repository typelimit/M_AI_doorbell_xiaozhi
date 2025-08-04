#include "com_status.h"

Status xiaozhi_status = IDLE;

char *status_arr[] = {"START", "CONNECTED", "IDLE", "LISTEN", "SPEACKING"};

void com_status_switch_status(Status status) {
  // 判断小智的状态是否发生了改变
  if (xiaozhi_status != status) {

    MY_LOGE("切换状态：小智状态%s===>%s", status_arr[xiaozhi_status],
            status_arr[status]);
    // 更新小智状态
    xiaozhi_status = status;
  }
}
