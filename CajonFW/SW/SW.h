#ifndef _SW_H_
#define _SW_H_

#include <FreeRTOS.h>
#include <queue.h>

// キューの定義
// 無し

// 要求の種類
// 無し


// 要求の構造体
typedef struct StagSWParam {
  int unShortPushReq;
  QueueHandle_t pstShorQue;
  int unLongPushReq;
  QueueHandle_t pstLongQue;
  TickType_t xTimeNow; 
}TS_SWParam;

// 初期化関数
extern void SWInit(void) ;

#endif