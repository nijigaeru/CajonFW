#ifndef _SW_H_
#define _SW_H_

#include <M5Unified.h>
#include <queue.h>

// キューの定義
// 無し

// 要求の種類
// 無し


// 要求の構造体
typedef struct StagSWParam {
  uint16_t unShortPushReq;
  QueueHandle_t pstShorQue;
  uint16_t unLongPushReq;
  QueueHandle_t pstLongQue;
  TickType_t xTimeNow; 
}TS_SWParam;

// 初期化関数
extern void SWInit(void) ;

#endif
