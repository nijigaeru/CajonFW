#ifndef _READMID_H_
#define _READMID_H_

#include <queue.h>
#include <map>
#include <vector>
#include "REQ.h"

// キューの定義
extern QueueHandle_t g_pstREADMIDQueue;

// 要求の種類
enum READMIDRequestType {
  READMID_START = READMID_REQ_BEGIN,
};

// 要求の構造体
typedef struct STagREADMIDStartParam {
  char ucFileName[32];
}TS_READMIDStartParam;

extern void READMIDTask(void* pvParameters);

#endif
