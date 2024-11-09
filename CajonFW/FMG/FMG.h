#ifndef _SD_H_
#define _SD_H_

#include <FreeRTOS.h>
#include <queue.h>

// キューの定義
extern QueueHandle_t g_pstFMGQueue;

// 要求の種類
enum FMGRequestType {
  FMG_SD_SET
  FMG_OPEN,
  FMG_CLOSE,
  FMG_READ
};

// 要求の構造体
typedef struct STagFMGRequest {
  FMGRequestType type;
  char fileName[32];
  uint8_t* buffer;
  size_t length;
}TS_FMGRequest;

extern void FMGTask(void* pvParameters);

#endif