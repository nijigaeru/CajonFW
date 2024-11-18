#ifndef _SLD_H_
#define _SLD_H_

#include <M5Unified.h>
#include <queue.h>
#include "reqid.h"

// キューの定義
extern QueueHandle_t g_pstSLDueue;

// 要求の種類
enum SLDRequestType {
  SLD_TURN_ON = SLD_REQ_BEGIN,
};


// 要求の構造体
typedef struct StagFETRequest {
  uint16_t unReqType;
  uint16_t unReserve;
}TS_SLDRequest;

// 初期化関数
extern void SLDTask(void* pvParameters);

#endif
