#ifndef _FNG_H_
#define _FMG_H_

#include <queue.h>
#include <map>
#include <vector>
#include "reqid.h"

// キューの定義
extern QueueHandle_t g_pstFMGQueue;

// 要求の種類
enum FMGRequestType {
  FMG_SD_SET = FMG_REQ_BEGIN,
  FMG_SD_CLR,
  FMG_GET_SND_LIST,
  FMG_READ
};

// 要求の構造体
typedef struct STagFMGRequest {
  uint16_t unReqType;
  uint16_t unReserve;
  char ucFileName[32];
  uint8_t* pucBuffer;
  uint32_t ulLength;
}TS_FMGRequest;

extern void FMGTask(void* pvParameters);
extern std::vector<String> htmlFiles;
extern std::vector<String> midFiles;

#endif
