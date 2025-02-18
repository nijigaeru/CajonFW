#ifndef _SLD_H_
#define _SLD_H_

#include <queue.h>
#include "REQ.h"

#define SLD_NUM 8

// キューの定義
extern QueueHandle_t g_pstSLDQueue[SLD_NUM];

// 要求の種類
enum SLDRequestType {
  SLD_TURN_ON = SLD_REQ_BEGIN,
};


// 要求の構造体
typedef struct StagFETRequest {
  uint8_t  ucPower;
}TS_SLDOnParam;

// 初期化関数
extern void SLDTask(void* pvParameters);

#endif
