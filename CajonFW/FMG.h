#ifndef _FNG_H_
#define _FMG_H_

#include <queue.h>
#include <map>
#include <vector>
#include "REQ.h"

// キューの定義
extern QueueHandle_t g_pstFMGQueue;

// 要求の種類
enum FMGRequestType {
  FMG_SD_SET = FMG_REQ_BEGIN,
  FMG_SD_CLR,
  FMG_GET_SND_LIST,
  FMG_OPEN,
  FMG_OPEN_ANS,
  FMG_READ,
  FMG_READ_ANS,
  FMG_CLOSE,
  FMG_CLOSE_ANS,
};

// 要求の構造体

typedef struct STagFMGOpenParam {
  char ucFileName[32];
}TS_FMGOpenParam;

typedef struct STagFMGReadParam {
  uint8_t* pucBuffer;
  uint32_t ulLength;
}TS_FMGReadParam;

typedef struct STagFMGReadAns {
  uint32_t ulLength;
}TS_FMGReadAns;

extern void FMGTask(void* pvParameters);
extern std::vector<String> htmlFiles;
extern std::vector<String> midFiles;

#endif
