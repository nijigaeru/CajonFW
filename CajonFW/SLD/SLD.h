

#ifndef _SLD_H_
#define _SLD_H_

#include <FreeRTOS.h>
#include <queue.h>

// キューの定義
extern QueueHandle_t g_pstSLDueue;

// 要求の種類
enum SLDRequestType {
  SLD_TURN_ON
};


// 要求の構造体
typedef struct StagFETRequest {
  SLDRequestType type;
}TS_SLDRequest;

// 初期化関数
extern void SLDTask(void* pvParameters);

#endif
